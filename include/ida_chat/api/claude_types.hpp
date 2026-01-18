/**
 * @file claude_types.hpp
 * @brief Type definitions for Claude API communication.
 */

#pragma once

#include <ida_chat/core/types.hpp>

#include <string>
#include <vector>
#include <optional>
#include <variant>
#include <cstdint>

#include <ida_chat/common/json.hpp>

namespace ida_chat {

// ============================================================================
// Content Block Types
// ============================================================================

/**
 * @brief Text content block in a message.
 */
struct TextContent {
    std::string text;
    
    static constexpr const char* type() { return "text"; }
};

/**
 * @brief Tool use content block (Claude requesting to use a tool).
 */
struct ToolUseContent {
    std::string id;         ///< Unique tool use ID
    std::string name;       ///< Tool name (e.g., "idascript")
    nlohmann::json input;   ///< Tool input parameters
    
    static constexpr const char* type() { return "tool_use"; }
};

/**
 * @brief Tool result content block (response to a tool use).
 */
struct ToolResultContent {
    std::string tool_use_id;    ///< ID of the tool use this responds to
    std::string content;        ///< Result content
    bool is_error = false;      ///< Whether the result is an error
    
    static constexpr const char* type() { return "tool_result"; }
};

/**
 * @brief Thinking content block (Claude's reasoning, if enabled).
 */
struct ThinkingContent {
    std::string thinking;
    
    static constexpr const char* type() { return "thinking"; }
};

/**
 * @brief Union of all content block types.
 */
using ContentBlock = std::variant<TextContent, ToolUseContent, ToolResultContent, ThinkingContent>;

/**
 * @brief Get the type string for a content block.
 */
[[nodiscard]] inline std::string content_block_type(const ContentBlock& block) {
    return std::visit([](const auto& b) -> std::string { return b.type(); }, block);
}

// ============================================================================
// Message Types
// ============================================================================

/**
 * @brief Role in a conversation.
 */
enum class MessageRole : std::uint8_t {
    User,
    Assistant
};

[[nodiscard]] inline const char* message_role_str(MessageRole role) noexcept {
    return role == MessageRole::User ? "user" : "assistant";
}

[[nodiscard]] inline MessageRole message_role_from_str(const std::string& s) noexcept {
    return s == "user" ? MessageRole::User : MessageRole::Assistant;
}

/**
 * @brief A message in a Claude conversation.
 */
struct ClaudeMessage {
    MessageRole role;
    std::vector<ContentBlock> content;
    
    /**
     * @brief Create a simple text message.
     */
    static ClaudeMessage text(MessageRole role, const std::string& text) {
        ClaudeMessage msg;
        msg.role = role;
        msg.content.push_back(TextContent{text});
        return msg;
    }
    
    /**
     * @brief Create a user message with simple text.
     */
    static ClaudeMessage user(const std::string& text) {
        return ClaudeMessage::text(MessageRole::User, text);
    }
    
    /**
     * @brief Create a tool result message.
     */
    static ClaudeMessage tool_result(const std::string& tool_use_id, 
                                      const std::string& result,
                                      bool is_error = false) {
        ClaudeMessage msg;
        msg.role = MessageRole::User;
        msg.content.push_back(ToolResultContent{tool_use_id, result, is_error});
        return msg;
    }
    
    /**
     * @brief Get all text content concatenated.
     */
    [[nodiscard]] std::string get_text() const {
        std::string result;
        for (const auto& block : content) {
            if (auto* text = std::get_if<TextContent>(&block)) {
                if (!result.empty()) result += "\n";
                result += text->text;
            }
        }
        return result;
    }
    
    /**
     * @brief Check if message contains any tool use.
     */
    [[nodiscard]] bool has_tool_use() const {
        for (const auto& block : content) {
            if (std::holds_alternative<ToolUseContent>(block)) {
                return true;
            }
        }
        return false;
    }
    
    /**
     * @brief Get all tool uses in this message.
     */
    [[nodiscard]] std::vector<ToolUseContent> get_tool_uses() const {
        std::vector<ToolUseContent> uses;
        for (const auto& block : content) {
            if (auto* tool = std::get_if<ToolUseContent>(&block)) {
                uses.push_back(*tool);
            }
        }
        return uses;
    }
};

// ============================================================================
// Tool Definitions
// ============================================================================

/**
 * @brief JSON Schema for tool input parameters.
 */
struct ToolInputSchema {
    nlohmann::json schema;  ///< JSON Schema object
};

/**
 * @brief Definition of a tool that Claude can use.
 */
struct ToolDefinition {
    std::string name;
    std::string description;
    ToolInputSchema input_schema;
};

// ============================================================================
// API Request/Response Types
// ============================================================================

/**
 * @brief Request to create a message.
 */
struct CreateMessageRequest {
    std::string model = "claude-sonnet-4-20250514";
    std::vector<ClaudeMessage> messages;
    std::string system;
    std::vector<ToolDefinition> tools;
    int max_tokens = 8192;
    std::optional<double> temperature;
    bool stream = true;
    
