/**
 * @file claude_client.hpp
 * @brief Claude API client implementation.
 */

#pragma once

#include <ida_chat/core/types.hpp>
#include <ida_chat/api/http_client.hpp>
#include <ida_chat/api/claude_types.hpp>

#include <memory>
#include <functional>
#include <atomic>

namespace ida_chat {

/**
 * @brief Callback for streaming events.
 */
using StreamEventCallback = std::function<void(const StreamEvent& event)>;

/**
 * @brief Claude API client.
 * 
 * Handles authentication and communication with Claude's API.
 */
class ClaudeClient {
public:
    /**
     * @brief Create a client with explicit credentials.
     */
    explicit ClaudeClient(const AuthCredentials& credentials);
    
    /**
     * @brief Create a client using environment variables or system auth.
     */
    ClaudeClient();
    
    ~ClaudeClient();
    
    // Non-copyable
    ClaudeClient(const ClaudeClient&) = delete;
    ClaudeClient& operator=(const ClaudeClient&) = delete;
    
    /**
     * @brief Test the connection to Claude API.
     * @return pair of (success, message)
     */
    [[nodiscard]] std::pair<bool, std::string> test_connection();
    
    /**
     * @brief Send a message and get a complete response.
     */
    [[nodiscard]] std::optional<CreateMessageResponse> send_message(
        const CreateMessageRequest& request);
    
    /**
     * @brief Send a message with streaming response.
     * @param request The message request
     * @param callback Called for each streaming event
     * @return Final response (assembled from stream)
     */
    [[nodiscard]] std::optional<CreateMessageResponse> send_message_streaming(
        const CreateMessageRequest& request,
        StreamEventCallback callback);
    
    /**
     * @brief Cancel any ongoing request.
     */
    void cancel();
    
    /**
     * @brief Check if the client is configured and ready.
     */
    [[nodiscard]] bool is_configured() const noexcept;
    
    /**
     * @brief Get the current model name.
     */
    [[nodiscard]] std::string get_model() const;
    
    /**
     * @brief Set the model to use.
     */
    void set_model(const std::string& model);
    
    /**
     * @brief Get accumulated token usage.
     */
    [[nodiscard]] TokenUsage get_total_usage() const;
    
    /**
     * @brief Reset token usage counter.
     */
    void reset_usage();
    
    /**
     * @brief Estimate cost based on token usage.
     * @return Estimated cost in USD
     */
    [[nodiscard]] double estimate_cost() const;
    
    /**
     * @brief Get the idascript tool definition.
     */
    [[nodiscard]] static ToolDefinition get_idascript_tool();
    
    /**
     * @brief Get all available tool definitions.
     */
    [[nodiscard]] static std::vector<ToolDefinition> get_default_tools();

private:
    struct Impl;
    std::unique_ptr<Impl> impl_;
};

/**
 * @brief Create a ClaudeClient from environment variables.
 * 
 * Checks:
 * 1. ANTHROPIC_API_KEY
 * 2. CLAUDE_API_KEY
 * 3. System Claude Code authentication
 */
[[nodiscard]] std::unique_ptr<ClaudeClient> create_client_from_env();

} // namespace ida_chat
