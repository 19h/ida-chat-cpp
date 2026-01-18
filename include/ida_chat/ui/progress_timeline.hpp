/**
 * @file progress_timeline.hpp
 * @brief Horizontal progress timeline widget.
 */

#pragma once

#include <ida_chat/core/types.hpp>

#include <QFrame>
#include <QString>
#include <QList>
#include <QLabel>
#include <QHBoxLayout>

namespace ida_chat {

/**
 * @brief Compact progress timeline showing agent stages.
 * 
 * Displays current stage of agent processing with visual indicators.
 */
class ProgressTimeline : public QFrame {
    Q_OBJECT

public:
    explicit ProgressTimeline(QWidget* parent = nullptr);
    ~ProgressTimeline() override;
    
    /**
     * @brief Reset the timeline to initial state.
     */
    void reset();
    
    /**
     * @brief Add a new stage.
     * @param name Stage name (e.g., "Thinking", "Executing", etc.)
     */
    void add_stage(const QString& name);
    
    /**
     * @brief Mark current processing as complete.
     */
    void complete();
    
    /**
     * @brief Hide the timeline.
     */
    void hide_timeline();

private:
    void setup_ui();
    void update_display();
    
    QStringList stages_;
    bool completed_ = false;
    
    QLabel* label_ = nullptr;
    QHBoxLayout* layout_ = nullptr;
};

} // namespace ida_chat
