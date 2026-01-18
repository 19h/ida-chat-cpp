/**
 * @file streaming_parser.hpp
 * @brief Parser for Claude's Server-Sent Events (SSE) streaming format.
 */

#pragma once

#include <ida_chat/api/claude_types.hpp>

#include <string>
#include <functional>
#include <optional>
#include <memory>
#include <vector>

namespace ida_chat {

/**
 * @brief Parser for SSE (Server-Sent Events) stream format.
 * 
 * Handles incremental parsing of:
 * - event: type
 * - data: json
 * 
 * And calls callback for each complete event.
 */
class StreamingParser {
public:
    using EventCallback = std::function<void(const StreamEvent& event)>;
    
    explicit StreamingParser(EventCallback callback);
    ~StreamingParser();
    
    /**
     * @brief Feed data chunk to the parser.
     * @param data Raw data from HTTP response
     * 
     * May call the callback multiple times if multiple events
     * are present in the data chunk.
     */
    void feed(const std::string& data);
    
    /**
     * @brief Signal end of stream.
     * Processes any remaining buffered data.
     */
    void finish();
    
    /**
     * @brief Reset parser state for new stream.
     */
    void reset();
    
    /**
     * @brief Get the accumulated response.
     */
    [[nodiscard]] std::optional<CreateMessageResponse> get_response() const;
    
    /**
     * @brief Check if stream has completed successfully.
     */
    [[nodiscard]] bool is_complete() const noexcept;
    
    /**
     * @brief Check if an error occurred.
     */
    [[nodiscard]] bool has_error() const noexcept;
    
    /**
     * @brief Get error message if any.
     */
    [[nodiscard]] std::string get_error() const;

private:
    struct Impl;
    std::unique_ptr<Impl> impl_;
};

/**
 * @brief Extract idascript blocks from text.
 * 
 * Parses text for <idascript>...</idascript> XML tags.
 * 
 * @param text Text potentially containing idascript blocks
 * @return Vector of (script_code, text_without_script) pairs
 */
struct ScriptBlock {
    std::string code;           ///< The script code
    std::string preceding_text; ///< Text before this script block
};

[[nodiscard]] std::vector<ScriptBlock> extract_idascript_blocks(const std::string& text);

/**
 * @brief Check if text contains any idascript blocks.
 */
[[nodiscard]] bool has_idascript_blocks(const std::string& text);

/**
 * @brief Strip idascript blocks from text, returning just the text portions.
 */
[[nodiscard]] std::string strip_idascript_blocks(const std::string& text);

} // namespace ida_chat
