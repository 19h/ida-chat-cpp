/**
 * @file task_sidebar.hpp
 * @brief Cursor-style task sidebar with IN PROGRESS / READY FOR REVIEW sections.
 */

#pragma once

#include <QWidget>
#include <QVBoxLayout>
#include <QScrollArea>
#include <QLabel>
#include <QString>
#include <QDateTime>
#include <QTimer>
#include <vector>
#include <memory>

namespace ida_chat {

// ============================================================================
// Task Status
// ============================================================================

enum class TaskStatus {
    Generating,     // Spinning indicator, in progress
    Complete,       // Checkmark, ready for review
    Error           // Error state
};

// ============================================================================
// Task Item Data
// ============================================================================

struct TaskItem {
    QString id;
    QString title;
    QString summary;
    TaskStatus status = TaskStatus::Generating;
    QDateTime created;
    QDateTime completed;
    int lines_added = 0;
    int lines_removed = 0;
    double cost = 0.0;
    int turns = 0;
};

// ============================================================================
// Task Card Widget
// ============================================================================

class TaskCard : public QWidget {
    Q_OBJECT
    
public:
    explicit TaskCard(const TaskItem& task, QWidget* parent = nullptr);
    
    void update_task(const TaskItem& task);
    QString task_id() const { return task_.id; }
    
signals:
    void clicked(const QString& task_id);
    
protected:
    void mousePressEvent(QMouseEvent* event) override;
    void enterEvent(QEnterEvent* event) override;
    void leaveEvent(QEvent* event) override;
    
private:
    void setup_ui();
    void update_status_icon();
    QString format_time_ago() const;
    
    TaskItem task_;
    QLabel* status_icon_;
    QLabel* title_label_;
    QLabel* summary_label_;
    QLabel* time_label_;
    QLabel* diff_label_;
    QTimer* animation_timer_;
    int animation_frame_ = 0;
    bool hovered_ = false;
};

// ============================================================================
// Task Section Widget (header + list of cards)
// ============================================================================

class TaskSection : public QWidget {
    Q_OBJECT
    
public:
    TaskSection(const QString& title, QWidget* parent = nullptr);
    
    void add_task(const TaskItem& task);
    void update_task(const TaskItem& task);
    void remove_task(const QString& task_id);
    void clear();
    int task_count() const;
    
signals:
    void task_clicked(const QString& task_id);
    
private:
    void update_header();
    
    QString title_;
    QLabel* header_label_;
    QVBoxLayout* cards_layout_;
    std::vector<TaskCard*> cards_;
};

// ============================================================================
// Task Sidebar Widget
// ============================================================================

class TaskSidebar : public QWidget {
    Q_OBJECT
    
public:
    explicit TaskSidebar(QWidget* parent = nullptr);
    
    // Add a new task (starts in Generating state)
    QString add_task(const QString& title);
    
    // Update task status
    void update_task_status(const QString& task_id, TaskStatus status);
    void update_task_summary(const QString& task_id, const QString& summary);
    void update_task_diff(const QString& task_id, int added, int removed);
    void update_task_cost(const QString& task_id, double cost, int turns);
    
    // Complete a task (moves to Ready for Review)
    void complete_task(const QString& task_id, const QString& summary = "");
    
    // Mark task as error
    void error_task(const QString& task_id, const QString& error);
    
    // Get current task
    QString current_task_id() const { return current_task_id_; }
    
signals:
    void task_selected(const QString& task_id);
    
private:
    void setup_ui();
    TaskItem* find_task(const QString& task_id);
    void move_task_to_section(const QString& task_id, TaskSection* from, TaskSection* to);
    
    QScrollArea* scroll_area_;
    TaskSection* in_progress_section_;
    TaskSection* ready_section_;
    
    std::vector<TaskItem> tasks_;
    QString current_task_id_;
    int next_task_id_ = 1;
};

} // namespace ida_chat
