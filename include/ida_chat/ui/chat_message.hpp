/**
 * @file chat_message.hpp
 * @brief Single chat message bubble widget.
 */

#pragma once

#include <ida_chat/core/types.hpp>

#include <QFrame>
#include <QString>
#include <QTimer>
#include <QLabel>
#include <QVBoxLayout>

namespace ida_chat {

/**
 * @brief A single chat message bubble with optional status indicator.
 */
class ChatMessage : public QFrame {
    Q_OBJECT

public:
    /**
     * @brief Create a chat message widget.
     * @param text Initial message text
     * @param is_user Whether this is a user message
     * @param is_processing Whether message is being processed
     * @param msg_type Message type for styling
     * @param parent Parent widget
     */
    ChatMessage(const QString& text,
                bool is_user = true,
                bool is_processing = false,
                MessageType msg_type = MessageType::Text,
                QWidget* parent = nullptr);
    
    ~ChatMessage() override;
    
    /**
     * @brief Update the message text.
     */
    void update_text(const QString& text);
    
    /**
     * @brief Append text to the message.
     */
    void append_text(const QString& text);
    
    /**
     * @brief Mark message as complete (stop any animations).
     */
    void set_complete();
    
    /**
     * @brief Check if this is a user message.
     */
    [[nodiscard]] bool is_user_message() const noexcept { return is_user_; }
    
    /**
     * @brief Get the message type.
     */
    [[nodiscard]] MessageType message_type() const noexcept { return msg_type_; }
    
    /**
     * @brief Get the current text.
     */
    [[nodiscard]] QString text() const;

private slots:
    void toggle_blink();

private:
    void setup_ui(const QString& text);
    void update_indicator_style();
    void start_blinking();
    void stop_blinking();
    
    bool is_user_;
    bool is_processing_;
    MessageType msg_type_;
    
    QLabel* label_ = nullptr;
    QLabel* indicator_ = nullptr;
    QVBoxLayout* layout_ = nullptr;
    QTimer* blink_timer_ = nullptr;
    bool blink_state_ = false;
};

} // namespace ida_chat
