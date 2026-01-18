/**
 * @file cursor_chat_view.cpp
 * @brief Cursor-style chat conversation view implementation.
 */

#include <ida_chat/ui/cursor_chat_view.hpp>
#include <ida_chat/ui/cursor_theme.hpp>
#include <ida_chat/ui/markdown_renderer.hpp>

#include <QMouseEvent>
#include <QScrollBar>
#include <QApplication>
#include <QFrame>

namespace ida_chat {

using namespace theme;

// ============================================================================
// UserMessageWidget
// ============================================================================

UserMessageWidget::UserMessageWidget(const QString& text, QWidget* parent)
    : QWidget(parent)
{
    auto* layout = new QHBoxLayout(this);
    layout->setContentsMargins(20, 12, 20, 12);
    layout->addStretch();
    
    auto* bubble = new QWidget(this);
    bubble->setObjectName("UserBubble");
    bubble->setStyleSheet(QString(
        "#UserBubble { background: %1; border-radius: %2; }"
    ).arg(BG_USER_MESSAGE).arg(RADIUS_LG));
    
    auto* bubble_layout = new QVBoxLayout(bubble);
    bubble_layout->setContentsMargins(14, 12, 14, 12);
    
    auto* label = new QLabel(text, bubble);
    label->setWordWrap(true);
    label->setStyleSheet(QString(
        "color: %1; font-size: %2; line-height: 1.5; background: transparent;"
    ).arg(TEXT_PRIMARY).arg(FONT_BASE));
    label->setMaximumWidth(450);
    bubble_layout->addWidget(label);
    
    layout->addWidget(bubble);
}

// ============================================================================
// ThinkingIndicator
// ============================================================================

ThinkingIndicator::ThinkingIndicator(QWidget* parent)
    : QWidget(parent)
    , timer_(new QTimer(this))
{
    auto* layout = new QHBoxLayout(this);
    layout->setContentsMargins(0, 4, 0, 4);
    layout->setSpacing(8);
    
    icon_label_ = new QLabel("●", this);
    icon_label_->setStyleSheet(QString("color: %1; font-size: 8px;").arg(TEXT_MUTED));
    layout->addWidget(icon_label_);
    
    text_label_ = new QLabel("Thinking...", this);
    text_label_->setStyleSheet(QString("color: %1; font-size: %2;").arg(TEXT_MUTED).arg(FONT_SM));
    layout->addWidget(text_label_);
    
    layout->addStretch();
    
    connect(timer_, &QTimer::timeout, this, [this]() {
        frame_ = (frame_ + 1) % SPINNER_FRAMES.size();
        icon_label_->setText(SPINNER_FRAMES[frame_]);
        
        // Update elapsed time
        if (active_) {
            auto elapsed = start_time_.secsTo(QDateTime::currentDateTime());
            text_label_->setText(QString("Thought %1s").arg(elapsed));
        }
    });
}

void ThinkingIndicator::start() {
    active_ = true;
    start_time_ = QDateTime::currentDateTime();
    text_label_->setText("Thinking...");
    timer_->start(100);
    show();
}

void ThinkingIndicator::stop(int duration_seconds) {
    active_ = false;
    timer_->stop();
    icon_label_->setText("●");
    text_label_->setText(QString("Thought %1s").arg(duration_seconds));
}

// ============================================================================
// ToolActionWidget
// ============================================================================

ToolActionWidget::ToolActionWidget(ToolActionType type, const QString& detail, QWidget* parent)
    : QWidget(parent)
{
    auto* layout = new QHBoxLayout(this);
    layout->setContentsMargins(0, 4, 0, 4);
    layout->setSpacing(6);
    
    // Bullet point
    auto* bullet = new QLabel("●", this);
    bullet->setStyleSheet(QString("color: %1; font-size: 6px;").arg(TEXT_MUTED));
    bullet->setFixedWidth(12);
    layout->addWidget(bullet);
    
    // Action name (bold)
    auto* action = new QLabel(action_text(type), this);
    action->setStyleSheet(QString("color: %1; font-size: %2; font-weight: 600;").arg(TEXT_SECONDARY).arg(FONT_SM));
    layout->addWidget(action);
    
    // Detail text (lighter, same line)
    if (!detail.isEmpty()) {
        auto* detail_label = new QLabel(detail, this);
        detail_label->setStyleSheet(QString("color: %1; font-size: %2;").arg(TEXT_MUTED).arg(FONT_SM));
        layout->addWidget(detail_label);
    }
    
    layout->addStretch();
}

QString ToolActionWidget::action_text(ToolActionType type) const {
    switch (type) {
        case ToolActionType::Thought: return "Thought";
        case ToolActionType::Searched: return "Searched";
        case ToolActionType::Read: return "Read";
        case ToolActionType::Reviewed: return "Reviewed";
        case ToolActionType::Wrote: return "Wrote";
        case ToolActionType::Ran: return "Ran";
        default: return "";
    }
}

// ============================================================================
// FileBlockWidget
// ============================================================================

FileBlockWidget::FileBlockWidget(const FileBlockData& data, QWidget* parent)
    : QWidget(parent)
    , data_(data)
{
    setCursor(Qt::PointingHandCursor);
    
    setStyleSheet(QString(
        "FileBlockWidget { background: %1; border-radius: %2; }"
    ).arg(BG_FILE_BLOCK).arg(RADIUS_MD));
    
    auto* layout = new QHBoxLayout(this);
    layout->setContentsMargins(12, 10, 12, 10);
    layout->setSpacing(10);
    
    // File icon
    auto* icon = new QLabel("📄", this);
    icon->setStyleSheet("font-size: 16px;");
    layout->addWidget(icon);
    
    // Filename
    auto* name = new QLabel(data_.filename, this);
    name->setStyleSheet(QString("color: %1; font-size: %2;").arg(TEXT_PRIMARY).arg(FONT_BASE));
    layout->addWidget(name);
    
    layout->addStretch();
    
    // Diff stats
    QString diff_html;
    if (data_.lines_added > 0) {
        diff_html += QString("<span style='color: %1;'>+%2</span>").arg(ACCENT_GREEN).arg(data_.lines_added);
    }
    if (data_.lines_removed > 0) {
        if (!diff_html.isEmpty()) diff_html += " ";
        diff_html += QString("<span style='color: %1;'>-%2</span>").arg(ACCENT_RED).arg(data_.lines_removed);
    }
    
    if (!diff_html.isEmpty()) {
        auto* diff = new QLabel(this);
        diff->setTextFormat(Qt::RichText);
        diff->setText(diff_html);
        diff->setStyleSheet(QString("font-size: %1;").arg(FONT_SM));
        layout->addWidget(diff);
    }
}

void FileBlockWidget::mousePressEvent(QMouseEvent* event) {
    if (event->button() == Qt::LeftButton) {
        emit clicked(data_.filename);
    }
    QWidget::mousePressEvent(event);
}

void FileBlockWidget::enterEvent(QEnterEvent* event) {
    hovered_ = true;
    setStyleSheet(QString(
        "FileBlockWidget { background: %1; border-radius: %2; }"
    ).arg(BG_CARD_HOVER).arg(RADIUS_MD));
    QWidget::enterEvent(event);
}

void FileBlockWidget::leaveEvent(QEvent* event) {
    hovered_ = false;
    setStyleSheet(QString(
        "FileBlockWidget { background: %1; border-radius: %2; }"
    ).arg(BG_FILE_BLOCK).arg(RADIUS_MD));
    QWidget::leaveEvent(event);
}

// ============================================================================
// CodeBlockWidget
// ============================================================================

CodeBlockWidget::CodeBlockWidget(const QString& code, const QString& language, QWidget* parent)
    : QWidget(parent)
{
    Q_UNUSED(language);
    
    auto* layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 8, 0, 8);
    layout->setSpacing(0);
    
