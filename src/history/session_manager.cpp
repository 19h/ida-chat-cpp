/**
 * @file session_manager.cpp
 * @brief Session management for message history.
 */

#include <ida_chat/history/session_manager.hpp>

#include <fstream>
#include <filesystem>
#include <map>

namespace ida_chat {

namespace fs = std::filesystem;

// ============================================================================
// Implementation
// ============================================================================

struct SessionManager::Impl {
    std::string sessions_dir;
    std::map<std::string, std::string> database_to_session;  // Cache
    
    Impl() {
        sessions_dir = get_sessions_directory();
        (void)ensure_directory_exists(sessions_dir);
    }
    
    std::string encode_database_path(const std::string& path) const {
        return base64_url_encode(path);
    }
    
    std::string decode_database_path(const std::string& encoded) const {
        return base64_url_decode(encoded);
    }
};

// ============================================================================
// SessionManager
// ============================================================================

SessionManager::SessionManager()
    : impl_(std::make_unique<Impl>()) {}

SessionManager::~SessionManager() = default;

std::string SessionManager::get_or_create_session(const std::string& database_path) {
    // Check cache first
    auto it = impl_->database_to_session.find(database_path);
    if (it != impl_->database_to_session.end()) {
        return it->second;
    }
    
    // Look for existing sessions
    auto sessions = list_sessions_for_database(database_path);
    if (!sessions.empty()) {
        // Return most recent
        impl_->database_to_session[database_path] = sessions.front().id;
        return sessions.front().id;
    }
    
    // Create new session
    std::string session_id = create_session(database_path);
    impl_->database_to_session[database_path] = session_id;
    return session_id;
}

std::string SessionManager::get_session_path(const std::string& session_id) const {
    return impl_->sessions_dir + "/" + session_id + ".jsonl";
}

std::vector<SessionInfo> SessionManager::list_sessions() const {
    std::vector<SessionInfo> sessions;
    
    try {
        for (const auto& entry : fs::directory_iterator(impl_->sessions_dir)) {
            if (entry.path().extension() == ".jsonl") {
                SessionInfo info;
                info.id = entry.path().stem().string();
                info.path = entry.path().string();
                
                // Read first line to get metadata
                std::ifstream file(entry.path());
                if (file) {
                    std::string first_line;
                    if (std::getline(file, first_line)) {
                        // Parse JSON to get metadata
                        // For now, just count lines
                        info.message_count = 1;
                        while (std::getline(file, first_line)) {
                            info.message_count++;
                        }
                    }
                }
                
                sessions.push_back(info);
            }
        }
    } catch (const std::exception&) {
        // Directory might not exist
    }
    
    return sessions;
}

std::vector<SessionInfo> SessionManager::list_sessions_for_database(const std::string& database_path) const {
    std::vector<SessionInfo> result;
    
    // Encode the database path to find matching sessions
    std::string encoded = impl_->encode_database_path(database_path);
    
    auto all_sessions = list_sessions();
    for (const auto& session : all_sessions) {
        if (session.database_path == database_path) {
            result.push_back(session);
        }
    }
    
    return result;
}

std::optional<SessionInfo> SessionManager::get_session_info(const std::string& session_id) const {
    std::string path = get_session_path(session_id);
    
    if (!file_exists(path)) {
        return std::nullopt;
    }
    
    SessionInfo info;
    info.id = session_id;
    info.path = path;
    
    // Read file to get metadata
    std::ifstream file(path);
    if (file) {
        info.message_count = 0;
        std::string line;
        while (std::getline(file, line)) {
            info.message_count++;
        }
    }
    
    return info;
}

bool SessionManager::delete_session(const std::string& session_id) {
    std::string path = get_session_path(session_id);
    
    try {
        return fs::remove(path);
    } catch (const std::exception&) {
        return false;
    }
}

std::string SessionManager::create_session(const std::string& database_path) {
    std::string session_id = generate_uuid();
    std::string path = get_session_path(session_id);
    
    // Create session file with metadata
    std::ofstream file(path);
    if (file) {
        // Write initial metadata line
        file << "{\"type\":\"session_start\",\"session_id\":\"" << session_id 
             << "\",\"database_path\":\"" << database_path
             << "\",\"timestamp\":\"" << get_iso_timestamp() << "\"}\n";
    }
    
    return session_id;
}

std::string SessionManager::sessions_directory() const {
    return impl_->sessions_dir;
}

} // namespace ida_chat
