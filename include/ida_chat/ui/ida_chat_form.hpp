/**
 * @file ida_chat_form.hpp
 * @brief Main dockable chat widget form for IDA Pro.
 * 
 * Cursor-style agent UI with sidebar task list and conversation view.
 */

#pragma once

// IMPORTANT: Qt headers MUST come before IDA headers to avoid symbol conflicts
// (both Qt and IDA define qstrlen, qstrncmp, qsnprintf, etc.)
#include <QWidget>
#include <QString>
#include <QObject>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QSplitter>
#include <QStackedWidget>
#include <memory>

#include <ida_chat/core/types.hpp>
#include <ida_chat/history/message_history.hpp>
#include <ida_chat/ui/task_sidebar.hpp>
#include <ida_chat/ui/cursor_chat_view.hpp>
#include <ida_chat/ui/cursor_input.hpp>
#include <ida_chat/ui/onboarding_panel.hpp>
#include <ida_chat/ui/agent_worker.hpp>

namespace ida_chat {

// Forward declaration for IDA's TWidget (which is actually QWidget*)
// We avoid including IDA headers here to prevent Qt/IDA symbol conflicts
using IDAWidget = void;

/**
 * @brief Main chat widget form for IDA Pro.
 * 
 * Uses IDA 9.x widget API (create_empty_widget/display_widget/close_widget).
 * This class manages a QWidget that is embedded into an IDA TWidget.
 */
class IDAChatForm : public QObject {
    Q_OBJECT

public:
    IDAChatForm();
    ~IDAChatForm() override;
    
    /**
     * @brief Create and show the form.
     * Called when the plugin wants to show the chat widget.
     */
    void create_and_show();
    
    /**
     * @brief Called when the IDA widget becomes visible.
     * @param ida_widget The IDA TWidget* cast to void*
     */
    void on_widget_visible(IDAWidget* ida_widget);
    
    /**
     * @brief Called when the IDA widget is about to close.
     */
    void on_widget_closing();
    
    /**
     * @brief Check if the form is currently showing.
     */
    [[nodiscard]] bool is_visible() const;
    
    /**
     * @brief Show the form.
     */
    void show();
    
    /**
     * @brief Hide the form.
     */
    void hide();
    
    /**
     * @brief Get the widget title.
     */
    static const char* widget_title();

private:
    // UI initialization
    void create_ui(QWidget* parent);
    void create_main_view();
    void create_status_bar();
    void init_agent();
    
    // Event handlers
    void on_connection_ready();
    void on_connection_error(const QString& error);
    void on_turn_start(int turn, int max_turns);
    void on_thinking();
    void on_thinking_done();
    void on_tool_use(const QString& tool_name, const QString& details);
    void on_text(const QString& text);
    void on_script_code(const QString& code);
    void on_script_output(const QString& output);
    void on_error(const QString& error);
    void on_result(int num_turns, double cost);
    void on_finished();
    
    // UI actions
    void on_message_submitted(const QString& text);
    void on_cancel();
    void on_share();
    void on_clear();
    void on_settings();
    void on_onboarding_complete();
    
    // Helper methods
    void show_onboarding();
    void update_for_task(const QString& task_id);
    ScriptExecutorFn create_script_executor();
    
    // IDA widget (TWidget* stored as void* to avoid IDA header in this file)
    IDAWidget* ida_widget_ = nullptr;
    
    // Main widget
    QWidget* widget_ = nullptr;
    QVBoxLayout* main_layout_ = nullptr;
    QStackedWidget* stack_ = nullptr;
    
    // Cursor-style main view (sidebar + chat + input)
    QWidget* main_view_ = nullptr;
    QSplitter* splitter_ = nullptr;
    TaskSidebar* sidebar_ = nullptr;
    QWidget* chat_container_ = nullptr;
    CursorChatView* chat_view_ = nullptr;
    CursorInputWidget* input_ = nullptr;
    
    // Onboarding
    OnboardingPanel* onboarding_ = nullptr;
    
    // Agent
    std::unique_ptr<MessageHistory> message_history_;
    std::unique_ptr<AgentWorker> worker_;
    
    // State
    QString current_task_id_;
    bool processing_ = false;
    int thinking_start_time_ = 0;
    TokenUsage session_usage_;
};

} // namespace ida_chat
