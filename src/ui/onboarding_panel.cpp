/**
 * @file onboarding_panel.cpp
 * @brief Onboarding panel for first-time setup and settings configuration.
 */

#include <ida_chat/ui/onboarding_panel.hpp>
#include <ida_chat/ui/markdown_renderer.hpp>
#include <ida_chat/plugin/settings.hpp>
#include <ida_chat/core/chat_core.hpp>  // for test_claude_connection

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QRadioButton>
#include <QButtonGroup>
#include <QStackedWidget>
#include <QTimer>

namespace ida_chat {

OnboardingPanel::OnboardingPanel(QWidget* parent)
    : QFrame(parent)
{
    setup_ui();
    load_current_settings();
}

OnboardingPanel::~OnboardingPanel() = default;

void OnboardingPanel::load_current_settings() {
    Settings settings;
    
    AuthType type = settings.auth_type();
    switch (type) {
        case AuthType::System:
            system_radio_->setChecked(true);
            break;
        case AuthType::OAuth:
            oauth_radio_->setChecked(true);
            break;
        case AuthType::ApiKey:
            apikey_radio_->setChecked(true);
            break;
        default:
            system_radio_->setChecked(true);
            break;
    }
    
    QString key = settings.api_key();
    if (!key.isEmpty()) {
        api_key_edit_->setText(key);
    }
    
    on_auth_type_changed();
}

AuthCredentials OnboardingPanel::get_credentials() const {
    AuthCredentials creds;
    creds.type = get_auth_type();
    
    if (creds.requires_key() && api_key_edit_) {
        creds.api_key = api_key_edit_->text().toStdString();
    }
    
    return creds;
}

void OnboardingPanel::on_auth_type_changed() {
    AuthType type = get_auth_type();
    
    // Show/hide API key input based on auth type
    if (key_stack_) {
        key_stack_->setCurrentIndex(type == AuthType::System ? 0 : 1);
    }
    
    // Update status
    if (status_label_) {
        status_label_->clear();
    }
}

void OnboardingPanel::on_test_clicked() {
    if (testing_) return;
    testing_ = true;
    
    test_button_->setEnabled(false);
    test_button_->setText("Testing...");
    status_label_->setText("Testing connection...");
    status_label_->setStyleSheet("color: #f59e0b;");  // Orange
    
    // Get credentials
    AuthCredentials creds = get_credentials();
    
    // Test in background (would normally use QThread)
    // For simplicity, we'll test synchronously
    QTimer::singleShot(100, this, [this, creds]() {
        auto [success, message] = test_claude_connection(creds);
        on_test_finished(success, QString::fromStdString(message));
    });
}

void OnboardingPanel::on_test_finished(bool success, const QString& message) {
    testing_ = false;
    test_button_->setEnabled(true);
    test_button_->setText("Test Connection");
    
    if (success) {
        status_label_->setText(QString::fromUtf8("\u2713 ") + message);  // ✓
        status_label_->setStyleSheet("color: #22c55e;");  // Green
        save_button_->setEnabled(true);
    } else {
        status_label_->setText(QString::fromUtf8("\u2717 ") + message);  // ✗
        status_label_->setStyleSheet("color: #ef4444;");  // Red
    }
}

void OnboardingPanel::on_save_clicked() {
    apply_current_settings();
    emit settings_applied();
    emit onboarding_complete();
}

AuthType OnboardingPanel::get_auth_type() const {
    if (apikey_radio_ && apikey_radio_->isChecked()) {
        return AuthType::ApiKey;
    }
    if (oauth_radio_ && oauth_radio_->isChecked()) {
        return AuthType::OAuth;
    }
    return AuthType::System;
}

void OnboardingPanel::apply_current_settings() {
    Settings settings;
    
    AuthType type = get_auth_type();
    settings.set_auth_type(type);
    
    if (type != AuthType::System && api_key_edit_) {
        settings.set_api_key(api_key_edit_->text());
    }
    
    settings.set_show_wizard(false);
}

void OnboardingPanel::setup_ui() {
    ColorScheme colors = ColorScheme::from_ida_palette();
    
    setStyleSheet(QString("background-color: %1;").arg(colors.window.name()));
    
    main_layout_ = new QVBoxLayout(this);
    main_layout_->setContentsMargins(40, 40, 40, 40);
    main_layout_->setSpacing(20);
    
    // Title
    title_label_ = new QLabel("IDA Chat Setup", this);
    title_label_->setStyleSheet(QString(
        "font-size: 24px; font-weight: bold; color: %1;"
    ).arg(colors.text.name()));
    title_label_->setAlignment(Qt::AlignCenter);
    main_layout_->addWidget(title_label_);
    
    // Description
    description_label_ = new QLabel(
        "Configure how IDA Chat connects to Claude.\n"
        "Choose an authentication method below.",
        this
    );
    description_label_->setStyleSheet(QString("color: %1; font-size: 13px;").arg(colors.mid.name()));
    description_label_->setAlignment(Qt::AlignCenter);
    description_label_->setWordWrap(true);
    main_layout_->addWidget(description_label_);
    
    main_layout_->addSpacing(20);
    
    // Auth type selection
    QFrame* auth_frame = new QFrame(this);
    auth_frame->setStyleSheet(QString(
        "QFrame { background-color: %1; border-radius: 8px; padding: 16px; }"
    ).arg(colors.alternate_base.name()));
    
    QVBoxLayout* auth_layout = new QVBoxLayout(auth_frame);
    auth_layout->setSpacing(12);
    
    auth_group_ = new QButtonGroup(this);
    
    // System auth option
    system_radio_ = new QRadioButton("Use System Configuration", auth_frame);
    system_radio_->setToolTip("Use existing Claude Code authentication (keychain, environment variables)");
    system_radio_->setStyleSheet(QString("color: %1;").arg(colors.text.name()));
    auth_group_->addButton(system_radio_);
    auth_layout->addWidget(system_radio_);
    
    QLabel* system_hint = new QLabel("Uses existing Claude Code setup or environment variables", auth_frame);
    system_hint->setStyleSheet(QString("color: %1; font-size: 11px; margin-left: 24px;").arg(colors.mid.name()));
    auth_layout->addWidget(system_hint);
    
    // OAuth option
    oauth_radio_ = new QRadioButton("OAuth Token", auth_frame);
    oauth_radio_->setStyleSheet(QString("color: %1;").arg(colors.text.name()));
    auth_group_->addButton(oauth_radio_);
    auth_layout->addWidget(oauth_radio_);
    
    // API Key option
    apikey_radio_ = new QRadioButton("API Key", auth_frame);
    apikey_radio_->setStyleSheet(QString("color: %1;").arg(colors.text.name()));
    auth_group_->addButton(apikey_radio_);
    auth_layout->addWidget(apikey_radio_);
    
    main_layout_->addWidget(auth_frame);
    
    // API key input (stacked widget)
    key_stack_ = new QStackedWidget(this);
    
    // Empty page for system auth
    QWidget* empty_page = new QWidget(key_stack_);
    key_stack_->addWidget(empty_page);
    
    // API key input page
    QWidget* key_page = new QWidget(key_stack_);
    QVBoxLayout* key_layout = new QVBoxLayout(key_page);
    key_layout->setContentsMargins(0, 0, 0, 0);
    
    QLabel* key_label = new QLabel("API Key / OAuth Token:", key_page);
    key_label->setStyleSheet(QString("color: %1;").arg(colors.text.name()));
    key_layout->addWidget(key_label);
    
    api_key_edit_ = new QLineEdit(key_page);
    api_key_edit_->setPlaceholderText("sk-ant-... or oauth token");
    api_key_edit_->setEchoMode(QLineEdit::Password);
    api_key_edit_->setStyleSheet(QString(
        "QLineEdit {"
        "  background-color: %1;"
        "  color: %2;"
        "  border: 1px solid %3;"
        "  border-radius: 4px;"
        "  padding: 8px;"
        "}"
    ).arg(colors.base.name()).arg(colors.text.name()).arg(colors.mid.name()));
    key_layout->addWidget(api_key_edit_);
    
    key_stack_->addWidget(key_page);
    main_layout_->addWidget(key_stack_);
    
    // Connect auth type change
    connect(system_radio_, &QRadioButton::toggled, this, &OnboardingPanel::on_auth_type_changed);
    connect(oauth_radio_, &QRadioButton::toggled, this, &OnboardingPanel::on_auth_type_changed);
    connect(apikey_radio_, &QRadioButton::toggled, this, &OnboardingPanel::on_auth_type_changed);
    
    // Buttons
    QHBoxLayout* button_layout = new QHBoxLayout();
    button_layout->setSpacing(12);
    
    test_button_ = new QPushButton("Test Connection", this);
    test_button_->setStyleSheet(QString(
        "QPushButton {"
        "  background-color: %1;"
        "  color: %2;"
        "  border: none;"
        "  border-radius: 4px;"
        "  padding: 10px 20px;"
        "  font-weight: bold;"
        "}"
        "QPushButton:hover {"
        "  background-color: %3;"
        "}"
        "QPushButton:disabled {"
        "  background-color: %4;"
        "}"
    ).arg(colors.button.name()).arg(colors.button_text.name())
     .arg(colors.mid.name()).arg(colors.dark.name()));
    connect(test_button_, &QPushButton::clicked, this, &OnboardingPanel::on_test_clicked);
    button_layout->addWidget(test_button_);
    
    save_button_ = new QPushButton("Save & Start", this);
    save_button_->setEnabled(false);
    save_button_->setStyleSheet(QString(
        "QPushButton {"
        "  background-color: %1;"
        "  color: %2;"
        "  border: none;"
        "  border-radius: 4px;"
        "  padding: 10px 20px;"
        "  font-weight: bold;"
        "}"
        "QPushButton:hover {"
        "  background-color: #1d4ed8;"
        "}"
        "QPushButton:disabled {"
        "  background-color: %3;"
        "  color: %4;"
        "}"
    ).arg(colors.highlight.name()).arg(colors.highlight_text.name())
     .arg(colors.dark.name()).arg(colors.mid.name()));
    connect(save_button_, &QPushButton::clicked, this, &OnboardingPanel::on_save_clicked);
    button_layout->addWidget(save_button_);
    
    main_layout_->addLayout(button_layout);
    
    // Status label
    status_label_ = new QLabel(this);
    status_label_->setAlignment(Qt::AlignCenter);
    status_label_->setWordWrap(true);
    main_layout_->addWidget(status_label_);
    
    main_layout_->addStretch();
    
    // Default to system auth
    system_radio_->setChecked(true);
    on_auth_type_changed();
}

} // namespace ida_chat
