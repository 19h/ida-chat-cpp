/**
 * @file agent_worker.cpp
 * @brief Background worker thread for running agent operations.
 */

#include <ida_chat/ui/agent_worker.hpp>
#include <ida_chat/core/script_executor.hpp>

#include <QFile>
#include <QDir>
#include <QTextStream>
#include <QDebug>

// IDA headers for msg() debug output
#include <ida_chat/common/ida_begin.hpp>
#include <ida.hpp>
#include <kernwin.hpp>
#include <ida_chat/common/ida_end.hpp>

namespace ida_chat {

// ============================================================================
// WorkerCallback Implementation
// ============================================================================

void AgentWorker::WorkerCallback::on_turn_start(int turn, int max_turns) {
    emit agent_sigs_.turn_start(turn, max_turns);
}

void AgentWorker::WorkerCallback::on_thinking() {
    emit agent_sigs_.thinking();
}

void AgentWorker::WorkerCallback::on_thinking_done() {
    emit agent_sigs_.thinking_done();
}

void AgentWorker::WorkerCallback::on_tool_use(const std::string& tool_name, const std::string& details) {
    emit agent_sigs_.tool_use(QString::fromStdString(tool_name), QString::fromStdString(details));
}

void AgentWorker::WorkerCallback::on_text(const std::string& text) {
    emit agent_sigs_.text(QString::fromStdString(text));
}

void AgentWorker::WorkerCallback::on_script_code(const std::string& code) {
    emit agent_sigs_.script_code(QString::fromStdString(code));
}

void AgentWorker::WorkerCallback::on_script_output(const std::string& output) {
    emit agent_sigs_.script_output(QString::fromStdString(output));
}

void AgentWorker::WorkerCallback::on_error(const std::string& error) {
    emit agent_sigs_.error(QString::fromStdString(error));
}

void AgentWorker::WorkerCallback::on_result(int num_turns, std::optional<double> cost) {
    emit agent_sigs_.result(num_turns, cost.value_or(0.0));
}

// ============================================================================
// AgentWorker Implementation
// ============================================================================

AgentWorker::AgentWorker(ScriptExecutorFn script_executor,
                         MessageHistory* history,
                         QObject* parent)
    : QThread(parent)
    , callback_(agent_signals_)
    , script_executor_(std::move(script_executor))
    , history_(history)
{
}

AgentWorker::~AgentWorker() {
    stop();
}

void AgentWorker::request_connect(const AuthCredentials& credentials) {
    std::lock_guard<std::mutex> locker(mutex_);
    pending_credentials_ = credentials;
    command_queue_.push({WorkerCommand::Connect, QString()});
    condition_.notify_one();
}

void AgentWorker::request_disconnect() {
    std::lock_guard<std::mutex> locker(mutex_);
    command_queue_.push({WorkerCommand::Disconnect, QString()});
    condition_.notify_one();
}

void AgentWorker::request_cancel() {
    std::lock_guard<std::mutex> locker(mutex_);
    command_queue_.push({WorkerCommand::Cancel, QString()});
    condition_.notify_one();
    
    // Also request cancel on the core if available
    if (core_) {
        core_->request_cancel();
    }
}

void AgentWorker::request_new_session() {
    std::lock_guard<std::mutex> locker(mutex_);
    command_queue_.push({WorkerCommand::NewSession, QString()});
    condition_.notify_one();
}

void AgentWorker::send_message(const QString& message) {
    std::lock_guard<std::mutex> locker(mutex_);
    command_queue_.push({WorkerCommand::SendMessage, message});
    condition_.notify_one();
}

bool AgentWorker::is_processing() const noexcept {
    return state_ == ChatState::Processing;
}

bool AgentWorker::is_connected() const noexcept {
    return state_ == ChatState::Idle || state_ == ChatState::Processing;
}

ChatState AgentWorker::get_state() const noexcept {
    return state_;
}

void AgentWorker::stop() {
    {
        std::lock_guard<std::mutex> locker(mutex_);
        running_ = false;
        command_queue_.push({WorkerCommand::Quit, QString()});
        condition_.notify_one();
    }
    
    // Cancel any ongoing operation
    if (core_) {
        core_->request_cancel();
    }
    
    // Wait for thread to finish
    if (isRunning()) {
        wait(5000);  // 5 second timeout
        if (isRunning()) {
            terminate();
            wait();
        }
    }
}

void AgentWorker::set_system_prompt(const QString& prompt) {
    std::lock_guard<std::mutex> locker(mutex_);
    system_prompt_ = prompt;
}

void AgentWorker::load_system_prompt(const QString& project_dir) {
    std::lock_guard<std::mutex> locker(mutex_);
    project_dir_ = project_dir;
    
    msg("[IDA Chat] load_system_prompt: project_dir='%s'\n", project_dir.toUtf8().constData());
    
    // Load the prompt synchronously
    QString prompt;
    
    QStringList files = {"PROMPT.md", "API_REFERENCE.md", "USAGE.md", "IDA.md"};
    for (const QString& file : files) {
        QString path = project_dir + "/" + file;
        QFile f(path);
        if (f.open(QIODevice::ReadOnly | QIODevice::Text)) {
            QTextStream in(&f);
            if (!prompt.isEmpty()) {
                prompt += "\n\n";
            }
            prompt += in.readAll();
            f.close();
            msg("[IDA Chat] load_system_prompt: loaded '%s' (%d chars)\n", 
                file.toUtf8().constData(), (int)prompt.size());
        } else {
            msg("[IDA Chat] load_system_prompt: FAILED to open '%s'\n", path.toUtf8().constData());
        }
    }
    
    system_prompt_ = prompt;
    msg("[IDA Chat] load_system_prompt: total prompt size = %d chars\n", (int)system_prompt_.size());
}

void AgentWorker::run() {
    running_ = true;
    
    while (running_) {
        WorkerCommand cmd = WorkerCommand::None;
        QString data;
        
        {
            std::unique_lock<std::mutex> locker(mutex_);
            
            // Wait for a command
            while (command_queue_.empty() && running_) {
                condition_.wait(locker);
            }
            
            if (!running_) break;
            
            if (!command_queue_.empty()) {
                auto [c, d] = command_queue_.front();
                command_queue_.pop();
                cmd = c;
                data = d;
            }
        }
        
        if (cmd != WorkerCommand::None) {
            process_command(cmd, data);
        }
    }
}

void AgentWorker::process_command(WorkerCommand cmd, const QString& data) {
    switch (cmd) {
        case WorkerCommand::Connect: {
            state_ = ChatState::Connecting;
            
            // Create ChatCore
            ChatCoreOptions options;
            core_ = std::make_unique<ChatCore>(callback_, script_executor_, history_, options);
            
            // Set system prompt if available
            if (!system_prompt_.isEmpty()) {
                core_->set_system_prompt(system_prompt_.toStdString());
            }
            
            // Connect
            if (core_->connect(pending_credentials_)) {
                state_ = ChatState::Idle;
                emit agent_signals_.connection_ready();
            } else {
                state_ = ChatState::Disconnected;
                emit agent_signals_.connection_error("Failed to connect to Claude API");
            }
            break;
        }
        
        case WorkerCommand::Disconnect: {
            if (core_) {
                core_->disconnect();
                core_.reset();
            }
            state_ = ChatState::Disconnected;
            break;
        }
        
        case WorkerCommand::SendMessage: {
            if (!core_ || !core_->is_connected()) {
                emit agent_signals_.error("Not connected");
                break;
            }
            
            state_ = ChatState::Processing;
            
            auto result = core_->process_message(data.toStdString());
            
            if (result.cancelled) {
                state_ = ChatState::Cancelled;
            } else if (!result.success) {
                emit agent_signals_.error(QString::fromStdString(result.error));
            }
            
            state_ = ChatState::Idle;
            emit agent_signals_.finished();
            break;
        }
        
        case WorkerCommand::NewSession: {
            if (core_) {
                core_->start_new_session();
            }
            break;
        }
        
        case WorkerCommand::Cancel: {
            if (core_) {
                core_->request_cancel();
            }
            state_ = ChatState::Cancelled;
            break;
        }
        
        case WorkerCommand::Quit:
            running_ = false;
            break;
            
        default:
            break;
    }
}

} // namespace ida_chat
