/**
 * @file chat_history_widget.hpp
 * @brief Scrollable chat history container widget.
 */

#pragma once

#include <ida_chat/core/types.hpp>
#include <ida_chat/ui/chat_message.hpp>
#include <ida_chat/ui/collapsible_section.hpp>

#include <QScrollArea>
#include <QString>
#include <QList>
#include <QVBoxLayout>
#include <QWidget>

namespace ida_chat {

/**
 * @brief Scrollable chat history container.
 * 
 * Manages a list of ChatMessage widgets and auto-scrolls to show
 * the latest message.
 */
class ChatHistoryWidget : public QScrollArea {
    Q_OBJECT

public:
    explicit ChatHistoryWidget(QWidget* parent = nullptr);
    ~ChatHistoryWidget() override;
    
    /**
     * @brief Add a new message to the history.
     * @param text Message text
     * @param is_user Whether it's a user message
     * @param is_processing Whether it's being processed
     * @param msg_type Message type for styling
     * @return Pointer to the created message widget
     */
    ChatMessage* add_message(const QString& text,
                             bool is_user = true,
                             bool is_processing = false,
                             MessageType msg_type = MessageType::Text);
    
    /**
     * @brief Add a collapsible section.
     * @param title Section title
     * @param content Section content
     * @param collapsed Whether to start collapsed
     * @return Pointer to the created section
     */
    CollapsibleSection* add_collapsible(const QString& title,
                                        const QString& content,
                                        bool collapsed = true);
    
    /**
     * @brief Mark the current (last) processing message as complete.
     */
    void mark_current_complete();
    
    /**
     * @brief Scroll to the bottom of the history.
     */
    void scroll_to_bottom();
    
    /**
     * @brief Clear all messages.
     */
    void clear_history();
    
    /**
     * @brief Get the number of messages.
     */
    [[nodiscard]] int message_count() const;
    
    /**
     * @brief Get a message by index.
     */
    [[nodiscard]] ChatMessage* message_at(int index) const;
    
    /**
     * @brief Get the last message.
     */
    [[nodiscard]] ChatMessage* last_message() const;

private slots:
    void on_content_changed();

private:
    void setup_ui();
    
    QWidget* container_ = nullptr;
    QVBoxLayout* layout_ = nullptr;
    QList<ChatMessage*> messages_;
    ChatMessage* current_processing_ = nullptr;
};

} // namespace ida_chat
