/**
 * @file progress_timeline.cpp
 * @brief Compact progress timeline showing agent stages.
 */

#include <ida_chat/ui/progress_timeline.hpp>
#include <ida_chat/ui/markdown_renderer.hpp>

#include <QHBoxLayout>
#include <QLabel>

namespace ida_chat {

ProgressTimeline::ProgressTimeline(QWidget* parent)
    : QFrame(parent)
{
    setup_ui();
}

ProgressTimeline::~ProgressTimeline() = default;

void ProgressTimeline::reset() {
    stages_.clear();
    completed_ = false;
    stages_.append("User");
    update_display();
    setVisible(true);
}

void ProgressTimeline::add_stage(const QString& name) {
    stages_.append(name);
    update_display();
}

void ProgressTimeline::complete() {
    completed_ = true;
    update_display();
}

void ProgressTimeline::hide_timeline() {
    setVisible(false);
}

void ProgressTimeline::setup_ui() {
    ColorScheme colors = ColorScheme::from_ida_palette();
    
    setStyleSheet(QString("background-color: %1;").arg(colors.window.name()));
    
    layout_ = new QHBoxLayout(this);
    layout_->setContentsMargins(10, 4, 10, 4);
    layout_->setSpacing(4);
    
    label_ = new QLabel(this);
    label_->setStyleSheet(QString("color: %1; font-size: 10px;").arg(colors.mid.name()));
    layout_->addWidget(label_);
    layout_->addStretch();
    
    setVisible(false);
}

void ProgressTimeline::update_display() {
    QStringList parts;
    
    // Track script count
    int script_count = 0;
    QString current_stage;
    
    for (const QString& stage : stages_) {
        if (stage.startsWith("Script ")) {
            bool ok;
            int num = stage.mid(7).toInt(&ok);
            if (ok && num > script_count) {
                script_count = num;
            }
        }
        current_stage = stage;
    }
    
    // Always show User as complete
    parts.append("<span style='color: #22c55e;'>&#x2713; User</span>");  // ✓
    
    // Show script count if any
    if (script_count > 0) {
        if (completed_) {
            parts.append(QString("<span style='color: #22c55e;'>&#x2713; %1 scripts</span>").arg(script_count));
        } else {
            parts.append(QString("<b style='color: #f59e0b;'>%1 scripts</b>").arg(script_count));
        }
    }
    
    // Show current stage (Thinking, Retrying, Done)
    if (completed_) {
        parts.append("<span style='color: #22c55e;'>&#x2713; Done</span>");
    } else if (!current_stage.isEmpty() && 
               current_stage != "User" && 
               !current_stage.startsWith("Script")) {
        parts.append(QString("<b style='color: #f59e0b;'>%1</b>").arg(current_stage));
    }
    
    label_->setText(parts.join(QString::fromUtf8(" \u2192 ")));  // →
}

} // namespace ida_chat
