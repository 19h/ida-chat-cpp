/**
 * @file types.hpp
 * @brief Core type definitions for ida_chat.
 */

#pragma once

#include <ida_chat/common/platform.hpp>

#include <string>
#include <vector>
#include <optional>
#include <variant>
#include <functional>
#include <cstdint>
#include <chrono>

namespace ida_chat {

// ============================================================================
// Message Types
// ============================================================================

/**
 * @brief Message type identifiers for UI differentiation.
 */
enum class MessageType : std::uint8_t {
    Text,       ///< Normal assistant text
    ToolUse,    ///< Tool invocation (muted, italic)
    Script,     ///< Script code (monospace, dark bg)
    Output,     ///< Script output (monospace, gray bg)
    Error,      ///< Error message (red accent)
    User,       ///< User message
    System,     ///< System notification
    Thinking    ///< Agent thinking/reasoning
};

/**
 * @brief Convert MessageType to string for display/logging.
 */
[[nodiscard]] inline const char* message_type_str(MessageType type) noexcept {
    switch (type) {
        case MessageType::Text:     return "text";
        case MessageType::ToolUse:  return "tool_use";
        case MessageType::Script:   return "script";
        case MessageType::Output:   return "output";
        case MessageType::Error:    return "error";
        case MessageType::User:     return "user";
        case MessageType::System:   return "system";
        case MessageType::Thinking: return "thinking";
        default:                    return "unknown";
    }
}

// ============================================================================
// Authentication Types
// ============================================================================

/**
 * @brief Authentication method for Claude API.
 */
enum class AuthType : std::uint8_t {
    None,       ///< Not configured
    System,     ///< Use system Claude Code authentication
    OAuth,      ///< OAuth token
    ApiKey      ///< Direct API key
};

/**
 * @brief Convert AuthType to string.
 */
[[nodiscard]] inline const char* auth_type_str(AuthType type) noexcept {
    switch (type) {
        case AuthType::None:   return "none";
        case AuthType::System: return "system";
        case AuthType::OAuth:  return "oauth";
        case AuthType::ApiKey: return "api_key";
        default:               return "unknown";
    }
}

/**
 * @brief Parse AuthType from string.
 */
[[nodiscard]] inline AuthType auth_type_from_str(const std::string& s) noexcept {
    if (s == "system")  return AuthType::System;
    if (s == "oauth")   return AuthType::OAuth;
    if (s == "api_key") return AuthType::ApiKey;
    return AuthType::None;
}

/**
 * @brief Authentication credentials.
 */
struct AuthCredentials {
    AuthType type = AuthType::None;
    std::string api_key;        ///< API key or OAuth token
    std::string api_base_url;   ///< Custom API base URL (optional)
    
    [[nodiscard]] bool is_configured() const noexcept {
        // For System auth, we might not have api_key yet (it will be read from env)
        // For OAuth/ApiKey, we need an actual key
        if (type == AuthType::None) return false;
        if (type == AuthType::System) return true;  // Will try env vars
        return !api_key.empty();
    }
    
    [[nodiscard]] bool has_api_key() const noexcept {
        return !api_key.empty();
    }
    
    [[nodiscard]] bool requires_key() const noexcept {
        return type == AuthType::OAuth || type == AuthType::ApiKey;
    }
};

// ============================================================================
// Script Execution Types
// ============================================================================

/**
 * @brief Result of executing a Python script in IDA.
 */
struct ScriptResult {
    bool success = false;       ///< Whether execution succeeded
    std::string output;         ///< Captured stdout/stderr
    std::string error;          ///< Error message if failed
    double execution_time_ms = 0.0;  ///< Execution duration
    
    [[nodiscard]] static ScriptResult success_result(std::string out) {
        return {true, std::move(out), {}, 0.0};
    }
    
    [[nodiscard]] static ScriptResult error_result(std::string err) {
        return {false, {}, std::move(err), 0.0};
    }
};

/**
 * @brief Script executor function signature.
 * Takes Python code, returns execution result.
 */
using ScriptExecutorFn = std::function<ScriptResult(const std::string& code)>;

// ============================================================================
// Token Usage Tracking
// ============================================================================

/**
 * @brief Token usage statistics for a conversation turn.
 */
struct TokenUsage {
    std::int64_t input_tokens = 0;
    std::int64_t output_tokens = 0;
    std::int64_t cache_read_tokens = 0;
    std::int64_t cache_creation_tokens = 0;
    
    [[nodiscard]] std::int64_t total_tokens() const noexcept {
        return input_tokens + output_tokens;
    }
    
