/**
 * @file chat_history_widget.cpp
 * @brief Scrollable chat history container widget.
 */

#include <ida_chat/ui/chat_history_widget.hpp>
#include <ida_chat/ui/chat_message.hpp>
#include <ida_chat/ui/collapsible_section.hpp>
#include <ida_chat/ui/markdown_renderer.hpp>

#include <QVBoxLayout>
#include <QScrollBar>
#include <QTimer>

namespace ida_chat {

ChatHistoryWidget::ChatHistoryWidget(QWidget* parent)
    : QScrollArea(parent)
{
    setup_ui();
}

ChatHistoryWidget::~ChatHistoryWidget() = default;

ChatMessage* ChatHistoryWidget::add_message(const QString& text,
                                            bool is_user,
                                            bool is_processing,
                                            MessageType msg_type)
{
    ChatMessage* message = new ChatMessage(text, is_user, is_processing, msg_type, container_);
    layout_->addWidget(message);
    messages_.append(message);
    
    if (is_processing) {
        current_processing_ = message;
    }
    
    // Auto-scroll to bottom
    QTimer::singleShot(10, this, &ChatHistoryWidget::scroll_to_bottom);
    
    return message;
}

CollapsibleSection* ChatHistoryWidget::add_collapsible(const QString& title,
                                                       const QString& content,
                                                       bool collapsed)
{
    CollapsibleSection* section = new CollapsibleSection(title, content, collapsed, container_);
    layout_->addWidget(section);
    
    // Connect to resize signal for auto-scroll
    connect(section, &QFrame::customContextMenuRequested, this, &ChatHistoryWidget::on_content_changed);
    
    // Auto-scroll to bottom
    QTimer::singleShot(10, this, &ChatHistoryWidget::scroll_to_bottom);
    
    return section;
}

void ChatHistoryWidget::mark_current_complete() {
    if (current_processing_) {
        current_processing_->set_complete();
        current_processing_ = nullptr;
    }
}

void ChatHistoryWidget::scroll_to_bottom() {
    QScrollBar* vbar = verticalScrollBar();
    if (vbar) {
        vbar->setValue(vbar->maximum());
    }
}

void ChatHistoryWidget::clear_history() {
    // Remove all messages
    for (ChatMessage* msg : messages_) {
        layout_->removeWidget(msg);
        delete msg;
    }
    messages_.clear();
    current_processing_ = nullptr;
}

int ChatHistoryWidget::message_count() const {
    return messages_.size();
}

ChatMessage* ChatHistoryWidget::message_at(int index) const {
    if (index >= 0 && index < messages_.size()) {
        return messages_.at(index);
    }
    return nullptr;
}

ChatMessage* ChatHistoryWidget::last_message() const {
    if (messages_.isEmpty()) {
        return nullptr;
    }
    return messages_.last();
}

void ChatHistoryWidget::on_content_changed() {
    // Re-scroll when content changes
    QTimer::singleShot(10, this, &ChatHistoryWidget::scroll_to_bottom);
}

void ChatHistoryWidget::setup_ui() {
    ColorScheme colors = ColorScheme::from_ida_palette();
    
    // Configure scroll area
    setWidgetResizable(true);
    setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    setFrameShape(QFrame::NoFrame);
    
    // Create container widget
    container_ = new QWidget(this);
    container_->setStyleSheet(QString("background-color: %1;").arg(colors.window.name()));
    
    // Create layout
    layout_ = new QVBoxLayout(container_);
    layout_->setContentsMargins(0, 8, 0, 8);
    layout_->setSpacing(4);
    layout_->addStretch();  // Push messages to bottom initially
    
    setWidget(container_);
}

} // namespace ida_chat
