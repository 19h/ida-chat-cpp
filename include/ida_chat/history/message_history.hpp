/**
 * @file message_history.hpp
 * @brief Persistent message history storage in JSONL format.
 * 
 * Compatible with Claude Code's transcript format for HTML export.
 */

#pragma once

#include <ida_chat/core/types.hpp>
#include <ida_chat/api/claude_types.hpp>

#include <string>
#include <vector>
#include <optional>
#include <memory>

#include <ida_chat/common/json.hpp>

namespace ida_chat {

/**
 * @brief A message stored in history.
 */
struct HistoryMessage {
    std::string uuid;           ///< Unique message ID
    std::string parent_uuid;    ///< Parent message UUID (for threading)
    std::string type;           ///< Message type (user, assistant, tool_use, tool_result, etc.)
    std::int64_t timestamp;     ///< Unix timestamp in milliseconds
    nlohmann::json message;     ///< Full message content
    
    // Optional fields depending on type
    std::optional<std::string> model;
    std::optional<TokenUsage> usage;
    std::optional<std::string> tool_use_id;
    std::optional<bool> is_error;
};

/**
 * @brief Information about a chat session.
 */
struct SessionInfo {
    std::string session_id;     ///< Session UUID
    std::string first_message;  ///< First user message (truncated)
    std::int64_t timestamp;     ///< Session start timestamp
    int message_count = 0;      ///< Number of messages in session
    std::string file_path;      ///< Path to session file
};

/**
 * @brief Persistent message history for a binary.
 * 
 * Stores messages in JSONL format compatible with Claude Code's
 * transcript format, enabling HTML export via claude-code-transcripts.
 * 
 * Sessions are stored in:
 *   ~/.ida-chat/sessions/{base64_encoded_binary_path}/
 */
class MessageHistory {
public:
    static constexpr const char* BASE_DIR_NAME = ".ida-chat";
    static constexpr const char* SESSIONS_DIR_NAME = "sessions";
    static constexpr const char* VERSION = "ida-chat-1.0.0";
    
    /**
     * @brief Create history manager for a binary.
     * @param binary_path Full path to the binary being analyzed
     */
    explicit MessageHistory(const std::string& binary_path);
    ~MessageHistory();
    
    // Non-copyable
    MessageHistory(const MessageHistory&) = delete;
    MessageHistory& operator=(const MessageHistory&) = delete;
    
    /**
     * @brief Start a new session.
     * @return The new session ID (UUID)
     */
    [[nodiscard]] std::string start_new_session();
    
    /**
     * @brief Get the current session ID.
     * @return Session ID or empty if no session started
     */
    [[nodiscard]] std::string get_current_session_id() const;
    
    /**
     * @brief Append a user message.
     * @param content The user's message text
     * @return UUID of the appended message
     */
    std::string append_user_message(const std::string& content);
    
    /**
     * @brief Append an assistant message.
     * @param content The assistant's response text
     * @param model Model name used
     * @param usage Optional token usage
     * @return UUID of the appended message
     */
    std::string append_assistant_message(
        const std::string& content,
        const std::string& model = "claude-sonnet-4-20250514",
        const std::optional<TokenUsage>& usage = std::nullopt);
    
    /**
     * @brief Append a tool use record.
     * @param tool_name Name of the tool
     * @param tool_input Tool input parameters
     * @param tool_use_id Optional tool use ID (generated if not provided)
     * @return UUID of the appended message
     */
    std::string append_tool_use(
        const std::string& tool_name,
        const nlohmann::json& tool_input,
        const std::string& tool_use_id = "");
    
    /**
     * @brief Append a tool result record.
     * @param tool_use_id ID of the tool use this responds to
     * @param result The tool result content
     * @param is_error Whether the result is an error
     * @return UUID of the appended message
     */
    std::string append_tool_result(
        const std::string& tool_use_id,
        const std::string& result,
        bool is_error = false);
    
    /**
     * @brief Append a thinking block.
     * @param thinking The thinking/reasoning text
     * @return UUID of the appended message
     */
    std::string append_thinking(const std::string& thinking);
    
    /**
     * @brief Append a system message.
     * @param content The system message content
     * @param level Message level (info, warning, error)
     * @param subtype Optional subtype
     * @return UUID of the appended message
     */
    std::string append_system_message(
        const std::string& content,
        const std::string& level = "info",
        const std::string& subtype = "");
    
    /**
     * @brief Append a script execution record (tool use + result).
     * @param code The Python code that was executed
     * @param output The output from execution
     * @param is_error Whether the execution resulted in an error
     * @return UUID of the tool result message
     */
    std::string append_script_execution(
        const std::string& code,
        const std::string& output,
        bool is_error = false);
    
    /**
     * @brief Load all messages from a session.
     * @param session_id The session UUID to load
     * @return List of messages
     */
    [[nodiscard]] std::vector<HistoryMessage> load_session(const std::string& session_id) const;
    
    /**
     * @brief List all sessions for the current binary.
     * @return List of session summaries
     */
    [[nodiscard]] std::vector<SessionInfo> list_sessions() const;
    
    /**
     * @brief Get all user messages from all sessions.
     * @return List of user message contents (oldest first)
     */
    [[nodiscard]] std::vector<std::string> get_all_user_messages() const;
    
    /**
     * @brief Get the path to the current session file.
     */
    [[nodiscard]] std::string get_session_file_path() const;
    
    /**
     * @brief Get the sessions directory for this binary.
     */
    [[nodiscard]] std::string get_sessions_directory() const;

private:
    struct Impl;
    std::unique_ptr<Impl> impl_;
};

/**
 * @brief Export a session to HTML.
 * @param session_file Path to the JSONL session file
 * @param output_path Path for the output HTML file
 */
void export_transcript_html(const std::string& session_file, const std::string& output_path);

} // namespace ida_chat
