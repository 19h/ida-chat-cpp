/**
 * @file settings.hpp
 * @brief Plugin settings persistence.
 */

#pragma once

#include <ida_chat/core/types.hpp>

#include <string>
#include <optional>
#include <QString>

namespace ida_chat {

/**
 * @brief Get whether to show the setup wizard.
 */
[[nodiscard]] bool get_show_wizard();

/**
 * @brief Set whether to show the setup wizard.
 */
void set_show_wizard(bool value);

/**
 * @brief Get the configured authentication type.
 * @return AuthType or None if not configured
 */
[[nodiscard]] AuthType get_auth_type();

/**
 * @brief Get the stored API key/token.
 * @return API key or nullopt if not set
 */
[[nodiscard]] std::optional<std::string> get_api_key();

/**
 * @brief Save authentication settings.
 * @param auth_type Authentication type
 * @param api_key API key (not required for System auth)
 */
void save_auth_settings(AuthType auth_type, const std::string& api_key = "");

/**
 * @brief Get the full credentials from settings.
 */
[[nodiscard]] AuthCredentials get_auth_credentials();

/**
 * @brief Apply saved auth settings to the environment.
 * 
 * Sets environment variables based on stored settings:
 * - For System auth: Uses existing Claude Code authentication
 * - For OAuth/ApiKey: Sets ANTHROPIC_API_KEY
 */
void apply_auth_to_environment();

/**
 * @brief Clear all stored settings.
 */
void clear_settings();

/**
 * @brief Settings keys used for persistence.
 */
namespace settings_keys {
    constexpr const char* SHOW_WIZARD = "show_wizard";
    constexpr const char* AUTH_TYPE = "auth_type";
    constexpr const char* API_KEY = "api_key";
}

/**
 * @brief Convenience wrapper class for settings access.
 * 
 * Wraps the free functions for easier use in UI code.
 */
class Settings {
public:
    [[nodiscard]] bool show_wizard() const { return get_show_wizard(); }
    void set_show_wizard(bool value) { ::ida_chat::set_show_wizard(value); }
    
    [[nodiscard]] AuthType auth_type() const { return get_auth_type(); }
    void set_auth_type(AuthType type) { save_auth_settings(type); }
    
    [[nodiscard]] QString api_key() const {
        auto key = get_api_key();
        return key.has_value() ? QString::fromStdString(key.value()) : QString();
    }
    void set_api_key(const QString& key) {
        save_auth_settings(auth_type(), key.toStdString());
    }
    
    [[nodiscard]] AuthCredentials credentials() const { return get_auth_credentials(); }
};

} // namespace ida_chat