    // Code block container
    auto* code_container = new QWidget(this);
    code_container->setObjectName("CodeContainer");
    code_container->setStyleSheet(QString(
        "#CodeContainer { background: %1; border: 1px solid %2; border-radius: %3; }"
    ).arg(BG_CODE).arg(BORDER_DEFAULT).arg(RADIUS_MD));
    
    auto* code_layout = new QVBoxLayout(code_container);
    code_layout->setContentsMargins(14, 12, 14, 12);
    code_layout->setSpacing(0);
    
    code_label_ = new QLabel(code, code_container);
    code_label_->setWordWrap(true);
    code_label_->setTextInteractionFlags(Qt::TextSelectableByMouse);
    code_label_->setStyleSheet(QString(
        "color: %1; font-family: 'SF Mono', 'Menlo', 'Monaco', 'Consolas', monospace; "
        "font-size: %2; line-height: 1.4; background: transparent; border: none;"
    ).arg(TEXT_CODE).arg(FONT_SM));
    code_layout->addWidget(code_label_);
    
    layout->addWidget(code_container);
    
    // Output widget (hidden initially)
    output_widget_ = new QWidget(this);
    output_widget_->setObjectName("OutputContainer");
    output_widget_->setVisible(false);
    
    auto* output_layout = new QVBoxLayout(output_widget_);
    output_layout->setContentsMargins(14, 10, 14, 10);
    output_layout->setSpacing(0);
    
