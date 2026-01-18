/**
 * @file agent_worker.hpp
 * @brief Background worker thread for running agent operations.
 */

#pragma once

#include <ida_chat/core/types.hpp>
#include <ida_chat/core/chat_core.hpp>
#include <ida_chat/core/chat_callback.hpp>
#include <ida_chat/history/message_history.hpp>
#include <ida_chat/ui/agent_signals.hpp>

#include <QThread>
#include <QString>

#include <memory>
#include <atomic>
#include <queue>
#include <functional>
#include <mutex>
#include <condition_variable>

namespace ida_chat {

/**
 * @brief Command types for the worker thread.
 */
enum class WorkerCommand {
    None,
    Connect,
    Disconnect,
    SendMessage,
    NewSession,
    Cancel,
    Quit
};

/**
 * @brief Background worker for running async agent calls.
 * 
 * Runs the ChatCore on a separate thread and communicates with
 * the UI thread via Qt signals.
 */
class AgentWorker : public QThread {
    Q_OBJECT

public:
    /**
     * @brief Create a worker.
     * @param script_executor Function to execute scripts on main thread
     * @param history Message history (optional)
     * @param parent Parent QObject
     */
    AgentWorker(ScriptExecutorFn script_executor,
                MessageHistory* history = nullptr,
                QObject* parent = nullptr);
    
    ~AgentWorker() override;
    
    /**
     * @brief Get the signals object for connecting to UI slots.
     */
    [[nodiscard]] AgentSignals* get_signals() { return &agent_signals_; }
    
    /**
     * @brief Request connection to Claude API.
     * @param credentials Authentication credentials
     */
    void request_connect(const AuthCredentials& credentials = {});
    
    /**
     * @brief Request disconnection.
     */
    void request_disconnect();
    
    /**
     * @brief Request cancellation of current operation.
     */
    void request_cancel();
    
    /**
     * @brief Request a new session.
     */
    void request_new_session();
    
    /**
     * @brief Send a message to the agent.
     * @param message The message to send
     */
    void send_message(const QString& message);
    
    /**
     * @brief Check if currently processing.
     */
    [[nodiscard]] bool is_processing() const noexcept;
    
    /**
     * @brief Check if connected.
     */
    [[nodiscard]] bool is_connected() const noexcept;
    
    /**
     * @brief Get the current state.
     */
    [[nodiscard]] ChatState get_state() const noexcept;
    
    /**
     * @brief Stop the worker thread.
     */
    void stop();
    
    /**
     * @brief Set the system prompt.
     */
    void set_system_prompt(const QString& prompt);
    
    /**
     * @brief Load system prompt from project directory.
     */
    void load_system_prompt(const QString& project_dir);

protected:
    void run() override;

private:
    /**
     * @brief Callback implementation that emits Qt signals.
     */
    class WorkerCallback : public ChatCallback {
    public:
        explicit WorkerCallback(AgentSignals& sigs) : agent_sigs_(sigs) {}
        
        void on_turn_start(int turn, int max_turns) override;
        void on_thinking() override;
        void on_thinking_done() override;
        void on_tool_use(const std::string& tool_name, const std::string& details) override;
        void on_text(const std::string& text) override;
        void on_script_code(const std::string& code) override;
        void on_script_output(const std::string& output) override;
        void on_error(const std::string& error) override;
        void on_result(int num_turns, std::optional<double> cost) override;
        
    private:
        AgentSignals& agent_sigs_;
    };
    
    void process_command(WorkerCommand cmd, const QString& data);
    
    AgentSignals agent_signals_;
    WorkerCallback callback_;
    ScriptExecutorFn script_executor_;
    MessageHistory* history_;
    std::unique_ptr<ChatCore> core_;
    
    // Thread synchronization (use std:: instead of Qt to avoid version compatibility issues)
    std::mutex mutex_;
    std::condition_variable condition_;
    std::queue<std::pair<WorkerCommand, QString>> command_queue_;
    std::atomic<bool> running_{false};
    std::atomic<ChatState> state_{ChatState::Disconnected};
    
    // Configuration
    AuthCredentials pending_credentials_;
    QString system_prompt_;
    QString project_dir_;
};

} // namespace ida_chat
