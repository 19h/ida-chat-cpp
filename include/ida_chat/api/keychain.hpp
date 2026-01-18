/**
 * @file keychain.hpp
 * @brief macOS Keychain access for Claude Code credentials.
 */

#pragma once

#include <string>
#include <vector>
#include <optional>
#include <chrono>
#include <cstdint>

namespace ida_chat {

/**
 * @brief Credentials stored by Claude Code in the macOS keychain.
 */
struct ClaudeCodeCredentials {
    std::string access_token;       ///< OAuth access token (sk-ant-oat01-...)
    std::string refresh_token;      ///< OAuth refresh token
    int64_t expires_at = 0;         ///< Token expiration timestamp (ms since epoch)
    std::string subscription_type;  ///< "max", "pro", etc.
    std::vector<std::string> scopes;
    
    /**
     * @brief Check if the access token has expired.
     */
    [[nodiscard]] bool is_expired() const;
    
    /**
     * @brief Check if this is a Max/Pro subscription (uses different API).
     */
    [[nodiscard]] bool is_max_subscription() const;
};

/**
 * @brief Read Claude Code credentials from macOS Keychain.
 * 
 * Looks for the "Claude Code-credentials" entry in the keychain,
 * which contains OAuth tokens used by Claude Code CLI.
 * 
 * @return Credentials if found and valid, nullopt otherwise.
 */
[[nodiscard]] std::optional<ClaudeCodeCredentials> read_claude_code_credentials();

} // namespace ida_chat
