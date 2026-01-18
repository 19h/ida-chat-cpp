/**
 * @file cursor_input.hpp
 * @brief Cursor-style input widget with Agent toggle and model selector.
 */

#pragma once

#include <QWidget>
#include <QTextEdit>
#include <QLabel>
#include <QPushButton>
#include <QComboBox>
#include <QString>

namespace ida_chat {

// ============================================================================
// Cursor Input Widget
// ============================================================================

class CursorInputWidget : public QWidget {
    Q_OBJECT
    
public:
    explicit CursorInputWidget(QWidget* parent = nullptr);
    
    QString text() const;
    void clear();
    void set_enabled(bool enabled);
    void set_placeholder(const QString& text);
    void focus();
    
    // Model selection
    void set_model(const QString& model);
    QString current_model() const;
    void set_available_models(const QStringList& models);
    
signals:
    void message_submitted(const QString& text);
    void cancel_requested();
    void model_changed(const QString& model);
    
protected:
    bool eventFilter(QObject* obj, QEvent* event) override;
    
private:
    void setup_ui();
    void submit();
    void update_submit_button();
    
    QTextEdit* text_edit_;
    QPushButton* agent_button_;
    QComboBox* model_combo_;
    QPushButton* submit_button_;
    QLabel* hint_label_;
    
    bool enabled_ = true;
};

} // namespace ida_chat
