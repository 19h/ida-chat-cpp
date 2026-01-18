/**
 * @file chat_core.cpp
 * @brief Core chat engine implementation.
 */

#include <ida_chat/core/chat_core.hpp>
#include <ida_chat/api/streaming_parser.hpp>
#include <ida_chat/api/cli_transport.hpp>

#include <fstream>
#include <sstream>
#include <cstdio>

#ifdef __APPLE__
#include <unistd.h>
#endif

// IDA headers for msg() debug output
#include <ida_chat/common/ida_begin.hpp>
#include <ida.hpp>
#include <kernwin.hpp>
#include <ida_chat/common/ida_end.hpp>

// Debug logging macro - outputs to IDA's output window
#define IDA_CHAT_DEBUG(fmt, ...) msg("[IDA Chat] " fmt "\n", ##__VA_ARGS__)

namespace ida_chat {

// ============================================================================
// Implementation
// ============================================================================

struct ChatCore::Impl {
    ChatCallback& callback;
    ScriptExecutorFn script_executor;
    MessageHistory* history;
    ChatCoreOptions options;
    
    std::unique_ptr<ClaudeClient> client;
    std::vector<ClaudeMessage> conversation;
    std::string system_prompt;
    
    std::atomic<bool> cancelled{false};
    std::atomic<ChatState> state{ChatState::Disconnected};
    TokenUsage total_usage;
    
    // CLI mode support
    bool use_cli_mode = false;
    std::string cli_path;
    
    Impl(ChatCallback& cb, ScriptExecutorFn exec, MessageHistory* hist, const ChatCoreOptions& opts)
        : callback(cb)
        , script_executor(std::move(exec))
        , history(hist)
        , options(opts) {}
    
    // Execute idascript and return output
    std::string execute_script(const std::string& code) {
        if (!script_executor) {
            return "Error: No script executor available";
        }
        
        callback.on_script_code(code);
        
        auto result = script_executor(code);
        
        if (result.success) {
            callback.on_script_output(result.output);
            
            // Log to history
            if (history) {
                history->append_script_execution(code, result.output, false);
            }
            
            return result.output;
        } else {
            std::string error_msg = "Error: " + result.error;
            callback.on_error(error_msg);
            
            // Log error to history
            if (history) {
                history->append_script_execution(code, error_msg, true);
            }
            
            return error_msg;
        }
    }
    
    // Process idascript blocks in response
    std::pair<std::vector<std::string>, std::vector<std::string>> 
    process_scripts(const std::string& text) {
        std::vector<std::string> scripts;
        std::vector<std::string> outputs;
        
        auto blocks = extract_idascript_blocks(text);
        for (const auto& block : blocks) {
            if (!block.code.empty()) {
                scripts.push_back(block.code);
                outputs.push_back(execute_script(block.code));
            }
        }
        
        return {scripts, outputs};
    }
    
    // Run CLI and parse response
    struct CLIResponse {
        std::string response_text;
        std::string error_text;
        bool got_response = false;
        double cost = 0.0;
        int num_turns = 1;
        std::string session_id;
    };
    
