/**
 * @file cursor_input.cpp
 * @brief Cursor-style input widget implementation.
 */

#include <ida_chat/ui/cursor_input.hpp>
#include <ida_chat/ui/cursor_theme.hpp>

#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QKeyEvent>

namespace ida_chat {

using namespace theme;

CursorInputWidget::CursorInputWidget(QWidget* parent)
    : QWidget(parent)
{
    setup_ui();
}

void CursorInputWidget::setup_ui() {
    setStyleSheet(QString("background: %1;").arg(BG_BASE));
    
    auto* main_layout = new QVBoxLayout(this);
    main_layout->setContentsMargins(16, 12, 16, 16);
    main_layout->setSpacing(10);
    
    // Text input - simple, minimal border
    text_edit_ = new QTextEdit(this);
    text_edit_->setPlaceholderText("Plan, search, build anything...");
    text_edit_->setStyleSheet(QString(
        "QTextEdit {"
        "  background: %1;"
        "  color: %2;"
        "  border: 1px solid %3;"
        "  border-radius: 8px;"
        "  padding: 12px;"
        "  font-size: %4;"
        "  selection-background-color: %5;"
        "}"
        "QTextEdit:focus {"
        "  border-color: %6;"
        "}"
    ).arg(BG_INPUT).arg(TEXT_PRIMARY).arg(BORDER_SUBTLE).arg(FONT_BASE).arg(ACCENT_BLUE).arg(BORDER_DEFAULT));
    text_edit_->setMinimumHeight(60);
    text_edit_->setMaximumHeight(120);
    text_edit_->setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    text_edit_->installEventFilter(this);
    main_layout->addWidget(text_edit_);
    
    // Bottom row with controls
    auto* controls = new QWidget(this);
    auto* controls_layout = new QHBoxLayout(controls);
    controls_layout->setContentsMargins(0, 0, 0, 0);
    controls_layout->setSpacing(12);
    
    // Agent button (pill style)
    agent_button_ = new QPushButton("∞ Agent", controls);
    agent_button_->setCheckable(true);
    agent_button_->setChecked(true);
    agent_button_->setCursor(Qt::PointingHandCursor);
    agent_button_->setStyleSheet(QString(
        "QPushButton {"
        "  background: %1;"
        "  color: %2;"
        "  border: none;"
        "  border-radius: 12px;"
        "  padding: 5px 14px;"
        "  font-size: %3;"
        "}"
        "QPushButton:checked {"
        "  background: %4;"
        "  color: %5;"
        "}"
        "QPushButton:hover {"
        "  background: %6;"
        "}"
    ).arg(BG_CARD).arg(TEXT_SECONDARY).arg(FONT_SM)
     .arg(BG_ELEVATED).arg(TEXT_PRIMARY).arg(BG_CARD_HOVER));
    controls_layout->addWidget(agent_button_);
    
    // Model label (just text, hidden for CLI mode)
    model_combo_ = new QComboBox(controls);
    model_combo_->addItems({"claude-sonnet-4-20250514"});
    model_combo_->setStyleSheet(QString(
        "QComboBox {"
        "  background: transparent;"
        "  color: %1;"
        "  border: none;"
        "  padding: 4px 0;"
        "  font-size: %2;"
        "}"
        "QComboBox::drop-down { width: 0; border: none; }"
        "QComboBox::down-arrow { image: none; }"
    ).arg(TEXT_MUTED).arg(FONT_SM));
    model_combo_->setVisible(false);  // Hidden by default for CLI mode
    controls_layout->addWidget(model_combo_);
    
    controls_layout->addStretch();
    
    // Hint label
    hint_label_ = new QLabel("/ for commands · @ for files", controls);
    hint_label_->setStyleSheet(QString("color: %1; font-size: %2;").arg(TEXT_PLACEHOLDER).arg(FONT_XS));
    controls_layout->addWidget(hint_label_);
    
    // Submit button (circle)
    submit_button_ = new QPushButton("↑", controls);
    submit_button_->setFixedSize(32, 32);
    submit_button_->setCursor(Qt::PointingHandCursor);
    submit_button_->setStyleSheet(QString(
        "QPushButton {"
        "  background: %1;"
        "  color: %2;"
        "  border: none;"
        "  border-radius: 16px;"
        "  font-size: 16px;"
        "  font-weight: bold;"
        "}"
        "QPushButton:hover {"
        "  background: %3;"
        "}"
        "QPushButton:disabled {"
        "  background: %4;"
        "  color: %5;"
        "}"
    ).arg(BG_ELEVATED).arg(TEXT_PRIMARY).arg(BG_CARD_HOVER).arg(BG_CARD).arg(TEXT_MUTED));
    
    connect(submit_button_, &QPushButton::clicked, this, &CursorInputWidget::submit);
    controls_layout->addWidget(submit_button_);
    
    main_layout->addWidget(controls);
    
    // Update button state when text changes
    connect(text_edit_, &QTextEdit::textChanged, this, &CursorInputWidget::update_submit_button);
    update_submit_button();
}

QString CursorInputWidget::text() const {
    return text_edit_->toPlainText().trimmed();
}

void CursorInputWidget::clear() {
    text_edit_->clear();
}

void CursorInputWidget::set_enabled(bool enabled) {
    enabled_ = enabled;
    text_edit_->setEnabled(enabled);
    submit_button_->setEnabled(enabled && !text().isEmpty());
    
    if (!enabled) {
        text_edit_->setPlaceholderText("Processing...");
    } else {
        text_edit_->setPlaceholderText("Plan, search, build anything...");
    }
}

void CursorInputWidget::set_placeholder(const QString& text) {
    text_edit_->setPlaceholderText(text);
}

void CursorInputWidget::focus() {
    text_edit_->setFocus();
}

void CursorInputWidget::set_model(const QString& model) {
    int idx = model_combo_->findText(model);
    if (idx >= 0) {
        model_combo_->setCurrentIndex(idx);
    }
}

QString CursorInputWidget::current_model() const {
    return model_combo_->currentText();
}

void CursorInputWidget::set_available_models(const QStringList& models) {
    QString current = model_combo_->currentText();
    model_combo_->clear();
    model_combo_->addItems(models);
    
    int idx = model_combo_->findText(current);
    if (idx >= 0) {
        model_combo_->setCurrentIndex(idx);
    }
    
    // Show model selector only if there are options
    model_combo_->setVisible(models.size() > 1);
}

bool CursorInputWidget::eventFilter(QObject* obj, QEvent* event) {
    if (obj == text_edit_ && event->type() == QEvent::KeyPress) {
        auto* key = static_cast<QKeyEvent*>(event);
        
        // Enter without modifier = submit
        if (key->key() == Qt::Key_Return && key->modifiers() == Qt::NoModifier) {
            submit();
            return true;
        }
        
        // Escape = cancel
        if (key->key() == Qt::Key_Escape) {
            emit cancel_requested();
            return true;
        }
    }
    return QWidget::eventFilter(obj, event);
}

void CursorInputWidget::submit() {
    QString msg = text();
    if (!msg.isEmpty() && enabled_) {
        emit message_submitted(msg);
        clear();
    }
}

void CursorInputWidget::update_submit_button() {
    submit_button_->setEnabled(enabled_ && !text().isEmpty());
}

} // namespace ida_chat
