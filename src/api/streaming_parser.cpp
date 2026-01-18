/**
 * @file streaming_parser.cpp
 * @brief SSE stream parser implementation.
 */

#include <ida_chat/api/streaming_parser.hpp>
#include <ida_chat/core/types.hpp>  // For trim()

#include <sstream>
#include <regex>

namespace ida_chat {

// ============================================================================
// StreamingParser Implementation
// ============================================================================

struct StreamingParser::Impl {
    EventCallback callback;
    std::string buffer;
    std::optional<CreateMessageResponse> response;
    std::vector<ContentBlock> content_blocks;
    std::vector<std::string> partial_jsons;  // For accumulating tool input JSON
    bool complete = false;
    bool has_error = false;
    std::string error_message;
    
    explicit Impl(EventCallback cb) : callback(std::move(cb)) {}
    
    void process_line(const std::string& line) {
        // Parse SSE format
        if (line.find("event:") == 0) {
            // Event type line - we use the type from the data
            return;
        }
        
        if (line.find("data:") == 0) {
            std::string data = line.substr(5);
            // Trim leading whitespace
            while (!data.empty() && (data.front() == ' ' || data.front() == '\t')) {
                data.erase(0, 1);
            }
            
            if (data.empty() || data == "[DONE]") {
                return;
            }
            
            try {
                auto json = nlohmann::json::parse(data);
                StreamEvent event;
                from_json(json, event);
                
                // Update internal state
                process_event(event);
                
                // Call callback
                if (callback) {
                    callback(event);
                }
            } catch (const std::exception& e) {
                // JSON parse error - might be partial data
            }
        }
    }
    
    void process_event(const StreamEvent& event) {
        switch (event.type) {
            case StreamEventType::MessageStart:
                if (event.message.has_value()) {
                    response = event.message;
                    response->content.clear();  // Clear, we'll rebuild from deltas
                }
                break;
                
            case StreamEventType::ContentBlockStart:
                // Ensure we have space for this block
                while (content_blocks.size() <= static_cast<size_t>(event.content_block_index)) {
                    content_blocks.push_back(TextContent{""});
                    partial_jsons.push_back("");
                }
                if (event.content_block.has_value()) {
                    content_blocks[event.content_block_index] = event.content_block.value();
                }
                break;
                
            case StreamEventType::ContentBlockDelta:
                if (event.delta.has_value()) {
                    size_t idx = event.content_block_index;
                    if (idx < content_blocks.size()) {
                        if (event.delta->type == "text_delta") {
                            if (auto* text = std::get_if<TextContent>(&content_blocks[idx])) {
                                text->text += event.delta->text;
                            }
                        } else if (event.delta->type == "input_json_delta") {
                            partial_jsons[idx] += event.delta->partial_json;
                        } else if (event.delta->type == "thinking_delta") {
                            if (auto* thinking = std::get_if<ThinkingContent>(&content_blocks[idx])) {
                                thinking->thinking += event.delta->thinking;
                            }
                        }
                    }
                }
                break;
                
            case StreamEventType::ContentBlockStop:
                // Finalize tool use if we have accumulated JSON
                {
                    size_t idx = event.content_block_index;
                    if (idx < content_blocks.size() && idx < partial_jsons.size()) {
                        if (auto* tool = std::get_if<ToolUseContent>(&content_blocks[idx])) {
                            if (!partial_jsons[idx].empty()) {
                                try {
                                    tool->input = nlohmann::json::parse(partial_jsons[idx]);
                                } catch (...) {
                                    // Keep partial JSON as string if parse fails
                                }
                            }
                        }
                    }
                }
                break;
                
            case StreamEventType::MessageDelta:
                if (event.stop_reason.has_value() && response.has_value()) {
                    response->stop_reason = event.stop_reason;
                }
                if (event.usage.has_value() && response.has_value()) {
                    response->usage = event.usage.value();
                }
                break;
                
            case StreamEventType::MessageStop:
                // Finalize response
                if (response.has_value()) {
                    response->content = content_blocks;
                }
                complete = true;
                break;
                
            case StreamEventType::Error:
                has_error = true;
                error_message = event.error.value_or("Unknown streaming error");
                break;
                
            default:
                break;
        }
    }
};

StreamingParser::StreamingParser(EventCallback callback)
    : impl_(std::make_unique<Impl>(std::move(callback))) {}

StreamingParser::~StreamingParser() = default;

void StreamingParser::feed(const std::string& data) {
    impl_->buffer += data;
    
    // Process complete lines
    size_t pos = 0;
    while ((pos = impl_->buffer.find('\n')) != std::string::npos) {
        std::string line = impl_->buffer.substr(0, pos);
        impl_->buffer.erase(0, pos + 1);
        
        // Remove trailing CR if present
        if (!line.empty() && line.back() == '\r') {
            line.pop_back();
        }
        
        if (!line.empty()) {
            impl_->process_line(line);
        }
    }
}

void StreamingParser::finish() {
    // Process any remaining data
    if (!impl_->buffer.empty()) {
        impl_->process_line(impl_->buffer);
        impl_->buffer.clear();
    }
}

void StreamingParser::reset() {
    impl_->buffer.clear();
    impl_->response.reset();
    impl_->content_blocks.clear();
    impl_->partial_jsons.clear();
    impl_->complete = false;
    impl_->has_error = false;
    impl_->error_message.clear();
}

std::optional<CreateMessageResponse> StreamingParser::get_response() const {
    return impl_->response;
}

bool StreamingParser::is_complete() const noexcept {
    return impl_->complete;
}

bool StreamingParser::has_error() const noexcept {
    return impl_->has_error;
}

std::string StreamingParser::get_error() const {
    return impl_->error_message;
}

// ============================================================================
// Script Block Extraction
// ============================================================================

std::vector<ScriptBlock> extract_idascript_blocks(const std::string& text) {
    std::vector<ScriptBlock> blocks;
    
    // Regex to match <idascript>...</idascript>
    static const std::regex script_regex(
        R"(<idascript>([\s\S]*?)</idascript>)",
        std::regex::ECMAScript);
    
    std::string remaining = text;
    std::smatch match;
    size_t last_end = 0;
    
    std::string::const_iterator search_start = text.cbegin();
    while (std::regex_search(search_start, text.cend(), match, script_regex)) {
        ScriptBlock block;
        
        // Get position of match
        size_t match_start = match.position(0) + (search_start - text.cbegin());
        
        // Text before this script block
        block.preceding_text = text.substr(last_end, match_start - last_end);
        
        // Script code (group 1) - trim whitespace like Python's .strip()
        block.code = trim(match[1].str());
        
        blocks.push_back(std::move(block));
        
        last_end = match_start + match.length(0);
        search_start = text.cbegin() + last_end;
    }
    
    // If there are blocks and there's text after the last one, add it
    if (!blocks.empty()) {
        // Remaining text after last script
        if (last_end < text.size()) {
            ScriptBlock final_block;
            final_block.preceding_text = text.substr(last_end);
            blocks.push_back(std::move(final_block));
        }
    }
    
    return blocks;
}

bool has_idascript_blocks(const std::string& text) {
    static const std::regex script_regex(R"(<idascript>)", std::regex::ECMAScript);
    return std::regex_search(text, script_regex);
}

std::string strip_idascript_blocks(const std::string& text) {
    static const std::regex script_regex(
        R"(<idascript>[\s\S]*?</idascript>)",
        std::regex::ECMAScript);
    
    return std::regex_replace(text, script_regex, "");
}

} // namespace ida_chat
