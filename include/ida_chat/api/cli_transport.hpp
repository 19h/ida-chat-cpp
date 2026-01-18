/**
 * @file cli_transport.hpp
 * @brief Transport layer using Claude Code CLI subprocess.
 * 
 * This transport spawns the `claude` CLI as a subprocess and communicates
 * via JSON over stdin/stdout, matching the Python SDK's approach.
 */

#pragma once

#include <string>
#include <vector>
#include <optional>
#include <functional>
#include <memory>
#include <atomic>

namespace ida_chat {

/**
 * @brief Options for CLI transport.
 */
struct CLITransportOptions {
    std::string cli_path;                  ///< Path to claude CLI (auto-detected if empty)
    std::string cwd;                       ///< Working directory
    std::string system_prompt;             ///< System prompt to append
    std::vector<std::string> allowed_tools;///< Tools to allow
    std::string permission_mode = "bypassPermissions"; ///< Permission mode
    int max_turns = 20;                    ///< Max agentic turns
    std::string model;                     ///< Model to use (empty = default)
};

/**
 * @brief Callback for receiving messages from CLI.
 * @param json_line A single JSON line from stdout
 * @return true to continue, false to stop
 */
using CLIMessageCallback = std::function<bool(const std::string& json_line)>;

/**
 * @brief Callback for stderr output.
 */
using CLIStderrCallback = std::function<void(const std::string& line)>;

/**
 * @brief Transport that communicates with Claude via the CLI subprocess.
 * 
 * Usage:
 * 1. Create transport with options
 * 2. Call connect() to spawn the CLI
 * 3. Call query() to send messages
 * 4. Use receive_messages() to get responses
 * 5. Call disconnect() when done
 */
class CLITransport {
public:
    explicit CLITransport(const CLITransportOptions& options = {});
    ~CLITransport();
    
    // Non-copyable
    CLITransport(const CLITransport&) = delete;
    CLITransport& operator=(const CLITransport&) = delete;
    
    /**
     * @brief Find the claude CLI binary.
     * @return Path to CLI or empty string if not found.
     */
    static std::string find_cli();
    
    /**
     * @brief Connect (spawn the CLI subprocess).
     * @return true on success
     */
    bool connect();
    
    /**
     * @brief Disconnect (terminate the CLI subprocess).
     */
    void disconnect();
    
    /**
     * @brief Check if connected.
     */
    [[nodiscard]] bool is_connected() const noexcept;
    
    /**
     * @brief Send a user message/query.
     * @param message The user's message
     * @param session_id Session ID (default: "default")
     * @return true if sent successfully
     */
    bool query(const std::string& message, const std::string& session_id = "default");
    
    /**
     * @brief Read messages from CLI until a result message or error.
     * @param callback Called for each JSON line received
     * @return true if completed successfully (got ResultMessage)
     */
    bool receive_messages(CLIMessageCallback callback);
    
    /**
     * @brief Send interrupt signal.
     */
    void interrupt();
    
    /**
     * @brief Set stderr callback.
     */
    void set_stderr_callback(CLIStderrCallback callback);
    
    /**
     * @brief Get the last error message.
     */
    [[nodiscard]] std::string get_last_error() const;

private:
    struct Impl;
    std::unique_ptr<Impl> impl_;
};

/**
 * @brief Test connection to Claude via CLI.
 * @param cli_path Path to CLI (empty for auto-detect)
 * @return (success, message) pair
 */
std::pair<bool, std::string> test_cli_connection(const std::string& cli_path = "");

} // namespace ida_chat