    // Run CLI call
    // - First call: no session_id, get one back from response
    // - Subsequent calls: use --resume <session_id> to continue
    CLIResponse run_cli_call(const std::string& message, const std::string& session_id, bool is_continue) {
        CLIResponse result;
        
        std::string pid_str = std::to_string(getpid());
        std::string tmp_script = "/tmp/ida_chat_run_" + pid_str + ".sh";
        std::string tmp_prompt = "/tmp/ida_chat_prompt_" + pid_str + ".txt";
        std::string tmp_message = "/tmp/ida_chat_message_" + pid_str + ".txt";
        
        // Write system prompt to temp file (only for first call)
        if (!is_continue && !system_prompt.empty()) {
            std::ofstream pf(tmp_prompt);
            if (pf) {
                pf << system_prompt;
                pf.close();
            }
        }
        
        // Write message to temp file
        {
            std::ofstream mf(tmp_message);
            if (mf) {
                mf << message;
                mf.close();
            }
        }
        
        // Build and write wrapper script
        {
            std::ofstream sf(tmp_script);
            if (sf) {
                sf << "#!/bin/bash\n";
                sf << "set -e\n";
                sf << "PROMPT_FILE='" << tmp_prompt << "'\n";
                sf << "MESSAGE_FILE='" << tmp_message << "'\n";
                sf << "CLI='" << cli_path << "'\n";
                sf << "\n";
                sf << "# Build command\n";
                sf << "CMD=(\"$CLI\" --print --output-format stream-json";
                sf << " --permission-mode bypassPermissions";
                
                // For continuation, use --resume with session ID
                if (is_continue && !session_id.empty()) {
                    sf << " --resume '" << session_id << "'";
                }
                
                sf << ")\n";
                sf << "\n";
                
                if (!is_continue) {
                    sf << "# Add system prompt if file exists and is non-empty\n";
                    sf << "if [[ -s \"$PROMPT_FILE\" ]]; then\n";
                    sf << "  PROMPT=$(cat \"$PROMPT_FILE\")\n";
                    sf << "  CMD+=(--append-system-prompt \"$PROMPT\")\n";
                    sf << "fi\n";
                    sf << "\n";
                }
                
                sf << "# Add message\n";
                sf << "MESSAGE=$(cat \"$MESSAGE_FILE\")\n";
                sf << "CMD+=(-- \"$MESSAGE\")\n";
                sf << "\n";
                sf << "# Execute\n";
                sf << "exec \"${CMD[@]}\" 2>&1\n";
                sf.close();
            }
        }
        
        // Make script executable and run it
        std::string cmd = "chmod +x '" + tmp_script + "' && '" + tmp_script + "'";
        
        IDA_CHAT_DEBUG("run_cli_call: is_continue=%d, session_id='%s'", is_continue, session_id.c_str());
        
        FILE* fp = popen(cmd.c_str(), "r");
        if (!fp) {
            result.error_text = "Failed to execute Claude CLI";
            return result;
        }
        
        char buffer[4096];
        std::string output;
        while (fgets(buffer, sizeof(buffer), fp)) {
            output += buffer;
        }
        pclose(fp);
        
        IDA_CHAT_DEBUG("run_cli_call: raw output length=%zu", output.size());
        
        // Parse JSON lines
        std::istringstream iss(output);
        std::string line;
        while (std::getline(iss, line)) {
            if (line.empty()) continue;
            
            try {
                auto json = nlohmann::json::parse(line);
                std::string type = json.value("type", "");
                
                if (type == "assistant") {
                    callback.on_thinking_done();
                    
                    if (json.contains("message") && json["message"].contains("content")) {
                        for (const auto& block : json["message"]["content"]) {
                            std::string block_type = block.value("type", "");
                            if (block_type == "text") {
                                std::string text = block.value("text", "");
                                result.response_text += text;
                                result.got_response = true;
                                
                                // Strip idascript blocks for display
                                std::string display_text = strip_idascript_blocks(text);
                                if (!display_text.empty()) {
                                    callback.on_text(display_text);
                                }
                            } else if (block_type == "tool_use") {
                                std::string tool_name = block.value("name", "");
                                callback.on_tool_use(tool_name, "");
                            }
                        }
                    }
                    
                    // Extract session ID if present
                    if (json.contains("session_id")) {
                        result.session_id = json["session_id"].get<std::string>();
                        IDA_CHAT_DEBUG("run_cli_call: got session_id='%s'", result.session_id.c_str());
                    }
                } else if (type == "result") {
                    if (json.value("is_error", false)) {
                        result.error_text = json.value("result", "Unknown error");
                    }
                    result.cost = json.value("total_cost_usd", 0.0);
                    result.num_turns = json.value("num_turns", 1);
                    
                    if (json.contains("session_id")) {
                        result.session_id = json["session_id"].get<std::string>();
                        IDA_CHAT_DEBUG("run_cli_call: got session_id from result='%s'", result.session_id.c_str());
                    }
                } else if (type == "system" && json.value("subtype", "") == "error") {
                    result.error_text = json["data"].value("message", "System error");
                }
            } catch (...) {
                // Not JSON - check for error messages
                if (line.find("Error:") != std::string::npos) {
                    result.error_text = line;
                    IDA_CHAT_DEBUG("run_cli_call: got error line='%s'", line.c_str());
                }
            }
        }
        
        return result;
    }
    
