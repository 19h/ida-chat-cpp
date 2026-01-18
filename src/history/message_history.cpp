/**
 * @file message_history.cpp
 * @brief Message history persistence implementation.
 */

#include <ida_chat/history/message_history.hpp>

#include <fstream>
#include <sstream>
#include <algorithm>

namespace ida_chat {

// ============================================================================
// Implementation
// ============================================================================

struct MessageHistory::Impl {
    std::string binary_path;
    std::string sessions_dir;
    std::string current_session_id;
    std::string current_session_file;
    std::string last_message_uuid;
    
    explicit Impl(const std::string& path) : binary_path(path) {
        // Create encoded path for directory name
        std::string encoded = base64_url_encode(path);
        // Use the free function from types.hpp (not the member function)
        sessions_dir = ::ida_chat::get_sessions_directory() + "/" + encoded;
        (void)ensure_directory_exists(sessions_dir);
    }
    
    std::string write_message(const nlohmann::json& msg_json) {
        if (current_session_file.empty()) {
            return "";
        }
        
        // Generate UUID
        std::string uuid = generate_uuid();
        
        // Build full message object
        nlohmann::json full_msg = msg_json;
        full_msg["uuid"] = uuid;
        full_msg["parentUuid"] = last_message_uuid;
        full_msg["timestamp"] = get_timestamp_ms();
        
        // Append to file
        std::string line = full_msg.dump() + "\n";
        (void)append_to_file(current_session_file, line);
        
        last_message_uuid = uuid;
        return uuid;
    }
};

// ============================================================================
// MessageHistory Implementation
// ============================================================================

MessageHistory::MessageHistory(const std::string& binary_path)
    : impl_(std::make_unique<Impl>(binary_path)) {}

MessageHistory::~MessageHistory() = default;

std::string MessageHistory::start_new_session() {
    impl_->current_session_id = generate_uuid();
    impl_->current_session_file = impl_->sessions_dir + "/" + impl_->current_session_id + ".jsonl";
    impl_->last_message_uuid = "";
    
    // Write initial summary message
    nlohmann::json summary = {
        {"type", "summary"},
        {"version", VERSION},
        {"sessionId", impl_->current_session_id},
        {"binaryPath", impl_->binary_path}
    };
    impl_->write_message(summary);
    
    return impl_->current_session_id;
}

std::string MessageHistory::get_current_session_id() const {
    return impl_->current_session_id;
}

std::string MessageHistory::append_user_message(const std::string& content) {
    nlohmann::json msg = {
        {"type", "user"},
        {"message", {
            {"role", "user"},
            {"content", content}
        }}
    };
    return impl_->write_message(msg);
}

std::string MessageHistory::append_assistant_message(
    const std::string& content,
    const std::string& model,
    const std::optional<TokenUsage>& usage) {
    
    nlohmann::json msg = {
        {"type", "assistant"},
        {"message", {
            {"role", "assistant"},
            {"content", content}
        }},
        {"model", model}
    };
    
    if (usage.has_value()) {
        msg["usage"] = {
            {"input_tokens", usage->input_tokens},
            {"output_tokens", usage->output_tokens}
        };
    }
    
    return impl_->write_message(msg);
}

std::string MessageHistory::append_tool_use(
    const std::string& tool_name,
    const nlohmann::json& tool_input,
    const std::string& tool_use_id) {
    
    std::string id = tool_use_id.empty() ? generate_uuid() : tool_use_id;
    
    nlohmann::json msg = {
        {"type", "tool_use"},
        {"toolUseId", id},
        {"toolName", tool_name},
        {"toolInput", tool_input}
    };
    
    return impl_->write_message(msg);
}

std::string MessageHistory::append_tool_result(
    const std::string& tool_use_id,
    const std::string& result,
    bool is_error) {
    
    nlohmann::json msg = {
        {"type", "tool_result"},
        {"toolUseId", tool_use_id},
        {"content", result},
        {"isError", is_error}
    };
    
    return impl_->write_message(msg);
}

std::string MessageHistory::append_thinking(const std::string& thinking) {
    nlohmann::json msg = {
        {"type", "thinking"},
        {"thinking", thinking}
    };
    return impl_->write_message(msg);
}

std::string MessageHistory::append_system_message(
    const std::string& content,
    const std::string& level,
    const std::string& subtype) {
    
    nlohmann::json msg = {
        {"type", "system"},
        {"content", content},
        {"level", level}
    };
    
    if (!subtype.empty()) {
        msg["subtype"] = subtype;
    }
    
    return impl_->write_message(msg);
}

std::string MessageHistory::append_script_execution(
    const std::string& code,
    const std::string& output,
    bool is_error) {
    
    std::string tool_id = generate_uuid();
    
    // Write tool use
    append_tool_use("idascript", {{"code", code}}, tool_id);
    
    // Write tool result
    return append_tool_result(tool_id, output, is_error);
}

std::vector<HistoryMessage> MessageHistory::load_session(const std::string& session_id) const {
    std::vector<HistoryMessage> messages;
    
    std::string file_path = impl_->sessions_dir + "/" + session_id + ".jsonl";
    std::ifstream file(file_path);
    if (!file) {
        return messages;
    }
    
    std::string line;
    while (std::getline(file, line)) {
        if (line.empty()) continue;
        
        try {
            auto json = nlohmann::json::parse(line);
            
            HistoryMessage msg;
            msg.uuid = json.value("uuid", "");
            msg.parent_uuid = json.value("parentUuid", "");
            msg.type = json.value("type", "");
            msg.timestamp = json.value("timestamp", 0);
            msg.message = json;
            
            if (json.contains("model")) {
                msg.model = json["model"].get<std::string>();
            }
            if (json.contains("toolUseId")) {
                msg.tool_use_id = json["toolUseId"].get<std::string>();
            }
            if (json.contains("isError")) {
                msg.is_error = json["isError"].get<bool>();
            }
            if (json.contains("usage")) {
                TokenUsage usage;
                usage.input_tokens = json["usage"].value("input_tokens", 0);
                usage.output_tokens = json["usage"].value("output_tokens", 0);
                msg.usage = usage;
            }
            
            messages.push_back(std::move(msg));
        } catch (...) {
            // Skip malformed lines
        }
    }
    
    return messages;
}

std::vector<SessionInfo> MessageHistory::list_sessions() const {
    std::vector<SessionInfo> sessions;
    
    auto files = list_files(impl_->sessions_dir, ".jsonl");
    
    for (const auto& file_path : files) {
        // Extract session ID from filename
        size_t slash = file_path.find_last_of("/\\");
        std::string filename = (slash != std::string::npos) ? file_path.substr(slash + 1) : file_path;
        
        // Remove .jsonl extension
        if (ends_with(filename, ".jsonl")) {
            filename = filename.substr(0, filename.size() - 6);
        }
        
        SessionInfo info;
        info.session_id = filename;
        info.file_path = file_path;
        
        // Load first few messages to get info
        std::ifstream file(file_path);
        if (file) {
            std::string line;
            int count = 0;
            while (std::getline(file, line)) {
                count++;
                if (count <= 3 && !line.empty()) {
                    try {
                        auto json = nlohmann::json::parse(line);
                        
                        if (count == 1) {
                            info.timestamp = json.value("timestamp", 0);
                        }
                        
                        if (json.value("type", "") == "user" && info.first_message.empty()) {
                            if (json.contains("message") && json["message"].contains("content")) {
                                info.first_message = json["message"]["content"].get<std::string>();
                                // Truncate
                                if (info.first_message.size() > 100) {
                                    info.first_message = info.first_message.substr(0, 97) + "...";
                                }
                            }
                        }
                    } catch (...) {}
                }
            }
            info.message_count = count;
        }
        
        sessions.push_back(std::move(info));
    }
    
    return sessions;
}

std::vector<std::string> MessageHistory::get_all_user_messages() const {
    std::vector<std::string> messages;
    
    auto sessions = list_sessions();
    
    // Sort by timestamp (oldest first)
    std::sort(sessions.begin(), sessions.end(), 
        [](const SessionInfo& a, const SessionInfo& b) {
            return a.timestamp < b.timestamp;
        });
    
    for (const auto& session : sessions) {
        auto session_msgs = load_session(session.session_id);
        for (const auto& msg : session_msgs) {
            if (msg.type == "user" && msg.message.contains("message")) {
                auto& m = msg.message["message"];
                if (m.contains("content") && m["content"].is_string()) {
                    messages.push_back(m["content"].get<std::string>());
                }
            }
        }
    }
    
    return messages;
}

std::string MessageHistory::get_session_file_path() const {
    return impl_->current_session_file;
}

std::string MessageHistory::get_sessions_directory() const {
    return impl_->sessions_dir;
}

// ============================================================================
// Export Function
// ============================================================================

void export_transcript_html(const std::string& session_file, const std::string& output_path) {
    // Simple HTML export (full implementation would use a proper template)
    std::ifstream file(session_file);
    if (!file) {
        return;
    }
    
    std::ostringstream html;
    html << R"(<!DOCTYPE html>
<html>
<head>
    <title>IDA Chat Transcript</title>
    <style>
        body { font-family: -apple-system, sans-serif; max-width: 800px; margin: 0 auto; padding: 20px; }
        .message { margin: 10px 0; padding: 10px; border-radius: 8px; }
        .user { background: #e3f2fd; }
        .assistant { background: #f5f5f5; }
        .tool { background: #fff3e0; font-family: monospace; font-size: 12px; }
        pre { background: #263238; color: #fff; padding: 10px; border-radius: 4px; overflow-x: auto; }
    </style>
</head>
<body>
<h1>IDA Chat Transcript</h1>
)";
    
    std::string line;
    while (std::getline(file, line)) {
        if (line.empty()) continue;
        
        try {
            auto json = nlohmann::json::parse(line);
            std::string type = json.value("type", "");
            
            if (type == "user") {
                std::string content = json["message"].value("content", "");
                html << "<div class='message user'><strong>User:</strong><br>" 
                     << html_escape(content) << "</div>\n";
            } else if (type == "assistant") {
                std::string content = json["message"].value("content", "");
                html << "<div class='message assistant'><strong>Assistant:</strong><br>" 
                     << html_escape(content) << "</div>\n";
            } else if (type == "tool_use") {
                std::string code = json["toolInput"].value("code", "");
                html << "<div class='message tool'><strong>Script:</strong><pre>" 
                     << html_escape(code) << "</pre></div>\n";
            } else if (type == "tool_result") {
                std::string content = json.value("content", "");
                bool is_error = json.value("isError", false);
                html << "<div class='message tool'><strong>Output" 
                     << (is_error ? " (Error)" : "") << ":</strong><pre>" 
                     << html_escape(content) << "</pre></div>\n";
            }
        } catch (...) {}
    }
    
    html << "</body></html>";
    
    (void)write_file(output_path, html.str());
}

} // namespace ida_chat
