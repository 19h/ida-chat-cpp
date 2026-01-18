/**
 * @file cursor_chat_view.hpp
 * @brief Cursor-style chat conversation view with thinking indicators and file blocks.
 */

#pragma once

#include <QWidget>
#include <QScrollArea>
#include <QVBoxLayout>
#include <QLabel>
#include <QTimer>
#include <QString>
#include <QDateTime>
#include <vector>
#include <memory>

namespace ida_chat {

// ============================================================================
// Message Types
// ============================================================================

enum class CursorMessageType {
    User,           // User input message
    Thinking,       // "Thought Xs" indicator
    ToolAction,     // "Searched", "Read", "Reviewed" etc
    Text,           // Assistant text response
    FileBlock,      // File change block with diff stats
    CodeBlock,      // Code/script block
    Output,         // Script output
    Error,          // Error message
    Summary         // Summary with bullet points
};

// ============================================================================
// Tool Action Types
// ============================================================================

enum class ToolActionType {
    Thought,        // Thinking indicator with duration
    Searched,       // Web/code search
    Read,           // File read
    Reviewed,       // Code review
    Wrote,          // File write
    Ran,            // Command execution
    Custom          // Custom action
};

// ============================================================================
// File Block Data
// ============================================================================

struct FileBlockData {
    QString filename;
    int lines_added = 0;
    int lines_removed = 0;
    bool is_new = false;
    QString language;  // For syntax highlighting hints
};

// ============================================================================
// Message Data
// ============================================================================

struct CursorMessageData {
    CursorMessageType type;
    QString content;
    QDateTime timestamp;
    
    // For tool actions
    ToolActionType tool_type = ToolActionType::Custom;
    QString tool_detail;
    int duration_seconds = 0;
    
    // For file blocks
    FileBlockData file_data;
    
    // For code blocks
    QString code;
    QString code_output;
    bool code_error = false;
};

// ============================================================================
// User Message Widget
// ============================================================================

class UserMessageWidget : public QWidget {
    Q_OBJECT
public:
    explicit UserMessageWidget(const QString& text, QWidget* parent = nullptr);
};

// ============================================================================
// Thinking Indicator Widget
// ============================================================================

class ThinkingIndicator : public QWidget {
    Q_OBJECT
public:
    explicit ThinkingIndicator(QWidget* parent = nullptr);
    
    void start();
    void stop(int duration_seconds);
    bool is_active() const { return active_; }
    
private:
    QLabel* icon_label_;
    QLabel* text_label_;
    QTimer* timer_;
    QDateTime start_time_;
    bool active_ = false;
    int frame_ = 0;
};

// ============================================================================
// Tool Action Widget (Searched, Read, etc)
// ============================================================================

class ToolActionWidget : public QWidget {
    Q_OBJECT
public:
    ToolActionWidget(ToolActionType type, const QString& detail, QWidget* parent = nullptr);
    
private:
    QString action_text(ToolActionType type) const;
};

// ============================================================================
// File Block Widget
// ============================================================================

class FileBlockWidget : public QWidget {
    Q_OBJECT
public:
    explicit FileBlockWidget(const FileBlockData& data, QWidget* parent = nullptr);
    
signals:
    void clicked(const QString& filename);
    
protected:
    void mousePressEvent(QMouseEvent* event) override;
    void enterEvent(QEnterEvent* event) override;
    void leaveEvent(QEvent* event) override;
    
private:
    FileBlockData data_;
    bool hovered_ = false;
};

// ============================================================================
// Code Block Widget
// ============================================================================

class CodeBlockWidget : public QWidget {
    Q_OBJECT
public:
    CodeBlockWidget(const QString& code, const QString& language = "", QWidget* parent = nullptr);
    
    void set_output(const QString& output, bool is_error = false);
    
private:
    QLabel* code_label_;
    QWidget* output_widget_;
    QLabel* output_label_;
};

// ============================================================================
// Assistant Response Widget (contains multiple elements)
// ============================================================================

class AssistantResponseWidget : public QWidget {
    Q_OBJECT
public:
    explicit AssistantResponseWidget(QWidget* parent = nullptr);
    
    void add_thinking(int duration_seconds = 0);
    void add_tool_action(ToolActionType type, const QString& detail);
    void add_text(const QString& text);
    void add_file_block(const FileBlockData& data);
    void add_code_block(const QString& code, const QString& language = "");
    void add_output(const QString& output, bool is_error = false);
    void add_summary(const QStringList& points);
    
    ThinkingIndicator* thinking_indicator() { return thinking_; }
    
private:
    QVBoxLayout* layout_;
    ThinkingIndicator* thinking_ = nullptr;
    CodeBlockWidget* last_code_block_ = nullptr;
};

// ============================================================================
// Cursor Chat View (main conversation container)
// ============================================================================

class CursorChatView : public QWidget {
    Q_OBJECT
    
public:
    explicit CursorChatView(QWidget* parent = nullptr);
    
    // Add messages
    void add_user_message(const QString& text);
    AssistantResponseWidget* start_assistant_response();
    void finish_assistant_response();
    
    // Current response helpers
    void show_thinking();
    void hide_thinking(int duration_seconds);
    void add_tool_action(ToolActionType type, const QString& detail);
    void add_assistant_text(const QString& text);
    void add_file_block(const FileBlockData& data);
    void add_code_block(const QString& code, const QString& language = "");
    void add_code_output(const QString& output, bool is_error = false);
    void add_summary(const QStringList& points);
    
    // Clear
    void clear();
    
    // Scroll to bottom
    void scroll_to_bottom();
    
signals:
    void file_clicked(const QString& filename);
    
private:
    void setup_ui();
    
    QScrollArea* scroll_area_;
    QWidget* content_widget_;
    QVBoxLayout* content_layout_;
    
    AssistantResponseWidget* current_response_ = nullptr;
};

} // namespace ida_chat
