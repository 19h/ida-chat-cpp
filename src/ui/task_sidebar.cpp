/**
 * @file task_sidebar.cpp
 * @brief Cursor-style task sidebar implementation.
 */

#include <ida_chat/ui/task_sidebar.hpp>
#include <ida_chat/ui/cursor_theme.hpp>

#include <QMouseEvent>
#include <QUuid>
#include <QStyle>
#include <QFrame>

namespace ida_chat {

using namespace theme;

// ============================================================================
// TaskCard Implementation
// ============================================================================

TaskCard::TaskCard(const TaskItem& task, QWidget* parent)
    : QWidget(parent)
    , task_(task)
    , animation_timer_(new QTimer(this))
{
    setup_ui();
    
    connect(animation_timer_, &QTimer::timeout, this, [this]() {
        animation_frame_ = (animation_frame_ + 1) % SPINNER_FRAMES.size();
        update_status_icon();
    });
    
    if (task_.status == TaskStatus::Generating) {
        animation_timer_->start(100);
    }
}

void TaskCard::setup_ui() {
    setCursor(Qt::PointingHandCursor);
    setStyleSheet(QString("background: transparent;"));
    
    auto* layout = new QHBoxLayout(this);
    layout->setContentsMargins(8, 10, 12, 10);
    layout->setSpacing(10);
    
    // Status icon
    status_icon_ = new QLabel(this);
    status_icon_->setFixedWidth(20);
    status_icon_->setAlignment(Qt::AlignCenter);
    layout->addWidget(status_icon_);
    
    // Content area
    auto* content = new QWidget(this);
    auto* content_layout = new QVBoxLayout(content);
    content_layout->setContentsMargins(0, 0, 0, 0);
    content_layout->setSpacing(2);
    
    // Title row
    auto* title_row = new QHBoxLayout();
    title_row->setSpacing(8);
    
    title_label_ = new QLabel(task_.title, content);
    title_label_->setStyleSheet(QString("color: %1; font-size: 13px; font-weight: 500;").arg(TEXT_PRIMARY));
    title_label_->setMaximumWidth(180);
    title_row->addWidget(title_label_, 1);
    
    time_label_ = new QLabel(format_time_ago(), content);
    time_label_->setStyleSheet(QString("color: %1; font-size: 11px;").arg(TEXT_MUTED));
    title_row->addWidget(time_label_);
    
    content_layout->addLayout(title_row);
    
    // Subtitle / summary row
    summary_label_ = new QLabel(content);
    summary_label_->setStyleSheet(QString("color: %1; font-size: 11px;").arg(TEXT_MUTED));
    summary_label_->setMaximumWidth(200);
    content_layout->addWidget(summary_label_);
    
    // Diff stats (hidden initially)
    diff_label_ = new QLabel(content);
    diff_label_->setStyleSheet(QString("font-size: 11px;"));
    diff_label_->setVisible(false);
    content_layout->addWidget(diff_label_);
    
    layout->addWidget(content, 1);
    
    update_status_icon();
    update_task(task_);
}

void TaskCard::update_task(const TaskItem& task) {
    task_ = task;
    
    title_label_->setText(task_.title);
    time_label_->setText(format_time_ago());
    
    // Update subtitle based on status
    if (task_.status == TaskStatus::Generating) {
        summary_label_->setText("Generating");
        summary_label_->setStyleSheet(QString("color: %1; font-size: 11px;").arg(TEXT_MUTED));
        diff_label_->setVisible(false);
    } else if (task_.status == TaskStatus::Error) {
        summary_label_->setText(task_.summary.isEmpty() ? "Error" : task_.summary);
        summary_label_->setStyleSheet(QString("color: %1; font-size: 11px;").arg(ACCENT_RED));
        diff_label_->setVisible(false);
    } else {
        // Complete - show diff stats and summary
        summary_label_->setText(task_.summary.isEmpty() ? "Complete" : task_.summary);
        summary_label_->setStyleSheet(QString("color: %1; font-size: 11px;").arg(TEXT_MUTED));
        
        // Show diff if we have it
        if (task_.lines_added > 0 || task_.lines_removed > 0) {
            QString diff_text;
            if (task_.lines_added > 0) {
                diff_text += QString("<span style='color: %1;'>+%2</span>").arg(ACCENT_GREEN).arg(task_.lines_added);
            }
            if (task_.lines_removed > 0) {
                if (!diff_text.isEmpty()) diff_text += " ";
                diff_text += QString("<span style='color: %1;'>-%2</span>").arg(ACCENT_RED).arg(task_.lines_removed);
            }
            diff_label_->setTextFormat(Qt::RichText);
            diff_label_->setText(diff_text);
            diff_label_->setVisible(true);
        }
    }
    
    // Update animation
    if (task_.status == TaskStatus::Generating) {
        if (!animation_timer_->isActive()) {
            animation_timer_->start(100);
        }
    } else {
        animation_timer_->stop();
    }
    
    update_status_icon();
}

void TaskCard::update_status_icon() {
    switch (task_.status) {
        case TaskStatus::Generating:
            status_icon_->setText(SPINNER_FRAMES[animation_frame_]);
            status_icon_->setStyleSheet(QString("color: %1; font-size: 14px;").arg(TEXT_MUTED));
            break;
        case TaskStatus::Complete:
            status_icon_->setText("⊙");
            status_icon_->setStyleSheet(QString("color: %1; font-size: 14px;").arg(ACCENT_GREEN));
            break;
        case TaskStatus::Error:
            status_icon_->setText("⊗");
            status_icon_->setStyleSheet(QString("color: %1; font-size: 14px;").arg(ACCENT_RED));
            break;
    }
}

QString TaskCard::format_time_ago() const {
    if (!task_.created.isValid()) return "";
    
    auto secs = task_.created.secsTo(QDateTime::currentDateTime());
    if (secs < 60) return "now";
    if (secs < 3600) return QString("%1m").arg(secs / 60);
    if (secs < 86400) return QString("%1h").arg(secs / 3600);
    return QString("%1d").arg(secs / 86400);
}

void TaskCard::mousePressEvent(QMouseEvent* event) {
    if (event->button() == Qt::LeftButton) {
        emit clicked(task_.id);
    }
    QWidget::mousePressEvent(event);
}

void TaskCard::enterEvent(QEnterEvent* event) {
    hovered_ = true;
    setStyleSheet(QString("background: %1; border-radius: 6px;").arg(BG_CARD_HOVER));
    QWidget::enterEvent(event);
}

void TaskCard::leaveEvent(QEvent* event) {
    hovered_ = false;
    setStyleSheet(QString("background: transparent;"));
    QWidget::leaveEvent(event);
}

// ============================================================================
// TaskSection Implementation
// ============================================================================

TaskSection::TaskSection(const QString& title, QWidget* parent)
    : QWidget(parent)
    , title_(title)
{
    auto* layout = new QVBoxLayout(this);
    layout->setContentsMargins(12, 0, 12, 0);
    layout->setSpacing(4);
    
    // Header
    header_label_ = new QLabel(this);
    header_label_->setStyleSheet(QString(
        "color: %1; font-size: 11px; font-weight: 600; text-transform: uppercase; letter-spacing: 0.5px; padding: 8px 0;"
    ).arg(TEXT_MUTED));
    layout->addWidget(header_label_);
    
    // Cards container
    auto* cards_widget = new QWidget(this);
    cards_layout_ = new QVBoxLayout(cards_widget);
    cards_layout_->setContentsMargins(0, 0, 0, 0);
    cards_layout_->setSpacing(2);
    layout->addWidget(cards_widget);
    
    update_header();
}

void TaskSection::add_task(const TaskItem& task) {
    auto* card = new TaskCard(task, this);
    connect(card, &TaskCard::clicked, this, &TaskSection::task_clicked);
    cards_.push_back(card);
    cards_layout_->insertWidget(0, card);  // Add at top
    update_header();
}

void TaskSection::update_task(const TaskItem& task) {
    for (auto* card : cards_) {
        if (card->task_id() == task.id) {
            card->update_task(task);
            break;
        }
    }
}

void TaskSection::remove_task(const QString& task_id) {
    for (auto it = cards_.begin(); it != cards_.end(); ++it) {
        if ((*it)->task_id() == task_id) {
            cards_layout_->removeWidget(*it);
            (*it)->deleteLater();
            cards_.erase(it);
            update_header();
            break;
        }
    }
}

void TaskSection::clear() {
    for (auto* card : cards_) {
        cards_layout_->removeWidget(card);
        card->deleteLater();
    }
    cards_.clear();
    update_header();
}

int TaskSection::task_count() const {
    return static_cast<int>(cards_.size());
}

void TaskSection::update_header() {
    header_label_->setText(QString("%1 %2").arg(title_).arg(cards_.size()));
    setVisible(!cards_.empty());
}

// ============================================================================
// TaskSidebar Implementation
// ============================================================================

TaskSidebar::TaskSidebar(QWidget* parent)
    : QWidget(parent)
{
    setup_ui();
}

void TaskSidebar::setup_ui() {
    setMinimumWidth(220);
    setMaximumWidth(320);
    setStyleSheet(QString("background: %1;").arg(BG_SIDEBAR));
    
    auto* layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(0);
    
    // Scroll area
    scroll_area_ = new QScrollArea(this);
    scroll_area_->setWidgetResizable(true);
    scroll_area_->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    scroll_area_->setFrameShape(QFrame::NoFrame);
    scroll_area_->setStyleSheet(QString(
        "QScrollArea { border: none; background: transparent; }"
        "QScrollBar:vertical { background: transparent; width: 6px; }"
        "QScrollBar::handle:vertical { background: %1; border-radius: 3px; min-height: 20px; }"
        "QScrollBar::add-line:vertical, QScrollBar::sub-line:vertical { height: 0; }"
        "QScrollBar::add-page:vertical, QScrollBar::sub-page:vertical { background: none; }"
    ).arg(BORDER_DEFAULT));
    
    auto* content = new QWidget(scroll_area_);
    content->setStyleSheet(QString("background: %1;").arg(BG_SIDEBAR));
    
    auto* content_layout = new QVBoxLayout(content);
    content_layout->setContentsMargins(0, 16, 0, 16);
    content_layout->setSpacing(16);
    
    // Sections
    in_progress_section_ = new TaskSection("IN PROGRESS", content);
    connect(in_progress_section_, &TaskSection::task_clicked, this, &TaskSidebar::task_selected);
    content_layout->addWidget(in_progress_section_);
    
    ready_section_ = new TaskSection("READY FOR REVIEW", content);
    connect(ready_section_, &TaskSection::task_clicked, this, &TaskSidebar::task_selected);
    content_layout->addWidget(ready_section_);
    
    content_layout->addStretch();
    
    scroll_area_->setWidget(content);
    layout->addWidget(scroll_area_);
}

QString TaskSidebar::add_task(const QString& title) {
    TaskItem task;
    task.id = QString::number(next_task_id_++);
    task.title = title;
    task.status = TaskStatus::Generating;
    task.created = QDateTime::currentDateTime();
    
    tasks_.push_back(task);
    in_progress_section_->add_task(task);
    current_task_id_ = task.id;
    
    emit task_selected(task.id);
    return task.id;
}

void TaskSidebar::update_task_status(const QString& task_id, TaskStatus status) {
    if (auto* task = find_task(task_id)) {
        task->status = status;
        
        if (status == TaskStatus::Complete) {
            task->completed = QDateTime::currentDateTime();
            move_task_to_section(task_id, in_progress_section_, ready_section_);
        }
        
        in_progress_section_->update_task(*task);
        ready_section_->update_task(*task);
    }
}

void TaskSidebar::update_task_summary(const QString& task_id, const QString& summary) {
    if (auto* task = find_task(task_id)) {
        task->summary = summary;
        in_progress_section_->update_task(*task);
        ready_section_->update_task(*task);
    }
}

void TaskSidebar::update_task_diff(const QString& task_id, int added, int removed) {
    if (auto* task = find_task(task_id)) {
        task->lines_added = added;
        task->lines_removed = removed;
        in_progress_section_->update_task(*task);
        ready_section_->update_task(*task);
    }
}

void TaskSidebar::update_task_cost(const QString& task_id, double cost, int turns) {
    if (auto* task = find_task(task_id)) {
        task->cost = cost;
        task->turns = turns;
        in_progress_section_->update_task(*task);
        ready_section_->update_task(*task);
    }
}

void TaskSidebar::complete_task(const QString& task_id, const QString& summary) {
    if (!summary.isEmpty()) {
        update_task_summary(task_id, summary);
    }
    update_task_status(task_id, TaskStatus::Complete);
}

void TaskSidebar::error_task(const QString& task_id, const QString& error) {
    update_task_summary(task_id, error);
    update_task_status(task_id, TaskStatus::Error);
}

TaskItem* TaskSidebar::find_task(const QString& task_id) {
    for (auto& task : tasks_) {
        if (task.id == task_id) {
            return &task;
        }
    }
    return nullptr;
}

void TaskSidebar::move_task_to_section(const QString& task_id, TaskSection* from, TaskSection* to) {
    if (auto* task = find_task(task_id)) {
        from->remove_task(task_id);
        to->add_task(*task);
    }
}

} // namespace ida_chat