    TokenUsage& operator+=(const TokenUsage& other) noexcept {
        input_tokens += other.input_tokens;
        output_tokens += other.output_tokens;
        cache_read_tokens += other.cache_read_tokens;
        cache_creation_tokens += other.cache_creation_tokens;
        return *this;
    }
};

// ============================================================================
// Chat State Types
// ============================================================================

/**
 * @brief Current state of the chat session.
 */
enum class ChatState : std::uint8_t {
    Disconnected,   ///< Not connected to Claude
    Connecting,     ///< Connection in progress
    Idle,           ///< Connected and ready
    Processing,     ///< Processing a message
    Cancelled       ///< Operation was cancelled
};

/**
 * @brief Convert ChatState to string.
 */
[[nodiscard]] inline const char* chat_state_str(ChatState state) noexcept {
    switch (state) {
        case ChatState::Disconnected: return "disconnected";
        case ChatState::Connecting:   return "connecting";
        case ChatState::Idle:         return "idle";
        case ChatState::Processing:   return "processing";
        case ChatState::Cancelled:    return "cancelled";
        default:                      return "unknown";
    }
}

// ============================================================================
// IDA-related Types
// ============================================================================

/**
 * @brief Information about the current IDA database.
 */
struct DatabaseInfo {
    std::string path;           ///< Full path to the input file
    std::string module_name;    ///< Module/binary name
    std::string architecture;   ///< Processor architecture (x86, arm, etc.)
    int bitness = 64;           ///< 32 or 64 bit
    bool is_open = false;       ///< Whether a database is open
    
    [[nodiscard]] std::string display_name() const {
        if (module_name.empty()) return "(no database)";
        return module_name;
    }
};

// ============================================================================
// Timestamp Utilities
// ============================================================================

/**
 * @brief Get current timestamp in ISO 8601 format.
 */
[[nodiscard]] std::string get_iso_timestamp();

/**
 * @brief Get current timestamp as milliseconds since epoch.
 */
[[nodiscard]] std::int64_t get_timestamp_ms();

// ============================================================================
// UUID Generation
// ============================================================================

/**
 * @brief Generate a random UUID v4.
 */
[[nodiscard]] std::string generate_uuid();

// ============================================================================
// Path Utilities
// ============================================================================

/**
 * @brief Get the user's home directory.
 */
[[nodiscard]] std::string get_home_directory();

/**
 * @brief Get the IDA Chat configuration directory (~/.ida-chat/).
 */
[[nodiscard]] std::string get_config_directory();

/**
 * @brief Get the sessions directory (~/.ida-chat/sessions/).
 */
[[nodiscard]] std::string get_sessions_directory();

/**
 * @brief Ensure a directory exists, creating it if necessary.
 * @return true if directory exists or was created successfully.
 */
[[nodiscard]] bool ensure_directory_exists(const std::string& path);

/**
 * @brief URL-safe Base64 encode a string (for path encoding).
 */
[[nodiscard]] std::string base64_url_encode(const std::string& input);

/**
 * @brief URL-safe Base64 decode a string.
 */
[[nodiscard]] std::string base64_url_decode(const std::string& input);

// ============================================================================
// String Utilities
// ============================================================================

/**
 * @brief Trim whitespace from both ends of a string.
 */
[[nodiscard]] std::string trim(const std::string& s);

/**
 * @brief Split a string by delimiter.
 */
[[nodiscard]] std::vector<std::string> split(const std::string& s, char delimiter);

/**
 * @brief Join strings with a delimiter.
 */
[[nodiscard]] std::string join(const std::vector<std::string>& parts, const std::string& delimiter);

/**
 * @brief Check if a string starts with a prefix.
 */
[[nodiscard]] inline bool starts_with(const std::string& s, const std::string& prefix) {
    return s.size() >= prefix.size() && s.compare(0, prefix.size(), prefix) == 0;
}

/**
 * @brief Check if a string ends with a suffix.
 */
[[nodiscard]] inline bool ends_with(const std::string& s, const std::string& suffix) {
    return s.size() >= suffix.size() && s.compare(s.size() - suffix.size(), suffix.size(), suffix) == 0;
}

/**
 * @brief Replace all occurrences of a substring.
 */
[[nodiscard]] std::string replace_all(const std::string& s, const std::string& from, const std::string& to);

/**
 * @brief Escape a string for use in HTML.
 */
[[nodiscard]] std::string html_escape(const std::string& s);

// ============================================================================
// File I/O Utilities
// ============================================================================

/**
 * @brief Read entire file contents as a string.
 */
[[nodiscard]] std::optional<std::string> read_file(const std::string& path);

/**
 * @brief Write string contents to a file.
 */
[[nodiscard]] bool write_file(const std::string& path, const std::string& contents);

/**
 * @brief Append string contents to a file.
 */
[[nodiscard]] bool append_to_file(const std::string& path, const std::string& contents);

/**
 * @brief Check if a file exists.
 */
[[nodiscard]] bool file_exists(const std::string& path);

/**
 * @brief List files in a directory matching a pattern.
 */
[[nodiscard]] std::vector<std::string> list_files(const std::string& directory, const std::string& extension = "");

} // namespace ida_chat
