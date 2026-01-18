/**
 * @file chat_callback.hpp
 * @brief Callback interface for chat output events.
 */

#pragma once

#include <ida_chat/core/types.hpp>

#include <string>
#include <optional>

namespace ida_chat {

/**
 * @brief Interface for handling chat output events.
 * 
 * Implementations handle the presentation layer - whether terminal output
 * (CLI) or Qt widgets (Plugin). This is the C++ equivalent of the Python
 * ChatCallback protocol.
 */
class ChatCallback {
public:
    virtual ~ChatCallback() = default;
    
    /**
     * @brief Called at the start of each agentic turn.
     * @param turn Current turn number (1-based)
     * @param max_turns Maximum number of turns allowed
     */
    virtual void on_turn_start(int turn, int max_turns) = 0;
    
    /**
     * @brief Called when the agent starts processing (thinking).
     */
    virtual void on_thinking() = 0;
    
    /**
     * @brief Called when the agent produces first output.
     */
    virtual void on_thinking_done() = 0;
    
    /**
     * @brief Called when the agent uses a tool.
     * @param tool_name Name of the tool being used
     * @param details Brief description of the tool use
     */
    virtual void on_tool_use(const std::string& tool_name, const std::string& details) = 0;
    
    /**
     * @brief Called when the agent outputs text (excluding script blocks).
     * @param text The text output
     */
    virtual void on_text(const std::string& text) = 0;
    
    /**
     * @brief Called with script code before execution.
     * @param code The Python code to be executed
     */
    virtual void on_script_code(const std::string& code) = 0;
    
    /**
     * @brief Called with the output of an executed script.
     * @param output The script output (stdout/stderr)
     */
    virtual void on_script_output(const std::string& output) = 0;
    
    /**
     * @brief Called when an error occurs.
     * @param error The error message
     */
    virtual void on_error(const std::string& error) = 0;
    
    /**
     * @brief Called when the agent finishes processing.
     * @param num_turns Number of turns taken
     * @param cost Estimated cost in USD (optional)
     */
    virtual void on_result(int num_turns, std::optional<double> cost) = 0;
};

/**
 * @brief No-op callback implementation for testing or silent operation.
 */
class NullCallback : public ChatCallback {
public:
    void on_turn_start(int, int) override {}
    void on_thinking() override {}
    void on_thinking_done() override {}
    void on_tool_use(const std::string&, const std::string&) override {}
    void on_text(const std::string&) override {}
    void on_script_code(const std::string&) override {}
    void on_script_output(const std::string&) override {}
    void on_error(const std::string&) override {}
    void on_result(int, std::optional<double>) override {}
};

/**
 * @brief Callback that collects all output into strings.
 */
class CollectorCallback : public ChatCallback {
public:
    void on_turn_start(int turn, int max_turns) override;
    void on_thinking() override;
    void on_thinking_done() override;
    void on_tool_use(const std::string& tool_name, const std::string& details) override;
    void on_text(const std::string& text) override;
    void on_script_code(const std::string& code) override;
    void on_script_output(const std::string& output) override;
    void on_error(const std::string& error) override;
    void on_result(int num_turns, std::optional<double> cost) override;
    
    // Access collected data
    [[nodiscard]] const std::string& get_text() const { return text_; }
    [[nodiscard]] const std::string& get_errors() const { return errors_; }
    [[nodiscard]] const std::string& get_script_outputs() const { return script_outputs_; }
    [[nodiscard]] int get_turns() const { return turns_; }
    [[nodiscard]] std::optional<double> get_cost() const { return cost_; }
    
    void clear();

private:
    std::string text_;
    std::string errors_;
    std::string script_outputs_;
    int turns_ = 0;
    std::optional<double> cost_;
};

} // namespace ida_chat
