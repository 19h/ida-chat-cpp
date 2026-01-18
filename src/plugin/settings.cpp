/**
 * @file settings.cpp
 * @brief Plugin settings persistence implementation.
 */

// Include json.hpp FIRST before anything else (to avoid macro pollution)
#include <ida_chat/common/json.hpp>

#include <ida_chat/plugin/settings.hpp>
#include <ida_chat/core/types.hpp>

#include <cstdlib>
#include <fstream>

namespace ida_chat {

// ============================================================================
// Settings Storage
// ============================================================================

// Use file-based JSON storage for settings (cross-platform and simple)

static std::string get_settings_file_path() {
    return get_config_directory() + IDA_CHAT_PATH_SEP_STR "settings.json";
}

static nlohmann::json load_settings() {
    auto path = get_settings_file_path();
    std::ifstream file(path);
    if (file) {
        try {
            return nlohmann::json::parse(file);
        } catch (...) {}
    }
    return nlohmann::json::object();
}

static void save_settings(const nlohmann::json& settings) {
    ensure_directory_exists(get_config_directory());
    auto path = get_settings_file_path();
    std::ofstream file(path);
    if (file) {
        file << settings.dump(2);
    }
}

bool get_show_wizard() {
    auto settings = load_settings();
    return settings.value(settings_keys::SHOW_WIZARD, true);
}

void set_show_wizard(bool value) {
    auto settings = load_settings();
    settings[settings_keys::SHOW_WIZARD] = value;
    save_settings(settings);
}

AuthType get_auth_type() {
    auto settings = load_settings();
    std::string type_str = settings.value(settings_keys::AUTH_TYPE, "");
    return auth_type_from_str(type_str);
}

std::optional<std::string> get_api_key() {
    auto settings = load_settings();
    if (settings.contains(settings_keys::API_KEY)) {
        return settings[settings_keys::API_KEY].get<std::string>();
    }
    return std::nullopt;
}

void save_auth_settings(AuthType auth_type, const std::string& api_key) {
    auto settings = load_settings();
    settings[settings_keys::AUTH_TYPE] = auth_type_str(auth_type);
    if (!api_key.empty()) {
        settings[settings_keys::API_KEY] = api_key;
    } else {
        settings.erase(settings_keys::API_KEY);
    }
    settings[settings_keys::SHOW_WIZARD] = false;
    save_settings(settings);
}

void clear_settings() {
    auto path = get_settings_file_path();
    std::remove(path.c_str());
}

// ============================================================================
// Common Functions
// ============================================================================

AuthCredentials get_auth_credentials() {
    AuthCredentials creds;
    creds.type = get_auth_type();
    
    auto key = get_api_key();
    if (key.has_value()) {
        creds.api_key = key.value();
    }
    
    return creds;
}

void apply_auth_to_environment() {
    auto creds = get_auth_credentials();
    
    switch (creds.type) {
        case AuthType::System:
            // System auth uses existing Claude Code authentication
            // No environment changes needed
            break;
            
        case AuthType::OAuth:
        case AuthType::ApiKey:
            // Set API key in environment
            if (!creds.api_key.empty()) {
#ifdef IDA_CHAT_WINDOWS
                _putenv_s("ANTHROPIC_API_KEY", creds.api_key.c_str());
#else
                setenv("ANTHROPIC_API_KEY", creds.api_key.c_str(), 1);
#endif
            }
            break;
            
        case AuthType::None:
        default:
            // No configuration
            break;
    }
}

} // namespace ida_chat