    // Process message using CLI transport with proper agent loop
    ProcessResult process_message_cli(const std::string& user_input) {
        ProcessResult result;
        
        state = ChatState::Processing;
        cancelled = false;
        
        IDA_CHAT_DEBUG("process_message_cli: user_input='%s'", user_input.c_str());
        IDA_CHAT_DEBUG("process_message_cli: cli_path='%s'", cli_path.c_str());
        IDA_CHAT_DEBUG("process_message_cli: system_prompt length=%zu", system_prompt.size());
        
        // Log user message
        if (history) {
            history->append_user_message(user_input);
        }
        
        // Session ID will be captured from first response
        std::string session_id;
        
        std::string full_response;
        double total_cost = 0.0;
        int total_turns = 0;
        
        // Initial call with user input
        std::string current_message = user_input;
        bool is_continue = false;
        
        for (int turn = 0; turn < options.max_turns && !cancelled; turn++) {
            callback.on_turn_start(turn + 1, options.max_turns);
            callback.on_thinking();
            
            IDA_CHAT_DEBUG("process_message_cli: turn %d, is_continue=%d, session_id='%s'", 
                          turn + 1, is_continue, session_id.c_str());
            
            auto cli_result = run_cli_call(current_message, session_id, is_continue);
            
            if (!cli_result.error_text.empty()) {
                result.error = cli_result.error_text;
                callback.on_error(cli_result.error_text);
                state = ChatState::Idle;
                return result;
            }
            
            if (!cli_result.got_response) {
                result.error = "No response from Claude";
                state = ChatState::Idle;
                return result;
            }
            
            // Capture session ID from first response for subsequent calls
            if (!cli_result.session_id.empty()) {
                session_id = cli_result.session_id;
                IDA_CHAT_DEBUG("process_message_cli: captured session_id='%s'", session_id.c_str());
            }
            
            full_response += cli_result.response_text;
            total_cost += cli_result.cost;
            total_turns++;
            
            // Check for idascript blocks
            if (has_idascript_blocks(cli_result.response_text)) {
                auto [scripts, outputs] = process_scripts(cli_result.response_text);
                
                if (!scripts.empty() && !outputs.empty()) {
                    // Need a session ID to continue
                    if (session_id.empty()) {
                        IDA_CHAT_DEBUG("process_message_cli: WARNING - no session_id, cannot continue");
                        break;
                    }
                    
                    // Build combined output to send back
                    std::string combined_output = "Script execution results:\n\n";
                    for (size_t i = 0; i < outputs.size(); ++i) {
                        combined_output += "```\n" + outputs[i] + "\n```\n\n";
                    }
                    
                    IDA_CHAT_DEBUG("process_message_cli: feeding back script output (%zu bytes)", combined_output.size());
                    
                    // Feed output back to Claude
                    current_message = combined_output;
                    is_continue = true;
                    continue;  // Continue the loop
                }
            }
            
            // No scripts (or scripts with no output), we're done
            break;
        }
        
        // Log to history
        if (history) {
            TokenUsage usage;
            history->append_assistant_message(full_response, options.model, usage);
        }
        
        result.success = true;
        result.response = strip_idascript_blocks(full_response);
        result.turns_used = total_turns;
        result.cost = total_cost;
        
        callback.on_result(total_turns, total_cost);
        state = ChatState::Idle;
        
        return result;
    }
};

// ============================================================================
// ChatCore Implementation
// ============================================================================

ChatCore::ChatCore(ChatCallback& callback,
                   ScriptExecutorFn script_executor,
                   MessageHistory* history,
                   const ChatCoreOptions& options)
    : impl_(std::make_unique<Impl>(callback, std::move(script_executor), history, options)) {}

ChatCore::~ChatCore() = default;

bool ChatCore::connect(const AuthCredentials& credentials) {
    impl_->state = ChatState::Connecting;
    impl_->cancelled = false;
    
    // For System auth, use CLI mode
    if (credentials.type == AuthType::System || credentials.type == AuthType::None) {
        impl_->cli_path = CLITransport::find_cli();
        if (!impl_->cli_path.empty()) {
            impl_->use_cli_mode = true;
            impl_->state = ChatState::Idle;
            return true;
        }
        // Fall through to try API mode
    }
    
    // API mode
    impl_->use_cli_mode = false;
    AuthCredentials creds = credentials;
    if (!creds.is_configured()) {
        creds = AuthCredentials{};
    }
    
    impl_->client = std::make_unique<ClaudeClient>(creds);
    
    if (!impl_->client->is_configured()) {
        impl_->state = ChatState::Disconnected;
        return false;
    }
    
    impl_->client->set_model(impl_->options.model);
    impl_->state = ChatState::Idle;
    
    return true;
}

void ChatCore::disconnect() {
    impl_->client.reset();
    impl_->state = ChatState::Disconnected;
}

bool ChatCore::is_connected() const noexcept {
    if (impl_->use_cli_mode) {
        return impl_->state != ChatState::Disconnected && !impl_->cli_path.empty();
    }
    return impl_->state != ChatState::Disconnected && impl_->client != nullptr;
}

ProcessResult ChatCore::process_message(const std::string& user_input) {
    ProcessResult result;
    
    if (!is_connected()) {
        result.error = "Not connected";
        return result;
    }
    
    // Use CLI mode if configured
    if (impl_->use_cli_mode) {
        return impl_->process_message_cli(user_input);
    }
    
    impl_->state = ChatState::Processing;
    impl_->cancelled = false;
    
    // Add user message to conversation
    impl_->conversation.push_back(ClaudeMessage::user(user_input));
    
    // Log to history
    if (impl_->history) {
        impl_->history->append_user_message(user_input);
    }
    
    int turn = 0;
    std::string full_response;
    
    while (turn < impl_->options.max_turns && !impl_->cancelled) {
        turn++;
        impl_->callback.on_turn_start(turn, impl_->options.max_turns);
        impl_->callback.on_thinking();
        
        // Build request
        CreateMessageRequest request;
        request.model = impl_->options.model;
        request.messages = impl_->conversation;
        request.system = impl_->system_prompt;
        request.tools = ClaudeClient::get_default_tools();
        request.stream = true;
        
        if (impl_->options.enable_thinking) {
            request.thinking = CreateMessageRequest::ThinkingConfig{true, impl_->options.thinking_budget};
        }
        
        // Accumulate text for this turn
        std::string turn_text;
        bool first_text = true;
        
        auto response = impl_->client->send_message_streaming(request,
            [this, &turn_text, &first_text](const StreamEvent& event) {
                if (impl_->cancelled) return;
                
                if (event.type == StreamEventType::ContentBlockDelta && event.delta.has_value()) {
                    if (event.delta->type == "text_delta" && !event.delta->text.empty()) {
                        if (first_text) {
                            impl_->callback.on_thinking_done();
                            first_text = false;
                        }
                        turn_text += event.delta->text;
                        // Stream text without scripts
                        std::string text_only = strip_idascript_blocks(event.delta->text);
                        if (!text_only.empty()) {
                            impl_->callback.on_text(text_only);
                        }
                    }
                }
            });
        
        if (!response.has_value()) {
            if (impl_->cancelled) {
                result.cancelled = true;
            } else {
                result.error = "Failed to get response from Claude";
            }
            impl_->state = ChatState::Idle;
            return result;
        }
        
        // Get full response text
        std::string response_text = response->get_text();
        
        // Add to conversation
        ClaudeMessage assistant_msg;
        assistant_msg.role = MessageRole::Assistant;
        assistant_msg.content = response->content;
        impl_->conversation.push_back(assistant_msg);
        
        // Log to history
        if (impl_->history) {
            impl_->history->append_assistant_message(response_text, impl_->options.model, response->usage);
        }
        
        // Update usage
        impl_->total_usage += response->usage;
        
        full_response += response_text;
        
        // Check for scripts
        if (has_idascript_blocks(response_text)) {
            auto [scripts, outputs] = impl_->process_scripts(response_text);
            
            if (!scripts.empty()) {
                // Build tool result message
                // For simplicity, we'll concatenate results
                std::string combined_output;
                for (size_t i = 0; i < outputs.size(); ++i) {
                    if (i > 0) combined_output += "\n---\n";
                    combined_output += outputs[i];
                }
                
                // Add tool result to conversation as user message
                // Note: This is a simplification - real implementation would use proper tool_use/tool_result
                ClaudeMessage result_msg = ClaudeMessage::user("Script output:\n" + combined_output);
                impl_->conversation.push_back(result_msg);
                
                // Continue the loop for another turn
                continue;
            }
        }
        
        // No more scripts, we're done
        break;
    }
    
    result.success = true;
    result.response = strip_idascript_blocks(full_response);
    result.turns_used = turn;
    result.cost = impl_->client->estimate_cost();
    
    impl_->callback.on_result(turn, result.cost);
    impl_->state = ChatState::Idle;
    
    return result;
}

void ChatCore::request_cancel() {
    impl_->cancelled = true;
    if (impl_->client) {
        impl_->client->cancel();
    }
    impl_->state = ChatState::Cancelled;
}

bool ChatCore::is_cancelled() const noexcept {
    return impl_->cancelled;
}

ChatState ChatCore::get_state() const noexcept {
    return impl_->state;
}

TokenUsage ChatCore::get_total_usage() const {
    return impl_->total_usage;
}

void ChatCore::set_system_prompt(const std::string& prompt) {
    impl_->system_prompt = prompt;
}

void ChatCore::load_system_prompt(const std::string& project_dir, bool inside_ida) {
    impl_->system_prompt = load_default_system_prompt(project_dir, inside_ida);
}

void ChatCore::clear_conversation() {
    impl_->conversation.clear();
}

void ChatCore::start_new_session() {
    clear_conversation();
    if (impl_->history) {
        (void)impl_->history->start_new_session();
    }
}

int ChatCore::get_message_count() const {
    return static_cast<int>(impl_->conversation.size());
}

// ============================================================================
// Utility Functions
// ============================================================================

std::string load_default_system_prompt(const std::string& project_dir, bool inside_ida) {
    std::string prompt;
    
    // Load PROMPT.md
    std::string prompt_path = project_dir + "/PROMPT.md";
    if (auto content = read_file(prompt_path)) {
        prompt = content.value();
    }
    
    // Load API_REFERENCE.md
    std::string api_path = project_dir + "/API_REFERENCE.md";
    if (auto content = read_file(api_path)) {
        prompt += "\n\n" + content.value();
    }
    
    // Load USAGE.md
    std::string usage_path = project_dir + "/USAGE.md";
    if (auto content = read_file(usage_path)) {
        prompt += "\n\n" + content.value();
    }
    
    // Load IDA.md only when running inside IDA
    if (inside_ida) {
        std::string ida_path = project_dir + "/IDA.md";
        if (auto content = read_file(ida_path)) {
            prompt += "\n\n" + content.value();
        }
    }
    
    return prompt;
}

std::pair<bool, std::string> test_claude_connection(const AuthCredentials& credentials) {
    // For System auth, use CLI transport (like the Python SDK does)
    if (credentials.type == AuthType::System || credentials.type == AuthType::None) {
        return test_cli_connection();
    }
    
    // For explicit API key/OAuth, use direct API
    ClaudeClient client(credentials);
    return client.test_connection();
}

} // namespace ida_chat
