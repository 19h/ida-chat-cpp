/**
 * @file collapsible_section.hpp
 * @brief Expandable/collapsible section widget for long content.
 */

#pragma once

#include <QFrame>
#include <QString>
#include <QLabel>
#include <QVBoxLayout>
#include <QTextEdit>
#include <QPushButton>

namespace ida_chat {

/**
 * @brief Expandable/collapsible section for long content.
 * 
 * Auto-collapses content that exceeds COLLAPSE_THRESHOLD lines.
 */
class CollapsibleSection : public QFrame {
    Q_OBJECT

public:
    static constexpr int COLLAPSE_THRESHOLD = 10;
    
    /**
     * @brief Create a collapsible section.
     * @param title Section title
     * @param content Section content
     * @param collapsed Whether to start collapsed
     * @param parent Parent widget
     */
    CollapsibleSection(const QString& title,
                       const QString& content,
                       bool collapsed = true,
                       QWidget* parent = nullptr);
    
    ~CollapsibleSection() override;
    
    /**
     * @brief Check whether content should be collapsed.
     * @param content Content to check
     * @return true if line count exceeds COLLAPSE_THRESHOLD
     */
    [[nodiscard]] static bool should_collapse(const QString& content);
    
    /**
     * @brief Expand the section.
     */
    void expand();
    
    /**
     * @brief Collapse the section.
     */
    void collapse();
    
    /**
     * @brief Check if section is collapsed.
     */
    [[nodiscard]] bool is_collapsed() const noexcept { return collapsed_; }

private slots:
    void toggle();

private:
    void setup_ui();
    void update_header_text();
    void update_content();
    
    QString title_;
    QString content_;
    bool collapsed_;
    
    QPushButton* header_ = nullptr;
    QTextEdit* content_widget_ = nullptr;
    QVBoxLayout* layout_ = nullptr;
};

} // namespace ida_chat