    // Extended thinking support
    struct ThinkingConfig {
        bool enabled = false;
        int budget_tokens = 10000;
    };
    std::optional<ThinkingConfig> thinking;
};

/**
 * @brief Stop reason for message completion.
 */
enum class StopReason : std::uint8_t {
    EndTurn,        ///< Natural end of assistant turn
    MaxTokens,      ///< Hit max_tokens limit
    ToolUse,        ///< Stopped to use a tool
    StopSequence    ///< Hit a stop sequence
};

[[nodiscard]] inline const char* stop_reason_str(StopReason reason) noexcept {
    switch (reason) {
        case StopReason::EndTurn:      return "end_turn";
        case StopReason::MaxTokens:    return "max_tokens";
        case StopReason::ToolUse:      return "tool_use";
        case StopReason::StopSequence: return "stop_sequence";
        default:                        return "unknown";
    }
}

[[nodiscard]] inline StopReason stop_reason_from_str(const std::string& s) noexcept {
    if (s == "end_turn")       return StopReason::EndTurn;
    if (s == "max_tokens")     return StopReason::MaxTokens;
    if (s == "tool_use")       return StopReason::ToolUse;
    if (s == "stop_sequence")  return StopReason::StopSequence;
    return StopReason::EndTurn;
}

/**
 * @brief Response from message creation.
 */
struct CreateMessageResponse {
    std::string id;
    std::string model;
    std::optional<StopReason> stop_reason;
    std::vector<ContentBlock> content;
    TokenUsage usage;
    
    /**
     * @brief Get all text content.
     */
    [[nodiscard]] std::string get_text() const {
        std::string result;
        for (const auto& block : content) {
            if (auto* text = std::get_if<TextContent>(&block)) {
                if (!result.empty()) result += "\n";
                result += text->text;
            }
        }
        return result;
    }
    
    /**
     * @brief Check if response contains tool use.
     */
    [[nodiscard]] bool has_tool_use() const {
        for (const auto& block : content) {
            if (std::holds_alternative<ToolUseContent>(block)) {
                return true;
            }
        }
        return false;
    }
    
    /**
     * @brief Get all tool uses.
     */
    [[nodiscard]] std::vector<ToolUseContent> get_tool_uses() const {
        std::vector<ToolUseContent> uses;
        for (const auto& block : content) {
            if (auto* tool = std::get_if<ToolUseContent>(&block)) {
                uses.push_back(*tool);
            }
        }
        return uses;
    }
};

// ============================================================================
// Streaming Event Types
// ============================================================================

/**
 * @brief Type of streaming event.
 */
enum class StreamEventType : std::uint8_t {
    MessageStart,
    ContentBlockStart,
    ContentBlockDelta,
    ContentBlockStop,
    MessageDelta,
    MessageStop,
    Ping,
    Error
};

[[nodiscard]] inline StreamEventType stream_event_type_from_str(const std::string& s) noexcept {
    if (s == "message_start")        return StreamEventType::MessageStart;
    if (s == "content_block_start")  return StreamEventType::ContentBlockStart;
    if (s == "content_block_delta")  return StreamEventType::ContentBlockDelta;
    if (s == "content_block_stop")   return StreamEventType::ContentBlockStop;
    if (s == "message_delta")        return StreamEventType::MessageDelta;
    if (s == "message_stop")         return StreamEventType::MessageStop;
    if (s == "ping")                 return StreamEventType::Ping;
    return StreamEventType::Error;
}

/**
 * @brief Delta update for content block.
 */
struct ContentBlockDelta {
    int index = 0;
    std::string type;   // "text_delta", "input_json_delta", "thinking_delta"
    std::string text;   // For text_delta
    std::string partial_json;  // For input_json_delta
    std::string thinking;  // For thinking_delta
};

/**
 * @brief A streaming event from Claude API.
 */
struct StreamEvent {
    StreamEventType type;
    std::optional<CreateMessageResponse> message;     // For message_start
    std::optional<ContentBlock> content_block;        // For content_block_start
    std::optional<ContentBlockDelta> delta;           // For content_block_delta
    std::optional<StopReason> stop_reason;           // For message_delta
    std::optional<TokenUsage> usage;                 // For message_delta
    std::optional<std::string> error;                // For error events
    int content_block_index = 0;                     // Current content block index
};

// ============================================================================
// JSON Serialization
// ============================================================================

void to_json(nlohmann::json& j, const TextContent& c);
void to_json(nlohmann::json& j, const ToolUseContent& c);
void to_json(nlohmann::json& j, const ToolResultContent& c);
void to_json(nlohmann::json& j, const ContentBlock& c);
void to_json(nlohmann::json& j, const ClaudeMessage& m);
void to_json(nlohmann::json& j, const ToolDefinition& t);
void to_json(nlohmann::json& j, const CreateMessageRequest& r);

void from_json(const nlohmann::json& j, TextContent& c);
void from_json(const nlohmann::json& j, ToolUseContent& c);
void from_json(const nlohmann::json& j, ContentBlock& c);
void from_json(const nlohmann::json& j, TokenUsage& u);
void from_json(const nlohmann::json& j, CreateMessageResponse& r);
void from_json(const nlohmann::json& j, StreamEvent& e);

} // namespace ida_chat
