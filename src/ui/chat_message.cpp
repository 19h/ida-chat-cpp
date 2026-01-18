/**
 * @file chat_message.cpp
 * @brief Single chat message bubble widget.
 */

#include <ida_chat/ui/chat_message.hpp>
#include <ida_chat/ui/markdown_renderer.hpp>

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QTimer>

namespace ida_chat {

ChatMessage::ChatMessage(const QString& text,
                         bool is_user,
                         bool is_processing,
                         MessageType msg_type,
                         QWidget* parent)
    : QFrame(parent)
    , is_user_(is_user)
    , is_processing_(is_processing)
    , msg_type_(is_user ? MessageType::User : msg_type)
{
    setup_ui(text);
}

ChatMessage::~ChatMessage() {
    stop_blinking();
}

void ChatMessage::update_text(const QString& text) {
    if (!label_) return;
    
    if (is_user_) {
        label_->setText(text);
    } else if (msg_type_ == MessageType::ToolUse) {
        label_->setText(QString("<i>%1</i>").arg(MarkdownRenderer::escape_html(text)));
    } else if (msg_type_ == MessageType::Script || msg_type_ == MessageType::Output) {
        label_->setText(QString("<pre style='margin: 0; white-space: pre-wrap; word-wrap: break-word;'>%1</pre>")
            .arg(MarkdownRenderer::escape_html(text)));
    } else {
        label_->setText(markdown_to_html(text));
    }
}

void ChatMessage::append_text(const QString& text) {
    if (!label_) return;
    
    QString current = this->text();
    update_text(current + text);
}

QString ChatMessage::text() const {
    if (!label_) return {};
    
    // For rich text labels, we'd need to strip HTML
    // For simplicity, we'll store original text separately in a real implementation
    return label_->text();
}

void ChatMessage::set_complete() {
    is_processing_ = false;
    stop_blinking();
    update_indicator_style();
}

void ChatMessage::setup_ui(const QString& text) {
    ColorScheme colors = ColorScheme::from_ida_palette();
    
    QHBoxLayout* main_layout = new QHBoxLayout(this);
    main_layout->setContentsMargins(8, 4, 8, 4);
    
    if (is_user_) {
        // User message - right aligned, accent color background
        label_ = new QLabel(text, this);
        label_->setWordWrap(true);
        label_->setTextInteractionFlags(Qt::TextSelectableByMouse | Qt::TextSelectableByKeyboard);
        label_->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Minimum);
        
        main_layout->addStretch();
        label_->setStyleSheet(QString(
            "QLabel {"
            "  background-color: %1;"
            "  color: %2;"
            "  border-radius: 10px;"
            "  padding: 8px 12px;"
            "}"
        ).arg(colors.highlight.name()).arg(colors.highlight_text.name()));
        
        main_layout->addWidget(label_);
    } else {
        // Status indicator for assistant messages
        indicator_ = new QLabel(QString::fromUtf8("\u25CF"), this);  // ●
        indicator_->setFixedWidth(16);
        indicator_->setAlignment(Qt::AlignCenter | Qt::AlignTop);
        update_indicator_style();
        main_layout->addWidget(indicator_);
        
        // Assistant message - QLabel with rich text
        label_ = new QLabel(this);
        label_->setTextFormat(Qt::RichText);
        label_->setWordWrap(true);
        label_->setTextInteractionFlags(
            Qt::TextSelectableByMouse | Qt::TextSelectableByKeyboard | Qt::LinksAccessibleByMouse);
        label_->setOpenExternalLinks(true);
        label_->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Minimum);
        
        // Apply type-specific styling
        QString style;
        QString content;
        
        switch (msg_type_) {
            case MessageType::ToolUse:
                content = QString("<i>%1</i>").arg(MarkdownRenderer::escape_html(text));
                style = QString(
                    "QLabel {"
                    "  background-color: transparent;"
                    "  color: %1;"
                    "  padding: 4px 8px;"
                    "  font-size: 11px;"
                    "}"
                ).arg(colors.mid.name());
                break;
                
            case MessageType::Script:
                content = QString("<pre style='margin: 0; white-space: pre-wrap; word-wrap: break-word;'>%1</pre>")
                    .arg(MarkdownRenderer::escape_html(text));
                style = QString(
                    "QLabel {"
                    "  background-color: #1e1e1e;"
                    "  color: #d4d4d4;"
                    "  border-radius: 6px;"
                    "  padding: 8px 12px;"
                    "  font-family: monospace;"
                    "  font-size: 11px;"
                    "}"
                );
                break;
                
            case MessageType::Output:
                content = QString("<pre style='margin: 0; white-space: pre-wrap; word-wrap: break-word;'>%1</pre>")
                    .arg(MarkdownRenderer::escape_html(text));
                style = QString(
                    "QLabel {"
                    "  background-color: #2d2d2d;"
                    "  color: #a0a0a0;"
                    "  border-radius: 6px;"
                    "  padding: 8px 12px;"
                    "  font-family: monospace;"
                    "  font-size: 11px;"
                    "}"
                );
                break;
                
            case MessageType::Error:
                content = markdown_to_html(text);
                style = QString(
                    "QLabel {"
                    "  background-color: #2d1f1f;"
                    "  color: #f87171;"
                    "  border: 1px solid #dc2626;"
                    "  border-radius: 10px;"
                    "  padding: 8px 12px;"
                    "}"
                );
                break;
                
            default:
                content = markdown_to_html(text);
                style = QString(
                    "QLabel {"
                    "  background-color: %1;"
                    "  color: %2;"
                    "  border-radius: 10px;"
                    "  padding: 8px 12px;"
                    "}"
                ).arg(colors.alternate_base.name()).arg(colors.text.name());
                break;
        }
        
        label_->setText(content);
        label_->setStyleSheet(style);
        
        main_layout->addWidget(label_, 4);
        main_layout->addStretch(1);  // 4:1 ratio = ~80% for message
        
        // Start blinking if processing
        if (is_processing_) {
            start_blinking();
        }
    }
}

void ChatMessage::update_indicator_style() {
    if (!indicator_) return;
    
    QString color;
    if (is_processing_) {
        color = blink_state_ ? "#f59e0b" : "transparent";
    } else {
        color = "#22c55e";  // Green for complete
    }
    
    indicator_->setStyleSheet(QString("color: %1; font-size: 8px;").arg(color));
}

void ChatMessage::start_blinking() {
    if (blink_timer_) return;
    
    blink_timer_ = new QTimer(this);
    connect(blink_timer_, &QTimer::timeout, this, &ChatMessage::toggle_blink);
    blink_timer_->start(500);  // 500ms interval
}

void ChatMessage::stop_blinking() {
    if (blink_timer_) {
        blink_timer_->stop();
        delete blink_timer_;
        blink_timer_ = nullptr;
    }
    blink_state_ = false;
}

void ChatMessage::toggle_blink() {
    blink_state_ = !blink_state_;
    update_indicator_style();
}

} // namespace ida_chat
