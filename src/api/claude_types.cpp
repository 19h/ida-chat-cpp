/**
 * @file claude_types.cpp
 * @brief JSON serialization for Claude API types.
 */

#include <ida_chat/api/claude_types.hpp>

namespace ida_chat {

// ============================================================================
// JSON Serialization - To JSON
// ============================================================================

void to_json(nlohmann::json& j, const TextContent& c) {
    j = {
        {"type", "text"},
        {"text", c.text}
    };
}

void to_json(nlohmann::json& j, const ToolUseContent& c) {
    j = {
        {"type", "tool_use"},
        {"id", c.id},
        {"name", c.name},
        {"input", c.input}
    };
}

void to_json(nlohmann::json& j, const ToolResultContent& c) {
    j = {
        {"type", "tool_result"},
        {"tool_use_id", c.tool_use_id},
        {"content", c.content}
    };
    if (c.is_error) {
        j["is_error"] = true;
    }
}

void to_json(nlohmann::json& j, const ContentBlock& c) {
    std::visit([&j](const auto& content) {
        if constexpr (std::is_same_v<std::decay_t<decltype(content)>, TextContent>) {
            to_json(j, content);
        } else if constexpr (std::is_same_v<std::decay_t<decltype(content)>, ToolUseContent>) {
            to_json(j, content);
        } else if constexpr (std::is_same_v<std::decay_t<decltype(content)>, ToolResultContent>) {
            to_json(j, content);
        } else if constexpr (std::is_same_v<std::decay_t<decltype(content)>, ThinkingContent>) {
            j = {
                {"type", "thinking"},
                {"thinking", content.thinking}
            };
        }
    }, c);
}

void to_json(nlohmann::json& j, const ClaudeMessage& m) {
    j = {
        {"role", message_role_str(m.role)},
        {"content", nlohmann::json::array()}
    };
    for (const auto& block : m.content) {
        nlohmann::json block_json;
        to_json(block_json, block);
        j["content"].push_back(block_json);
    }
}

void to_json(nlohmann::json& j, const ToolDefinition& t) {
    j = {
        {"name", t.name},
        {"description", t.description},
        {"input_schema", t.input_schema.schema}
    };
}

void to_json(nlohmann::json& j, const CreateMessageRequest& r) {
    j = {
        {"model", r.model},
        {"messages", nlohmann::json::array()},
        {"max_tokens", r.max_tokens}
    };
    
    for (const auto& msg : r.messages) {
        nlohmann::json msg_json;
        to_json(msg_json, msg);
        j["messages"].push_back(msg_json);
    }
    
    if (!r.system.empty()) {
        j["system"] = r.system;
    }
    
    if (!r.tools.empty()) {
        j["tools"] = nlohmann::json::array();
        for (const auto& tool : r.tools) {
            nlohmann::json tool_json;
            to_json(tool_json, tool);
            j["tools"].push_back(tool_json);
        }
    }
    
    if (r.temperature.has_value()) {
        j["temperature"] = r.temperature.value();
    }
    
    if (r.stream) {
        j["stream"] = true;
    }
    
    if (r.thinking.has_value() && r.thinking->enabled) {
        j["thinking"] = {
            {"type", "enabled"},
            {"budget_tokens", r.thinking->budget_tokens}
        };
    }
}

// ============================================================================
// JSON Serialization - From JSON
// ============================================================================

void from_json(const nlohmann::json& j, TextContent& c) {
    c.text = j.value("text", "");
}

void from_json(const nlohmann::json& j, ToolUseContent& c) {
    c.id = j.value("id", "");
    c.name = j.value("name", "");
    if (j.contains("input")) {
        c.input = j["input"];
    }
}

void from_json(const nlohmann::json& j, ContentBlock& c) {
    std::string type = j.value("type", "");
    
    if (type == "text") {
        TextContent text;
        from_json(j, text);
        c = text;
    } else if (type == "tool_use") {
        ToolUseContent tool;
        from_json(j, tool);
        c = tool;
    } else if (type == "tool_result") {
        ToolResultContent result;
        result.tool_use_id = j.value("tool_use_id", "");
        result.content = j.value("content", "");
        result.is_error = j.value("is_error", false);
        c = result;
    } else if (type == "thinking") {
        ThinkingContent thinking;
        thinking.thinking = j.value("thinking", "");
        c = thinking;
    }
}

void from_json(const nlohmann::json& j, TokenUsage& u) {
    u.input_tokens = j.value("input_tokens", 0);
    u.output_tokens = j.value("output_tokens", 0);
    u.cache_read_tokens = j.value("cache_read_input_tokens", 0);
    u.cache_creation_tokens = j.value("cache_creation_input_tokens", 0);
}

void from_json(const nlohmann::json& j, CreateMessageResponse& r) {
    r.id = j.value("id", "");
    r.model = j.value("model", "");
    
    if (j.contains("stop_reason") && !j["stop_reason"].is_null()) {
        r.stop_reason = stop_reason_from_str(j["stop_reason"].get<std::string>());
    }
    
    if (j.contains("content") && j["content"].is_array()) {
        for (const auto& block_json : j["content"]) {
            ContentBlock block;
            from_json(block_json, block);
            r.content.push_back(std::move(block));
        }
    }
    
    if (j.contains("usage")) {
        from_json(j["usage"], r.usage);
    }
}

void from_json(const nlohmann::json& j, StreamEvent& e) {
    std::string type_str = j.value("type", "");
    e.type = stream_event_type_from_str(type_str);
    
    switch (e.type) {
        case StreamEventType::MessageStart:
            if (j.contains("message")) {
                CreateMessageResponse msg;
                from_json(j["message"], msg);
                e.message = std::move(msg);
            }
            break;
            
        case StreamEventType::ContentBlockStart:
            e.content_block_index = j.value("index", 0);
            if (j.contains("content_block")) {
                ContentBlock block;
                from_json(j["content_block"], block);
                e.content_block = std::move(block);
            }
            break;
            
        case StreamEventType::ContentBlockDelta:
            e.content_block_index = j.value("index", 0);
            if (j.contains("delta")) {
                ContentBlockDelta delta;
                delta.index = e.content_block_index;
                delta.type = j["delta"].value("type", "");
                
                if (delta.type == "text_delta") {
                    delta.text = j["delta"].value("text", "");
                } else if (delta.type == "input_json_delta") {
                    delta.partial_json = j["delta"].value("partial_json", "");
                } else if (delta.type == "thinking_delta") {
                    delta.thinking = j["delta"].value("thinking", "");
                }
                
                e.delta = std::move(delta);
            }
            break;
            
        case StreamEventType::ContentBlockStop:
            e.content_block_index = j.value("index", 0);
            break;
            
        case StreamEventType::MessageDelta:
            if (j.contains("delta")) {
                if (j["delta"].contains("stop_reason")) {
                    e.stop_reason = stop_reason_from_str(j["delta"]["stop_reason"].get<std::string>());
                }
            }
            if (j.contains("usage")) {
                TokenUsage usage;
                from_json(j["usage"], usage);
                e.usage = std::move(usage);
            }
            break;
            
        case StreamEventType::Error:
            if (j.contains("error")) {
                e.error = j["error"].value("message", "Unknown error");
            }
            break;
            
        default:
            break;
    }
}

} // namespace ida_chat