    output_label_ = new QLabel(output_widget_);
    output_label_->setWordWrap(true);
    output_label_->setTextInteractionFlags(Qt::TextSelectableByMouse);
    output_layout->addWidget(output_label_);
    
    layout->addWidget(output_widget_);
}

void CodeBlockWidget::set_output(const QString& output, bool is_error) {
    output_label_->setText(output);
    output_label_->setStyleSheet(QString(
        "color: %1; font-family: 'SF Mono', 'Menlo', 'Monaco', 'Consolas', monospace; "
        "font-size: %2; line-height: 1.4; background: transparent; border: none;"
    ).arg(is_error ? ACCENT_RED : TEXT_SECONDARY).arg(FONT_SM));
    
    output_widget_->setStyleSheet(QString(
        "#OutputContainer { background: %1; border: 1px solid %2; border-radius: %3; margin-top: 6px; }"
    ).arg(is_error ? "#1c0f0f" : BG_ELEVATED).arg(is_error ? "#3d1f1f" : BORDER_SUBTLE).arg(RADIUS_MD));
    
    output_widget_->setVisible(true);
}

// ============================================================================
// AssistantResponseWidget
// ============================================================================

AssistantResponseWidget::AssistantResponseWidget(QWidget* parent)
    : QWidget(parent)
{
    layout_ = new QVBoxLayout(this);
    layout_->setContentsMargins(20, 12, 20, 12);
    layout_->setSpacing(12);
}

void AssistantResponseWidget::add_thinking(int duration_seconds) {
    if (!thinking_) {
        thinking_ = new ThinkingIndicator(this);
        layout_->addWidget(thinking_);
    }
    
    if (duration_seconds > 0) {
        thinking_->stop(duration_seconds);
    } else {
        thinking_->start();
    }
}

void AssistantResponseWidget::add_tool_action(ToolActionType type, const QString& detail) {
    auto* action = new ToolActionWidget(type, detail, this);
    layout_->addWidget(action);
}

void AssistantResponseWidget::add_text(const QString& text) {
    auto* label = new QLabel(this);
    label->setWordWrap(true);
    label->setTextInteractionFlags(Qt::TextSelectableByMouse | Qt::TextBrowserInteraction);
    label->setOpenExternalLinks(true);
    
    // Render markdown to HTML
    QString html = markdown_to_html(text);
    label->setTextFormat(Qt::RichText);
    label->setText(html);
    
    label->setStyleSheet(QString(
        "QLabel { color: %1; font-size: %2; line-height: 1.5; background: transparent; }"
        "QLabel a { color: %3; }"
    ).arg(TEXT_PRIMARY).arg(FONT_BASE).arg(ACCENT_BLUE));
    layout_->addWidget(label);
}

void AssistantResponseWidget::add_file_block(const FileBlockData& data) {
    auto* block = new FileBlockWidget(data, this);
    layout_->addWidget(block);
}

void AssistantResponseWidget::add_code_block(const QString& code, const QString& language) {
    last_code_block_ = new CodeBlockWidget(code, language, this);
    layout_->addWidget(last_code_block_);
}

void AssistantResponseWidget::add_output(const QString& output, bool is_error) {
    if (last_code_block_) {
        last_code_block_->set_output(output, is_error);
    } else {
        // Standalone output
        auto* label = new QLabel(output, this);
        label->setWordWrap(true);
        label->setStyleSheet(QString(
            "color: %1; font-family: monospace; font-size: %2; background: %3; padding: 8px; border-radius: %4;"
        ).arg(is_error ? ACCENT_RED : TEXT_SECONDARY).arg(FONT_SM).arg(BG_ELEVATED).arg(RADIUS_MD));
        layout_->addWidget(label);
    }
}

void AssistantResponseWidget::add_summary(const QStringList& points) {
    for (const QString& point : points) {
        auto* item = new QLabel("• " + point, this);
        item->setWordWrap(true);
        item->setStyleSheet(QString("color: %1; font-size: %2; margin-left: 8px;").arg(TEXT_PRIMARY).arg(FONT_BASE));
        layout_->addWidget(item);
    }
}

