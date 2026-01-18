/**
 * @file chat_input_widget.cpp
 * @brief Multi-line text input widget with history navigation.
 */

#include <ida_chat/ui/chat_input_widget.hpp>
#include <ida_chat/ui/markdown_renderer.hpp>

#include <QKeyEvent>

namespace ida_chat {

ChatInputWidget::ChatInputWidget(QWidget* parent)
    : QPlainTextEdit(parent)
{
    setup_ui();
}

ChatInputWidget::~ChatInputWidget() = default;

void ChatInputWidget::set_history(const QStringList& messages) {
    history_ = messages;
    history_index_ = -1;
}

void ChatInputWidget::add_to_history(const QString& message) {
    // Don't add duplicates
    if (!history_.isEmpty() && history_.last() == message) {
        return;
    }
    history_.append(message);
    history_index_ = -1;
}

void ChatInputWidget::clear_history() {
    history_.clear();
    history_index_ = -1;
}

void ChatInputWidget::set_input_enabled(bool enabled) {
    setEnabled(enabled);
    if (enabled) {
        setPlaceholderText("Send a message...");
        setFocus();
    } else {
        setPlaceholderText("Processing...");
    }
}

void ChatInputWidget::setup_ui() {
    ColorScheme colors = ColorScheme::from_ida_palette();
    
    setPlaceholderText("Send a message...");
    setMaximumHeight(150);
    setMinimumHeight(40);
    
    // Style
    setStyleSheet(QString(
        "QPlainTextEdit {"
        "  background-color: %1;"
        "  color: %2;"
        "  border: 1px solid %3;"
        "  border-radius: 8px;"
        "  padding: 8px;"
        "  font-size: 13px;"
        "}"
        "QPlainTextEdit:focus {"
        "  border-color: %4;"
        "}"
    ).arg(colors.base.name())
     .arg(colors.text.name())
     .arg(colors.mid.name())
     .arg(colors.highlight.name()));
    
    // Dynamic height based on content
    connect(this, &QPlainTextEdit::textChanged, this, [this]() {
        QFontMetrics fm(font());
        int lineHeight = fm.lineSpacing();
        int lines = qMax(1, document()->blockCount());
        int newHeight = qMin(150, qMax(40, lines * lineHeight + 20));
        setFixedHeight(newHeight);
    });
}

void ChatInputWidget::keyPressEvent(QKeyEvent* event) {
    // Enter to send (without shift)
    if (event->key() == Qt::Key_Return || event->key() == Qt::Key_Enter) {
        if (!(event->modifiers() & Qt::ShiftModifier)) {
            QString text = toPlainText().trimmed();
            if (!text.isEmpty()) {
                add_to_history(text);
                clear();
                emit message_submitted(text);
            }
            return;
        }
    }
    
    // Escape to cancel
    if (event->key() == Qt::Key_Escape) {
        emit cancel_requested();
        return;
    }
    
    // Up/Down arrow for history navigation
    if (event->key() == Qt::Key_Up && 
        (textCursor().blockNumber() == 0 || toPlainText().isEmpty())) {
        navigate_history(-1);
        return;
    }
    
    if (event->key() == Qt::Key_Down &&
        (textCursor().blockNumber() == document()->blockCount() - 1 || toPlainText().isEmpty())) {
        navigate_history(1);
        return;
    }
    
    QPlainTextEdit::keyPressEvent(event);
}

void ChatInputWidget::navigate_history(int direction) {
    if (history_.isEmpty()) return;
    
    // Save current input when starting to navigate
    if (history_index_ == -1) {
        current_input_ = toPlainText();
    }
    
    int new_index = history_index_ + direction;
    
    if (direction < 0) {
        // Going back in history (up arrow)
        if (new_index < -1) {
            new_index = history_.size() - 1;
        } else if (history_index_ == -1) {
            new_index = history_.size() - 1;
        }
    } else {
        // Going forward in history (down arrow)
        if (new_index >= history_.size()) {
            new_index = -1;
        }
    }
    
    history_index_ = new_index;
    
    if (history_index_ == -1) {
        // Back to current input
        setPlainText(current_input_);
    } else {
        setPlainText(history_.at(history_index_));
    }
    
    // Move cursor to end
    QTextCursor cursor = textCursor();
    cursor.movePosition(QTextCursor::End);
    setTextCursor(cursor);
}

} // namespace ida_chat
