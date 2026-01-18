/**
 * @file session_manager.hpp
 * @brief Session management for message history.
 */

#pragma once

#include <ida_chat/core/types.hpp>

#include <string>
#include <vector>
#include <optional>

namespace ida_chat {

/**
 * @brief Information about a saved session.
 */
struct SessionInfo {
    std::string id;             ///< Session UUID
    std::string path;           ///< Path to session file
    std::string database_path;  ///< Associated IDA database path
    std::string created_at;     ///< ISO timestamp of creation
    std::string updated_at;     ///< ISO timestamp of last update
    int message_count = 0;      ///< Number of messages in session
};

/**
 * @brief Manager for session files and persistence.
 */
class SessionManager {
public:
    SessionManager();
    ~SessionManager();
    
    /**
     * @brief Get or create session for a database.
     * @param database_path Path to the IDA database
     * @return Session ID
     */
    [[nodiscard]] std::string get_or_create_session(const std::string& database_path);
    
    /**
     * @brief Get session file path.
     * @param session_id Session UUID
     * @return Path to session JSONL file
     */
    [[nodiscard]] std::string get_session_path(const std::string& session_id) const;
    
    /**
     * @brief List all sessions.
     * @return Vector of session info
     */
    [[nodiscard]] std::vector<SessionInfo> list_sessions() const;
    
    /**
     * @brief List sessions for a specific database.
     * @param database_path Path to the IDA database
     * @return Vector of session info
     */
    [[nodiscard]] std::vector<SessionInfo> list_sessions_for_database(const std::string& database_path) const;
    
    /**
     * @brief Get session info by ID.
     * @param session_id Session UUID
     * @return Session info if found
     */
    [[nodiscard]] std::optional<SessionInfo> get_session_info(const std::string& session_id) const;
    
    /**
     * @brief Delete a session.
     * @param session_id Session UUID
     * @return true if deleted successfully
     */
    [[nodiscard]] bool delete_session(const std::string& session_id);
    
    /**
     * @brief Create a new session.
     * @param database_path Associated database path (optional)
     * @return New session ID
     */
    [[nodiscard]] std::string create_session(const std::string& database_path = "");
    
    /**
     * @brief Get the sessions directory.
     */
    [[nodiscard]] std::string sessions_directory() const;

private:
    struct Impl;
    std::unique_ptr<Impl> impl_;
};

} // namespace ida_chat
