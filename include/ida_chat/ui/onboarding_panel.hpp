/**
 * @file onboarding_panel.hpp
 * @brief Onboarding panel for first-time setup and settings configuration.
 */

#pragma once

#include <ida_chat/core/types.hpp>

#include <QFrame>
#include <QString>
#include <QLabel>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLineEdit>
#include <QPushButton>
#include <QRadioButton>
#include <QButtonGroup>
#include <QStackedWidget>

namespace ida_chat {

/**
 * @brief Onboarding panel for first-time setup and settings configuration.
 * 
 * Allows users to configure:
 * - Authentication method (System/OAuth/API Key)
 * - API key entry
 * - Connection testing
 */
class OnboardingPanel : public QFrame {
    Q_OBJECT

public:
    explicit OnboardingPanel(QWidget* parent = nullptr);
    ~OnboardingPanel() override;
    
    /**
     * @brief Load current settings into the panel.
     */
    void load_current_settings();
    
    /**
     * @brief Get the configured credentials.
     */
    [[nodiscard]] AuthCredentials get_credentials() const;

signals:
    /**
     * @brief Emitted when user clicks Save & Start.
     */
    void onboarding_complete();
    
    /**
     * @brief Emitted when settings are applied.
     */
    void settings_applied();

private slots:
    void on_auth_type_changed();
    void on_test_clicked();
    void on_test_finished(bool success, const QString& message);
    void on_save_clicked();

private:
    void setup_ui();
    void apply_current_settings();
    [[nodiscard]] AuthType get_auth_type() const;
    
    // UI components
    QVBoxLayout* main_layout_ = nullptr;
    QLabel* splash_label_ = nullptr;
    QLabel* title_label_ = nullptr;
    QLabel* description_label_ = nullptr;
    
    // Auth type selection
    QButtonGroup* auth_group_ = nullptr;
    QRadioButton* system_radio_ = nullptr;
    QRadioButton* oauth_radio_ = nullptr;
    QRadioButton* apikey_radio_ = nullptr;
    
    // API key entry
    QStackedWidget* key_stack_ = nullptr;
    QLineEdit* api_key_edit_ = nullptr;
    
    // Action buttons
    QPushButton* test_button_ = nullptr;
    QPushButton* save_button_ = nullptr;
    QLabel* status_label_ = nullptr;
    
    // State
    bool testing_ = false;
};

} // namespace ida_chat