// ============================================================================
// CursorChatView
// ============================================================================

CursorChatView::CursorChatView(QWidget* parent)
    : QWidget(parent)
{
    setup_ui();
}

void CursorChatView::setup_ui() {
    setStyleSheet(QString("background: %1;").arg(BG_BASE));
    
    auto* layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(0);
    
    // Scroll area
    scroll_area_ = new QScrollArea(this);
    scroll_area_->setWidgetResizable(true);
    scroll_area_->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    scroll_area_->setFrameShape(QFrame::NoFrame);
    scroll_area_->setStyleSheet(QString(
        "QScrollArea { border: none; background: %1; }"
        "QScrollBar:vertical { background: %1; width: 10px; margin: 2px; }"
        "QScrollBar::handle:vertical { background: %2; border-radius: 4px; min-height: 30px; }"
        "QScrollBar::add-line:vertical, QScrollBar::sub-line:vertical { height: 0; }"
        "QScrollBar::add-page:vertical, QScrollBar::sub-page:vertical { background: none; }"
    ).arg(BG_BASE).arg(BORDER_DEFAULT));
    
    // Content widget
    content_widget_ = new QWidget(scroll_area_);
    content_widget_->setStyleSheet(QString("background: %1;").arg(BG_BASE));
    
    content_layout_ = new QVBoxLayout(content_widget_);
    content_layout_->setContentsMargins(0, 20, 0, 20);
    content_layout_->setSpacing(8);
    content_layout_->addStretch();
    
    scroll_area_->setWidget(content_widget_);
    layout->addWidget(scroll_area_);
}

void CursorChatView::add_user_message(const QString& text) {
    auto* msg = new UserMessageWidget(text, content_widget_);
    content_layout_->insertWidget(content_layout_->count() - 1, msg);
    scroll_to_bottom();
}

AssistantResponseWidget* CursorChatView::start_assistant_response() {
    current_response_ = new AssistantResponseWidget(content_widget_);
    content_layout_->insertWidget(content_layout_->count() - 1, current_response_);
    return current_response_;
}

void CursorChatView::finish_assistant_response() {
    current_response_ = nullptr;
    scroll_to_bottom();
}

void CursorChatView::show_thinking() {
    if (!current_response_) {
        start_assistant_response();
    }
    current_response_->add_thinking();
    scroll_to_bottom();
}

void CursorChatView::hide_thinking(int duration_seconds) {
    if (current_response_ && current_response_->thinking_indicator()) {
        current_response_->thinking_indicator()->stop(duration_seconds);
    }
}

void CursorChatView::add_tool_action(ToolActionType type, const QString& detail) {
    if (!current_response_) {
        start_assistant_response();
    }
    current_response_->add_tool_action(type, detail);
    scroll_to_bottom();
}

void CursorChatView::add_assistant_text(const QString& text) {
    if (!current_response_) {
        start_assistant_response();
    }
    current_response_->add_text(text);
    scroll_to_bottom();
}

void CursorChatView::add_file_block(const FileBlockData& data) {
    if (!current_response_) {
        start_assistant_response();
    }
    current_response_->add_file_block(data);
    scroll_to_bottom();
}

void CursorChatView::add_code_block(const QString& code, const QString& language) {
    if (!current_response_) {
        start_assistant_response();
    }
    current_response_->add_code_block(code, language);
    scroll_to_bottom();
}

void CursorChatView::add_code_output(const QString& output, bool is_error) {
    if (!current_response_) {
        start_assistant_response();
    }
    current_response_->add_output(output, is_error);
    scroll_to_bottom();
}

void CursorChatView::add_summary(const QStringList& points) {
    if (!current_response_) {
        start_assistant_response();
    }
    current_response_->add_summary(points);
    scroll_to_bottom();
}

void CursorChatView::clear() {
    // Remove all widgets except the stretch
    while (content_layout_->count() > 1) {
        auto* item = content_layout_->takeAt(0);
        if (item->widget()) {
            item->widget()->deleteLater();
        }
        delete item;
    }
    current_response_ = nullptr;
}

void CursorChatView::scroll_to_bottom() {
    QTimer::singleShot(10, this, [this]() {
        scroll_area_->verticalScrollBar()->setValue(
            scroll_area_->verticalScrollBar()->maximum()
        );
    });
}

} // namespace ida_chat
