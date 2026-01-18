/**
 * @file ida_chat_form.cpp
 * @brief Main dockable chat widget form for IDA Pro.
 * 
 * Cursor-style agent UI with sidebar task list and conversation view.
 */

// Qt headers first to avoid symbol conflicts
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QPushButton>
#include <QLabel>
#include <QStackedWidget>
#include <QSplitter>
#include <QMessageBox>
#include <QFileDialog>
#include <QApplication>
#include <QClipboard>
#include <QDir>
#include <QFile>
#include <QElapsedTimer>

// Then IDA headers with our compatibility wrapper
#include <ida_chat/common/ida_begin.hpp>
#include <ida.hpp>
#include <kernwin.hpp>
#include <ida_chat/common/ida_end.hpp>

// Then our headers
#include <ida_chat/ui/ida_chat_form.hpp>
#include <ida_chat/ui/cursor_theme.hpp>
#include <ida_chat/core/script_executor.hpp>
#include <ida_chat/plugin/settings.hpp>

namespace ida_chat {

// ============================================================================
// Widget Title
// ============================================================================

static const char* WIDGET_TITLE = "IDA Chat";

const char* IDAChatForm::widget_title() {
    return WIDGET_TITLE;
}

// ============================================================================
// IDAChatForm Implementation
// ============================================================================

IDAChatForm::IDAChatForm() 
    : QObject(nullptr)
{}

IDAChatForm::~IDAChatForm() {
    if (worker_) {
        worker_->stop();
    }
}

void IDAChatForm::create_and_show() {
    // Create an empty IDA widget
    TWidget* tw = create_empty_widget(WIDGET_TITLE, -1);
    if (!tw) {
        msg("IDA Chat: Failed to create widget\n");
        return;
    }
    ida_widget_ = tw;
    
    // Display the widget (docked by default)
    display_widget(tw, WOPN_DP_RIGHT | WOPN_RESTORE);
    
    // Now initialize the UI - TWidget IS QWidget in IDA
    on_widget_visible(tw);
}

void IDAChatForm::on_widget_visible(IDAWidget* ida_widget) {
    // Get the QWidget* from the TWidget*
    // In IDA, TWidget IS QWidget, so we can just cast
    widget_ = reinterpret_cast<QWidget*>(ida_widget);
    if (!widget_) return;
    
    ida_widget_ = ida_widget;
    
    create_ui(widget_);
    init_agent();
    
    // Check if we need to show onboarding
    Settings settings;
    if (settings.show_wizard()) {
        show_onboarding();
    } else {
        // Auto-connect with saved settings
        AuthCredentials creds;
        creds.type = settings.auth_type();
        if (creds.requires_key()) {
            creds.api_key = settings.api_key().toStdString();
        }
        worker_->request_connect(creds);
    }
}

void IDAChatForm::on_widget_closing() {
    if (worker_) {
        worker_->stop();
    }
    widget_ = nullptr;
    ida_widget_ = nullptr;
}

bool IDAChatForm::is_visible() const {
    return widget_ && widget_->isVisible();
}

void IDAChatForm::show() {
    if (ida_widget_) {
        display_widget(reinterpret_cast<TWidget*>(ida_widget_), WOPN_RESTORE);
    } else {
        create_and_show();
    }
}

void IDAChatForm::hide() {
    if (ida_widget_) {
        close_widget(reinterpret_cast<TWidget*>(ida_widget_), WCLS_SAVE);
    }
}

void IDAChatForm::create_ui(QWidget* parent) {
    using namespace theme;
    
    // Main layout
    main_layout_ = new QVBoxLayout(parent);
    main_layout_->setContentsMargins(0, 0, 0, 0);
    main_layout_->setSpacing(0);
    
    // Apply dark theme to parent
    parent->setStyleSheet(QString(
        "QWidget { background-color: %1; color: %2; }"
        "QScrollBar:vertical { background: %1; width: 8px; border: none; }"
        "QScrollBar::handle:vertical { background: %3; border-radius: 4px; min-height: 20px; }"
        "QScrollBar::add-line:vertical, QScrollBar::sub-line:vertical { height: 0; }"
        "QScrollBar:horizontal { background: %1; height: 8px; border: none; }"
        "QScrollBar::handle:horizontal { background: %3; border-radius: 4px; min-width: 20px; }"
        "QScrollBar::add-line:horizontal, QScrollBar::sub-line:horizontal { width: 0; }"
    ).arg(BG_BASE, TEXT_PRIMARY, BORDER_DEFAULT));
    
    // Create stacked widget for switching between onboarding and main view
    stack_ = new QStackedWidget(parent);
    
    // Create main view (sidebar + chat)
    create_main_view();
    stack_->addWidget(main_view_);
    
    // Create onboarding panel
    onboarding_ = new OnboardingPanel(stack_);
    connect(onboarding_, &OnboardingPanel::onboarding_complete,
            this, &IDAChatForm::on_onboarding_complete);
    stack_->addWidget(onboarding_);
    
    main_layout_->addWidget(stack_, 1);
    
    // Default to main view
    stack_->setCurrentWidget(main_view_);
}

void IDAChatForm::create_main_view() {
    using namespace theme;
    
    main_view_ = new QWidget(stack_);
    QHBoxLayout* layout = new QHBoxLayout(main_view_);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(0);
    
    // Create splitter for sidebar and chat
    splitter_ = new QSplitter(Qt::Horizontal, main_view_);
    splitter_->setHandleWidth(1);
    splitter_->setChildrenCollapsible(false);
    splitter_->setStyleSheet(QString(
        "QSplitter::handle { background: %1; }"
    ).arg(BORDER_SUBTLE));
    
    // === LEFT: Task Sidebar ===
    sidebar_ = new TaskSidebar(splitter_);
    connect(sidebar_, &TaskSidebar::task_selected,
            this, &IDAChatForm::update_for_task);
    splitter_->addWidget(sidebar_);
    
    // === RIGHT: Chat Container ===
    chat_container_ = new QWidget(splitter_);
    chat_container_->setStyleSheet(QString("background-color: %1;").arg(BG_BASE));
    
    QVBoxLayout* chat_layout = new QVBoxLayout(chat_container_);
    chat_layout->setContentsMargins(0, 0, 0, 0);
    chat_layout->setSpacing(0);
    
    // Chat view (scrollable conversation)
    chat_view_ = new CursorChatView(chat_container_);
    chat_layout->addWidget(chat_view_, 1);
    
    // Input widget
    input_ = new CursorInputWidget(chat_container_);
    input_->set_placeholder("Plan, search, build anything...");
    connect(input_, &CursorInputWidget::message_submitted,
            this, &IDAChatForm::on_message_submitted);
    connect(input_, &CursorInputWidget::cancel_requested,
            this, &IDAChatForm::on_cancel);
    chat_layout->addWidget(input_);
    
    splitter_->addWidget(chat_container_);
    
    // Set initial splitter sizes (sidebar: 240px, chat: rest)
    splitter_->setSizes({240, 560});
    splitter_->setStretchFactor(0, 0);  // Sidebar doesn't stretch
    splitter_->setStretchFactor(1, 1);  // Chat stretches
    
    layout->addWidget(splitter_);
}

void IDAChatForm::create_status_bar() {
    // Status bar removed in Cursor-style UI
    // Status info is shown in sidebar task cards
}

void IDAChatForm::init_agent() {
    // Create script executor
    auto executor = create_script_executor();
    
    // Create message history
    // TODO: Get the actual binary path from IDA when available
    std::string binary_path = "unknown_binary";
    message_history_ = std::make_unique<MessageHistory>(binary_path);
    
    // Create worker
    worker_ = std::make_unique<AgentWorker>(executor, message_history_.get());
    
    // Connect worker signals (using 'sigs' to avoid conflict with Qt's 'signals' macro)
    AgentSignals* sigs = worker_->get_signals();
    
    connect(sigs, &AgentSignals::connection_ready,
            this, &IDAChatForm::on_connection_ready);
    connect(sigs, &AgentSignals::connection_error,
            this, &IDAChatForm::on_connection_error);
    connect(sigs, &AgentSignals::turn_start,
            this, &IDAChatForm::on_turn_start);
    connect(sigs, &AgentSignals::thinking,
            this, &IDAChatForm::on_thinking);
    connect(sigs, &AgentSignals::thinking_done,
            this, &IDAChatForm::on_thinking_done);
    connect(sigs, &AgentSignals::tool_use,
            this, &IDAChatForm::on_tool_use);
    connect(sigs, &AgentSignals::text,
            this, &IDAChatForm::on_text);
    connect(sigs, &AgentSignals::script_code,
            this, &IDAChatForm::on_script_code);
    connect(sigs, &AgentSignals::script_output,
            this, &IDAChatForm::on_script_output);
    connect(sigs, &AgentSignals::error,
            this, &IDAChatForm::on_error);
    connect(sigs, &AgentSignals::result,
            this, &IDAChatForm::on_result);
    connect(sigs, &AgentSignals::finished,
            this, &IDAChatForm::on_finished);
    
    // Load system prompt from project directory
    // Try multiple locations for the project files
    QStringList search_paths = {
        // Installed location (via CMake install)
        QApplication::applicationDirPath() + "/plugins/ida_chat_project",
        // User's idapro directory
        QDir::homePath() + "/.idapro/plugins/ida_chat_project",
        // Development location (source tree)
        "/Users/int/dev/ida-conv/ida-chat-cpp/project",
        // Python plugin's project dir (same content)
        "/Users/int/dev/ida-conv/ida-chat-plugin/project",
    };
    
    QString project_dir;
    for (const QString& path : search_paths) {
        msg("[IDA Chat] init_agent: checking path '%s'\n", path.toUtf8().constData());
        if (QDir(path).exists() && QFile::exists(path + "/PROMPT.md")) {
            project_dir = path;
            msg("[IDA Chat] init_agent: found project dir at '%s'\n", project_dir.toUtf8().constData());
            break;
        }
    }
    
    if (!project_dir.isEmpty()) {
        worker_->load_system_prompt(project_dir);
    } else {
        msg("[IDA Chat] init_agent: WARNING - no project directory found!\n");
    }
    
    // Start the worker thread
    worker_->start();
}

ScriptExecutorFn IDAChatForm::create_script_executor() {
    return create_main_thread_executor();
}

// ============================================================================
// Event Handlers
// ============================================================================

void IDAChatForm::on_connection_ready() {
    input_->set_enabled(true);
    
    // Add welcome message as first assistant response
    chat_view_->start_assistant_response();
    chat_view_->add_assistant_text(
        "Welcome to IDA Chat! I'm an AI assistant specialized in reverse engineering.\n\n"
        "I can help you with:\n"
        "- Analyzing functions and code\n"
        "- Understanding data structures\n"
        "- Writing IDAPython scripts\n"
        "- Renaming variables and functions\n\n"
        "How can I help you today?"
    );
    chat_view_->finish_assistant_response();
}

void IDAChatForm::on_connection_error(const QString& error) {
    chat_view_->start_assistant_response();
    chat_view_->add_assistant_text("Connection error: " + error);
    chat_view_->finish_assistant_response();
    
    input_->set_enabled(false);
    
    // Show onboarding panel for reconfiguration
    show_onboarding();
}

void IDAChatForm::on_turn_start(int turn, int max_turns) {
    // Could update sidebar task with turn info
    Q_UNUSED(turn);
    Q_UNUSED(max_turns);
}

void IDAChatForm::on_thinking() {
    // Record start time for duration tracking
    thinking_start_time_ = QDateTime::currentMSecsSinceEpoch();
    
    // Show thinking indicator in chat view
    chat_view_->show_thinking();
}

void IDAChatForm::on_thinking_done() {
    // Calculate duration
    int duration_ms = QDateTime::currentMSecsSinceEpoch() - thinking_start_time_;
    int duration_seconds = qMax(1, duration_ms / 1000);
    
    // Update thinking indicator with duration
    chat_view_->hide_thinking(duration_seconds);
}

void IDAChatForm::on_tool_use(const QString& tool_name, const QString& details) {
    // Map tool names to ToolActionType
    ToolActionType type = ToolActionType::Custom;
    
    if (tool_name.contains("search", Qt::CaseInsensitive) ||
        tool_name.contains("grep", Qt::CaseInsensitive) ||
        tool_name.contains("glob", Qt::CaseInsensitive)) {
        type = ToolActionType::Searched;
    } else if (tool_name.contains("read", Qt::CaseInsensitive)) {
        type = ToolActionType::Read;
    } else if (tool_name.contains("write", Qt::CaseInsensitive) ||
               tool_name.contains("edit", Qt::CaseInsensitive)) {
        type = ToolActionType::Wrote;
    } else if (tool_name.contains("bash", Qt::CaseInsensitive) ||
               tool_name.contains("run", Qt::CaseInsensitive)) {
        type = ToolActionType::Ran;
    }
    
    QString detail = details.isEmpty() ? tool_name : details;
    chat_view_->add_tool_action(type, detail);
}

void IDAChatForm::on_text(const QString& text) {
    chat_view_->add_assistant_text(text);
}

void IDAChatForm::on_script_code(const QString& code) {
    chat_view_->add_code_block(code, "python");
}

void IDAChatForm::on_script_output(const QString& output) {
    chat_view_->add_code_output(output, false);
}

void IDAChatForm::on_error(const QString& error) {
    chat_view_->add_code_output(error, true);
    
    // Mark current task as error
    if (!current_task_id_.isEmpty()) {
        sidebar_->error_task(current_task_id_, error);
    }
}

void IDAChatForm::on_result(int num_turns, double cost) {
    // Update task in sidebar with result info
    if (!current_task_id_.isEmpty()) {
        sidebar_->update_task_cost(current_task_id_, cost, num_turns);
    }
}

void IDAChatForm::on_finished() {
    processing_ = false;
    input_->set_enabled(true);
    chat_view_->finish_assistant_response();
    
    // Complete the task in sidebar
    if (!current_task_id_.isEmpty()) {
        sidebar_->complete_task(current_task_id_);
    }
}

// ============================================================================
// UI Actions
// ============================================================================

void IDAChatForm::on_message_submitted(const QString& text) {
    if (text.isEmpty() || processing_) return;
    
    processing_ = true;
    thinking_start_time_ = 0;
    
    // Create a new task in sidebar
    QString task_title = text;
    if (task_title.length() > 40) {
        task_title = task_title.left(37) + "...";
    }
    current_task_id_ = sidebar_->add_task(task_title);
    
    // Add user message to chat view
    chat_view_->add_user_message(text);
    
    // Start assistant response container
    chat_view_->start_assistant_response();
    
    // Disable input while processing
    input_->set_enabled(false);
    input_->clear();
    
    // Send to worker
    worker_->send_message(text);
}

void IDAChatForm::on_cancel() {
    if (processing_) {
        worker_->request_cancel();
        
        // Update UI
        chat_view_->add_assistant_text("(Cancelled)");
        chat_view_->finish_assistant_response();
        
        if (!current_task_id_.isEmpty()) {
            sidebar_->error_task(current_task_id_, "Cancelled by user");
        }
        
        processing_ = false;
        input_->set_enabled(true);
    }
}

void IDAChatForm::on_share() {
    // Copy conversation to clipboard
    // TODO: Implement proper conversation export
    QString text = "IDA Chat Conversation\n";
    text += "=====================\n\n";
    text += "(Conversation export not yet implemented)";
    
    QApplication::clipboard()->setText(text);
}

void IDAChatForm::on_clear() {
    // Confirm clear
    QMessageBox::StandardButton reply = QMessageBox::question(
        widget_,
        "Clear Conversation",
        "Clear the conversation and start a new session?",
        QMessageBox::Yes | QMessageBox::No
    );
    
    if (reply == QMessageBox::Yes) {
        chat_view_->clear();
        worker_->request_new_session();
        session_usage_ = TokenUsage{};
        current_task_id_.clear();
        
        // Re-add welcome message
        on_connection_ready();
    }
}

void IDAChatForm::on_settings() {
    show_onboarding();
}

void IDAChatForm::on_onboarding_complete() {
    stack_->setCurrentWidget(main_view_);
    
    // Reconnect with new settings
    AuthCredentials creds = onboarding_->get_credentials();
    worker_->request_connect(creds);
}

// ============================================================================
// Helper Methods
// ============================================================================

void IDAChatForm::show_onboarding() {
    onboarding_->load_current_settings();
    stack_->setCurrentWidget(onboarding_);
}

void IDAChatForm::update_for_task(const QString& task_id) {
    // Called when user clicks a task in sidebar
    // Could scroll to that task's messages in chat view
    // For now, just update the current task
    current_task_id_ = task_id;
}

} // namespace ida_chat
