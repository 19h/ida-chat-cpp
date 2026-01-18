/**
 * @file chat_core.hpp
 * @brief Core chat engine implementation.
 * 
 * This is the main backend for both CLI and Plugin modes, handling
 * Claude API communication, agentic loop processing, and script execution.
 */

#pragma once

#include <ida_chat/core/fwd.hpp>
#include <ida_chat/core/types.hpp>
#include <ida_chat/core/chat_callback.hpp>
#include <ida_chat/api/claude_client.hpp>
#include <ida_chat/api/claude_types.hpp>
#include <ida_chat/history/message_history.hpp>

#include <string>
#include <vector>
#include <memory>
#include <functional>
#include <atomic>
#include <optional>

namespace ida_chat {

/**
 * @brief Configuration options for the chat core.
 */
struct ChatCoreOptions {
    int max_turns = DEFAULT_MAX_TURNS;      ///< Maximum agentic turns
    bool verbose = false;                    ///< Enable verbose logging
    std::string model = "claude-sonnet-4-20250514";  ///< Model to use
    bool enable_thinking = false;            ///< Enable extended thinking
    int thinking_budget = 10000;             ///< Thinking token budget
};

/**
 * @brief Result of processing a message.
 */
struct ProcessResult {
    bool success = false;
    std::string response;           ///< Final response text
    int turns_used = 0;             ///< Number of turns taken
    std::optional<double> cost;     ///< Estimated cost if available
    std::string error;              ///< Error message if failed
    bool cancelled = false;         ///< Whether operation was cancelled
};

/**
 * @brief Core chat engine for IDA Chat.
 * 
 * Handles:
 * - Claude API communication (via ClaudeClient)
 * - Agentic loop with script execution
 * - Message history persistence
 * - Cancellation support
 */
class ChatCore {
public:
    /**
     * @brief Create a chat core instance.
     * @param callback Callback for output events
     * @param script_executor Function to execute Python scripts
     * @param history Message history (optional)
     * @param options Configuration options
     */
    ChatCore(ChatCallback& callback,
             ScriptExecutorFn script_executor,
             MessageHistory* history = nullptr,
             const ChatCoreOptions& options = {});
    
    ~ChatCore();
    
    // Non-copyable
    ChatCore(const ChatCore&) = delete;
    ChatCore& operator=(const ChatCore&) = delete;
    
    /**
     * @brief Connect to the Claude API.
     * @param credentials Authentication credentials
     * @return true if connection successful
     */
    bool connect(const AuthCredentials& credentials = {});
    
    /**
     * @brief Disconnect from the Claude API.
     */
    void disconnect();
    
    /**
     * @brief Check if connected and ready.
     */
    [[nodiscard]] bool is_connected() const noexcept;
    
    /**
     * @brief Process a user message through the agentic loop.
     * 
     * This implements the core agentic behavior:
     * 1. Send message to Claude
     * 2. If response contains <idascript> blocks, execute them
     * 3. Feed results back to Claude
     * 4. Repeat until no more scripts or max_turns reached
     * 
     * @param user_input The user's message
     * @return Processing result
     */
    [[nodiscard]] ProcessResult process_message(const std::string& user_input);
    
    /**
     * @brief Request cancellation of the current operation.
     */
    void request_cancel();
    
    /**
     * @brief Check if cancellation was requested.
     */
    [[nodiscard]] bool is_cancelled() const noexcept;
    
    /**
     * @brief Get the current chat state.
     */
    [[nodiscard]] ChatState get_state() const noexcept;
    
    /**
     * @brief Get total token usage for this session.
     */
    [[nodiscard]] TokenUsage get_total_usage() const;
    
    /**
     * @brief Set the system prompt.
     */
    void set_system_prompt(const std::string& prompt);
    
    /**
     * @brief Load system prompt from the project directory.
     * @param project_dir Path to project directory containing PROMPT.md, IDA.md, etc.
     * @param inside_ida Whether running inside IDA (adds IDA.md content)
     */
    void load_system_prompt(const std::string& project_dir, bool inside_ida = true);
    
    /**
     * @brief Clear conversation history (in memory).
     */
    void clear_conversation();
    
    /**
     * @brief Start a new session in the history.
     */
    void start_new_session();
    
    /**
     * @brief Get the conversation message count.
     */
    [[nodiscard]] int get_message_count() const;

private:
    struct Impl;
    std::unique_ptr<Impl> impl_;
};

/**
 * @brief Load the default system prompt from a project directory.
 * @param project_dir Path to project directory
 * @param inside_ida Whether running inside IDA
 * @return Combined system prompt text
 */
[[nodiscard]] std::string load_default_system_prompt(
    const std::string& project_dir,
    bool inside_ida = true);

/**
 * @brief Test connection to Claude API.
 * @param credentials Credentials to test
 * @return pair of (success, message)
 */
[[nodiscard]] std::pair<bool, std::string> test_claude_connection(
    const AuthCredentials& credentials = {});

} // namespace ida_chat
