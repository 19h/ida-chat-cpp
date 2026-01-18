/**
 * @file script_executor.hpp
 * @brief Script execution utilities for running Python in IDA.
 */

#pragma once

#include <ida_chat/core/types.hpp>

#include <string>
#include <functional>

namespace ida_chat {

/**
 * @brief Execute a Python script in IDA's Python environment.
 * 
 * This function must be called from the main thread in IDA.
 * For non-main thread execution, use execute_on_main_thread().
 * 
 * @param code Python code to execute
 * @return Execution result with captured output
 */
[[nodiscard]] ScriptResult execute_script_direct(const std::string& code);

/**
 * @brief Execute a script on IDA's main thread.
 * 
 * Uses ida_kernwin::execute_sync() to marshal execution to the main thread.
 * Safe to call from any thread.
 * 
 * @param code Python code to execute
 * @return Execution result with captured output
 */
[[nodiscard]] ScriptResult execute_script_on_main_thread(const std::string& code);

/**
 * @brief Create a script executor function that runs on the main thread.
 * 
 * Returns a function suitable for use with ChatCore that ensures
 * all script execution happens on IDA's main thread.
 * 
 * @return ScriptExecutorFn that executes scripts on the main thread
 */
[[nodiscard]] ScriptExecutorFn create_main_thread_executor();

/**
 * @brief Check if the current thread is IDA's main thread.
 */
[[nodiscard]] bool is_main_thread();

/**
 * @brief Wrapper for capturing Python output.
 * 
 * Temporarily redirects stdout/stderr to capture script output.
 */
class OutputCapture {
public:
    OutputCapture();
    ~OutputCapture();
    
    // Non-copyable
    OutputCapture(const OutputCapture&) = delete;
    OutputCapture& operator=(const OutputCapture&) = delete;
    
    /**
     * @brief Start capturing output.
     */
    void start();
    
    /**
     * @brief Stop capturing and get the captured output.
     */
    [[nodiscard]] std::string stop();
    
    /**
     * @brief Check if currently capturing.
     */
    [[nodiscard]] bool is_capturing() const noexcept;

private:
    struct Impl;
    std::unique_ptr<Impl> impl_;
};

/**
 * @brief RAII wrapper for output capture.
 */
class ScopedOutputCapture {
public:
    ScopedOutputCapture();
    ~ScopedOutputCapture();
    
    [[nodiscard]] std::string get_output();

private:
    OutputCapture capture_;
};

/**
 * @brief Format a script execution error message.
 */
[[nodiscard]] std::string format_script_error(
    const std::string& code,
    const std::string& error,
    int line_number = -1);

/**
 * @brief Validate Python code syntax without executing.
 * @return Error message if invalid, empty string if valid
 */
[[nodiscard]] std::string validate_script_syntax(const std::string& code);

} // namespace ida_chat
