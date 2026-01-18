/**
 * @file chat_callback.cpp
 * @brief Chat callback implementations.
 */

#include <ida_chat/core/chat_callback.hpp>

namespace ida_chat {

// ============================================================================
// CollectorCallback Implementation
// ============================================================================

void CollectorCallback::on_turn_start(int turn, int /*max_turns*/) {
    turns_ = turn;
}

void CollectorCallback::on_thinking() {
    // No-op for collector
}

void CollectorCallback::on_thinking_done() {
    // No-op for collector
}

void CollectorCallback::on_tool_use(const std::string& /*tool_name*/, const std::string& /*details*/) {
    // No-op for collector - could add tool use tracking if needed
}

void CollectorCallback::on_text(const std::string& text) {
    text_ += text;
}

void CollectorCallback::on_script_code(const std::string& /*code*/) {
    // No-op for collector - could add script tracking if needed
}

void CollectorCallback::on_script_output(const std::string& output) {
    if (!script_outputs_.empty()) {
        script_outputs_ += "\n---\n";
    }
    script_outputs_ += output;
}

void CollectorCallback::on_error(const std::string& error) {
    if (!errors_.empty()) {
        errors_ += "\n";
    }
    errors_ += error;
}

void CollectorCallback::on_result(int num_turns, std::optional<double> cost) {
    turns_ = num_turns;
    cost_ = cost;
}

void CollectorCallback::clear() {
    text_.clear();
    errors_.clear();
    script_outputs_.clear();
    turns_ = 0;
    cost_.reset();
}

} // namespace ida_chat
