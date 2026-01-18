/**
 * @file collapsible_section.cpp
 * @brief Expandable/collapsible section widget for long content.
 */

#include <ida_chat/ui/collapsible_section.hpp>
#include <ida_chat/ui/markdown_renderer.hpp>

#include <QVBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QTextEdit>

namespace ida_chat {

CollapsibleSection::CollapsibleSection(const QString& title,
                                       const QString& content,
                                       bool collapsed,
                                       QWidget* parent)
    : QFrame(parent)
    , title_(title)
    , content_(content)
    , collapsed_(collapsed)
{
    setup_ui();
}

CollapsibleSection::~CollapsibleSection() = default;

bool CollapsibleSection::should_collapse(const QString& content) {
    return content.trimmed().count('\n') + 1 > COLLAPSE_THRESHOLD;
}

void CollapsibleSection::expand() {
    if (!collapsed_) return;
    collapsed_ = false;
    update_header_text();
    update_content();
}

void CollapsibleSection::collapse() {
    if (collapsed_) return;
    collapsed_ = true;
    update_header_text();
    update_content();
}

void CollapsibleSection::toggle() {
    if (collapsed_) {
        expand();
    } else {
        collapse();
    }
}

void CollapsibleSection::setup_ui() {
    ColorScheme colors = ColorScheme::from_ida_palette();
    
    layout_ = new QVBoxLayout(this);
    layout_->setContentsMargins(0, 0, 0, 0);
    layout_->setSpacing(2);
    
    // Header button
    header_ = new QPushButton(this);
    update_header_text();
    header_->setStyleSheet(QString(
        "QPushButton {"
        "  background-color: transparent;"
        "  color: %1;"
        "  border: none;"
        "  text-align: left;"
        "  padding: 2px 4px;"
        "  font-size: 11px;"
        "}"
        "QPushButton:hover {"
        "  color: %2;"
        "}"
    ).arg(colors.mid.name()).arg(colors.text.name()));
    
    connect(header_, &QPushButton::clicked, this, &CollapsibleSection::toggle);
    layout_->addWidget(header_);
    
    // Content widget
    content_widget_ = new QTextEdit(this);
    content_widget_->setReadOnly(true);
    content_widget_->setStyleSheet(QString(
        "QTextEdit {"
        "  background-color: %1;"
        "  color: %2;"
        "  padding: 8px;"
        "  border-radius: 4px;"
        "  font-family: monospace;"
        "  font-size: 11px;"
        "  border: none;"
        "}"
    ).arg(colors.alternate_base.name()).arg(colors.text.name()));
    
    update_content();
    layout_->addWidget(content_widget_);
}

void CollapsibleSection::update_header_text() {
    QString arrow = collapsed_ ? QString::fromUtf8("\u25B6") : QString::fromUtf8("\u25BC");  // ▶ or ▼
    int line_count = content_.trimmed().count('\n') + 1;
    header_->setText(QString("%1 %2 (%3 lines)").arg(arrow).arg(title_).arg(line_count));
}

void CollapsibleSection::update_content() {
    if (collapsed_) {
        // Show first few lines with ellipsis
        QStringList lines = content_.trimmed().split('\n');
        QString preview;
        int preview_lines = qMin(3, lines.size());
        for (int i = 0; i < preview_lines; ++i) {
            if (!preview.isEmpty()) preview += '\n';
            preview += lines[i];
        }
        if (lines.size() > 3) {
            preview += QString("\n... (%1 more lines)").arg(lines.size() - 3);
        }
        content_widget_->setPlainText(preview);
        content_widget_->setMaximumHeight(100);
    } else {
        content_widget_->setPlainText(content_);
        content_widget_->setMaximumHeight(16777215);  // QWIDGETSIZE_MAX
    }
}

} // namespace ida_chat
