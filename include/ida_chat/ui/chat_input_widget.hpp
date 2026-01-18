/**
 * @file chat_input_widget.hpp
 * @brief Multi-line text input widget with history navigation.
 */

#pragma once

#include <QPlainTextEdit>
#include <QString>
#include <QStringList>

namespace ida_chat {

/**
 * @brief Multi-line text input with Enter to send and history navigation.
 * 
 * Features:
 * - Enter sends the message
 * - Shift+Enter inserts a newline
 * - Up/Down arrow keys navigate input history
 * - Escape cancels current operation
 */
class ChatInputWidget : public QPlainTextEdit {
    Q_OBJECT

public:
    explicit ChatInputWidget(QWidget* parent = nullptr);
    ~ChatInputWidget() override;
    
    /**
     * @brief Set the input history.
     */
    void set_history(const QStringList& messages);
    
    /**
     * @brief Add a message to the history.
     */
    void add_to_history(const QString& message);
    
    /**
     * @brief Clear the history.
     */
    void clear_history();
    
    /**
     * @brief Get the current history.
     */
    [[nodiscard]] QStringList get_history() const { return history_; }
    
    /**
     * @brief Set whether the widget is enabled.
     * 
     * When disabled, shows a placeholder indicating processing.
     */
    void set_input_enabled(bool enabled);

signals:
    /**
     * @brief Emitted when user presses Enter to send a message.
     */
    void message_submitted(const QString& message);
    
    /**
     * @brief Emitted when user presses Escape.
     */
    void cancel_requested();

protected:
    void keyPressEvent(QKeyEvent* event) override;

private:
    void setup_ui();
    void navigate_history(int direction);
    
    QStringList history_;
    int history_index_ = -1;
    QString current_input_;  // Saved input when navigating history
};

} // namespace ida_chat
