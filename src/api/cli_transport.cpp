/**
 * @file cli_transport.cpp
 * @brief CLI subprocess transport implementation.
 */

#include <ida_chat/api/cli_transport.hpp>
#include <ida_chat/common/json.hpp>

#include <array>
#include <cstdio>
#include <cstdlib>
#include <sstream>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <queue>

#ifdef __APPLE__
#include <unistd.h>
#include <signal.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <spawn.h>
extern char **environ;
#endif

namespace ida_chat {

// ============================================================================
// Implementation
// ============================================================================

struct CLITransport::Impl {
    CLITransportOptions options;
    
    // Process handles
    pid_t pid = -1;
    int stdin_fd = -1;
    int stdout_fd = -1;
    int stderr_fd = -1;
    
    // State
    std::atomic<bool> connected{false};
    std::atomic<bool> cancelled{false};
    std::string last_error;
    
    // Callbacks
    CLIStderrCallback stderr_callback;
    
    // Stderr reader thread
    std::thread stderr_thread;
    
    explicit Impl(const CLITransportOptions& opts) : options(opts) {
        if (options.cli_path.empty()) {
            options.cli_path = CLITransport::find_cli();
        }
    }
    
    ~Impl() {
        disconnect();
    }
    
    std::vector<std::string> build_command() {
        std::vector<std::string> cmd;
        cmd.push_back(options.cli_path);
        cmd.push_back("--output-format");
        cmd.push_back("stream-json");
        cmd.push_back("--verbose");
        
        // System prompt
        if (!options.system_prompt.empty()) {
            cmd.push_back("--append-system-prompt");
            cmd.push_back(options.system_prompt);
        }
        
        // Allowed tools
        if (!options.allowed_tools.empty()) {
            std::string tools;
            for (size_t i = 0; i < options.allowed_tools.size(); ++i) {
                if (i > 0) tools += ",";
                tools += options.allowed_tools[i];
            }
            cmd.push_back("--allowedTools");
            cmd.push_back(tools);
        }
        
        // Permission mode
        if (!options.permission_mode.empty()) {
            cmd.push_back("--permission-mode");
            cmd.push_back(options.permission_mode);
        }
        
        // Max turns
        cmd.push_back("--max-turns");
        cmd.push_back(std::to_string(options.max_turns));
        
        // Model
        if (!options.model.empty()) {
            cmd.push_back("--model");
            cmd.push_back(options.model);
        }
        
        // Setting sources - disable all external settings
        cmd.push_back("--setting-sources");
        cmd.push_back("");
        
        // Streaming input mode
        cmd.push_back("--input-format");
        cmd.push_back("stream-json");
        
        return cmd;
    }
    
    bool connect() {
        if (connected) return true;
        
        if (options.cli_path.empty()) {
            last_error = "Claude CLI not found";
            return false;
        }
        
#ifdef __APPLE__
        // Create pipes for stdin, stdout, stderr
        int stdin_pipe[2], stdout_pipe[2], stderr_pipe[2];
        
        if (pipe(stdin_pipe) < 0 || pipe(stdout_pipe) < 0 || pipe(stderr_pipe) < 0) {
            last_error = "Failed to create pipes";
            return false;
        }
        
        // Build command
        auto cmd_parts = build_command();
        
        // Convert to argv
        std::vector<char*> argv;
        for (auto& part : cmd_parts) {
            argv.push_back(const_cast<char*>(part.c_str()));
        }
        argv.push_back(nullptr);
        
        // Set up file actions
        posix_spawn_file_actions_t actions;
        posix_spawn_file_actions_init(&actions);
        
        // Child: stdin reads from stdin_pipe[0]
        posix_spawn_file_actions_adddup2(&actions, stdin_pipe[0], STDIN_FILENO);
        posix_spawn_file_actions_addclose(&actions, stdin_pipe[1]);
        
        // Child: stdout writes to stdout_pipe[1]
        posix_spawn_file_actions_adddup2(&actions, stdout_pipe[1], STDOUT_FILENO);
        posix_spawn_file_actions_addclose(&actions, stdout_pipe[0]);
        
        // Child: stderr writes to stderr_pipe[1]
        posix_spawn_file_actions_adddup2(&actions, stderr_pipe[1], STDERR_FILENO);
        posix_spawn_file_actions_addclose(&actions, stderr_pipe[0]);
        
        // Set up spawn attributes
        posix_spawnattr_t attr;
        posix_spawnattr_init(&attr);
        
        // Set environment
        std::vector<std::string> env_strings;
        std::vector<char*> envp;
        
        // Copy current environment
        for (char** e = environ; *e != nullptr; ++e) {
            env_strings.push_back(*e);
        }
        
        // Add our env vars
        env_strings.push_back("CLAUDE_CODE_ENTRYPOINT=sdk-cpp");
        
        if (!options.cwd.empty()) {
            env_strings.push_back("PWD=" + options.cwd);
        }
        
        for (auto& env : env_strings) {
            envp.push_back(const_cast<char*>(env.c_str()));
        }
        envp.push_back(nullptr);
        
        // Spawn the process
        int result = posix_spawn(&pid, argv[0], &actions, &attr, argv.data(), envp.data());
        
        posix_spawn_file_actions_destroy(&actions);
        posix_spawnattr_destroy(&attr);
        
        if (result != 0) {
            last_error = "Failed to spawn CLI process: " + std::to_string(result);
            close(stdin_pipe[0]); close(stdin_pipe[1]);
            close(stdout_pipe[0]); close(stdout_pipe[1]);
            close(stderr_pipe[0]); close(stderr_pipe[1]);
            return false;
        }
        
        // Parent: close unused ends
        close(stdin_pipe[0]);   // We write to stdin
        close(stdout_pipe[1]);  // We read from stdout
        close(stderr_pipe[1]);  // We read from stderr
        
        stdin_fd = stdin_pipe[1];
        stdout_fd = stdout_pipe[0];
        stderr_fd = stderr_pipe[0];
        
        // Make stdout non-blocking for interruptible reads
        int flags = fcntl(stdout_fd, F_GETFL, 0);
        fcntl(stdout_fd, F_SETFL, flags | O_NONBLOCK);
        
        // Start stderr reader thread
        stderr_thread = std::thread([this]() {
            read_stderr();
        });
        
        connected = true;
        cancelled = false;
        return true;
#else
        last_error = "Platform not supported";
        return false;
#endif
    }
    
    void disconnect() {
        if (!connected) return;
        connected = false;
        cancelled = true;
        
#ifdef __APPLE__
        // Close file descriptors
        if (stdin_fd >= 0) {
            close(stdin_fd);
            stdin_fd = -1;
        }
        if (stdout_fd >= 0) {
            close(stdout_fd);
            stdout_fd = -1;
        }
        if (stderr_fd >= 0) {
            close(stderr_fd);
            stderr_fd = -1;
        }
        
        // Wait for stderr thread
        if (stderr_thread.joinable()) {
            stderr_thread.join();
        }
        
        // Terminate process if still running
        if (pid > 0) {
            kill(pid, SIGTERM);
            int status;
            waitpid(pid, &status, 0);
            pid = -1;
        }
#endif
    }
    
    void read_stderr() {
#ifdef __APPLE__
        char buffer[4096];
        std::string line_buffer;
        
        while (connected && stderr_fd >= 0) {
            ssize_t n = read(stderr_fd, buffer, sizeof(buffer) - 1);
            if (n <= 0) {
                if (n < 0 && errno == EAGAIN) {
                    std::this_thread::sleep_for(std::chrono::milliseconds(10));
                    continue;
                }
                break;
            }
            
            buffer[n] = '\0';
            line_buffer += buffer;
            
            // Process complete lines
            size_t pos;
            while ((pos = line_buffer.find('\n')) != std::string::npos) {
                std::string line = line_buffer.substr(0, pos);
                line_buffer.erase(0, pos + 1);
                
                if (!line.empty() && stderr_callback) {
                    stderr_callback(line);
                }
            }
        }
#endif
    }
    
    bool write_line(const std::string& line) {
#ifdef __APPLE__
        if (stdin_fd < 0) return false;
        
        std::string data = line + "\n";
        ssize_t written = write(stdin_fd, data.c_str(), data.size());
        return written == static_cast<ssize_t>(data.size());
#else
        return false;
#endif
    }
    
    bool read_line(std::string& line, int timeout_ms = 100) {
#ifdef __APPLE__
        if (stdout_fd < 0) return false;
        
        static std::string buffer;
        char chunk[4096];
        
        auto start = std::chrono::steady_clock::now();
        
        while (!cancelled) {
            // Check for complete line in buffer
            size_t pos = buffer.find('\n');
            if (pos != std::string::npos) {
                line = buffer.substr(0, pos);
                buffer.erase(0, pos + 1);
                return true;
            }
            
            // Try to read more data
            ssize_t n = read(stdout_fd, chunk, sizeof(chunk) - 1);
            if (n > 0) {
                chunk[n] = '\0';
                buffer += chunk;
                continue;
            }
            
            if (n < 0 && errno == EAGAIN) {
                // No data available, check timeout
                auto now = std::chrono::steady_clock::now();
                auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(now - start).count();
                if (elapsed > timeout_ms) {
                    return false; // Timeout
                }
                std::this_thread::sleep_for(std::chrono::milliseconds(10));
                continue;
            }
            
            // EOF or error
            if (!buffer.empty()) {
                line = buffer;
                buffer.clear();
                return true;
            }
            return false;
        }
        
        return false;
#else
        return false;
#endif
    }
};

// ============================================================================
// CLITransport Implementation
// ============================================================================

CLITransport::CLITransport(const CLITransportOptions& options)
    : impl_(std::make_unique<Impl>(options)) {}

CLITransport::~CLITransport() = default;

std::string CLITransport::find_cli() {
#ifdef __APPLE__
    // Check common locations
    std::vector<std::string> locations = {
        std::string(getenv("HOME") ? getenv("HOME") : "") + "/.local/bin/claude",
        "/usr/local/bin/claude",
        std::string(getenv("HOME") ? getenv("HOME") : "") + "/.npm-global/bin/claude",
        std::string(getenv("HOME") ? getenv("HOME") : "") + "/node_modules/.bin/claude",
        std::string(getenv("HOME") ? getenv("HOME") : "") + "/.yarn/bin/claude",
        std::string(getenv("HOME") ? getenv("HOME") : "") + "/.claude/local/claude",
    };
    
    for (const auto& path : locations) {
        if (access(path.c_str(), X_OK) == 0) {
            return path;
        }
    }
    
    // Try which
    FILE* fp = popen("which claude 2>/dev/null", "r");
    if (fp) {
        char buffer[512];
        if (fgets(buffer, sizeof(buffer), fp)) {
            pclose(fp);
            std::string result(buffer);
            // Remove trailing newline
            while (!result.empty() && (result.back() == '\n' || result.back() == '\r')) {
                result.pop_back();
            }
            if (!result.empty()) {
                return result;
            }
        } else {
            pclose(fp);
        }
    }
#endif
    return "";
}

bool CLITransport::connect() {
    return impl_->connect();
}

void CLITransport::disconnect() {
    impl_->disconnect();
}

bool CLITransport::is_connected() const noexcept {
    return impl_->connected;
}

bool CLITransport::query(const std::string& message, const std::string& session_id) {
    if (!impl_->connected) return false;
    
    // Build JSON message
    nlohmann::json msg;
    msg["type"] = "user";
    msg["message"] = {
        {"role", "user"},
        {"content", message}
    };
    msg["parent_tool_use_id"] = nullptr;
    msg["session_id"] = session_id;
    
    return impl_->write_line(msg.dump());
}

bool CLITransport::receive_messages(CLIMessageCallback callback) {
    if (!impl_->connected) return false;
    
    std::string line;
    // Use longer timeout for waiting for response
    while (impl_->read_line(line, 30000)) {  // 30 second timeout
        if (line.empty()) continue;
        
        // Try to parse as JSON
        try {
            auto json = nlohmann::json::parse(line);
            
            // Check for result message (end of response)
            if (json.contains("type") && json["type"] == "result") {
                if (callback) callback(line);
                return true;
            }
            
            // Pass to callback
            if (callback && !callback(line)) {
                return false;
            }
        } catch (const std::exception&) {
            // Not JSON, skip
            continue;
        }
    }
    
    return false;
}

void CLITransport::interrupt() {
    impl_->cancelled = true;
#ifdef __APPLE__
    if (impl_->pid > 0) {
        kill(impl_->pid, SIGINT);
    }
#endif
}

void CLITransport::set_stderr_callback(CLIStderrCallback callback) {
    impl_->stderr_callback = std::move(callback);
}

std::string CLITransport::get_last_error() const {
    return impl_->last_error;
}

// ============================================================================
// Test Connection
// ============================================================================

std::pair<bool, std::string> test_cli_connection(const std::string& cli_path) {
    std::string path = cli_path.empty() ? CLITransport::find_cli() : cli_path;
    
    if (path.empty()) {
        return {false, "Claude CLI not found. Install with: npm install -g @anthropic-ai/claude-code"};
    }
    
#ifdef __APPLE__
    // Test with a simple --print query (single-shot mode)
    // This verifies both CLI presence and authentication
    std::string cmd = "\"" + path + "\" --print --output-format stream-json "
                      "--permission-mode bypassPermissions --setting-sources \"\" "
                      "--max-turns 1 -- \"Say exactly: Hello from IDA Chat\" 2>&1";
    
    FILE* fp = popen(cmd.c_str(), "r");
    if (!fp) {
        return {false, "Failed to execute Claude CLI"};
    }
    
    char buffer[4096];
    std::string output;
    while (fgets(buffer, sizeof(buffer), fp)) {
        output += buffer;
    }
    int status = pclose(fp);
    
    // Parse JSON lines to find assistant message or result
    std::string response_text;
    std::string error_text;
    bool got_response = false;
    
    std::istringstream iss(output);
    std::string line;
    while (std::getline(iss, line)) {
        if (line.empty()) continue;
        
        try {
            auto json = nlohmann::json::parse(line);
            std::string type = json.value("type", "");
            
            if (type == "assistant") {
                // Extract text from content blocks
                if (json.contains("message") && json["message"].contains("content")) {
                    for (const auto& block : json["message"]["content"]) {
                        if (block.value("type", "") == "text") {
                            response_text += block.value("text", "");
                            got_response = true;
                        }
                    }
                }
            } else if (type == "result") {
                if (json.value("is_error", false)) {
                    error_text = json.value("result", "Unknown error");
                }
            } else if (type == "system" && json.value("subtype", "") == "error") {
                error_text = json["data"].value("message", "System error");
            }
        } catch (...) {
            // Not JSON, might be stderr output
            if (line.find("Error") != std::string::npos || 
                line.find("error") != std::string::npos) {
                error_text = line;
            }
        }
    }
    
    if (!error_text.empty()) {
        return {false, error_text};
    }
    
    if (got_response) {
        // Trim response
        while (!response_text.empty() && 
               (response_text.back() == '\n' || response_text.back() == ' ')) {
            response_text.pop_back();
        }
        return {true, "Connected: " + response_text};
    }
    
    if (status != 0) {
        return {false, "CLI exited with error code " + std::to_string(status)};
    }
    
    return {false, "No response from Claude"};
#else
    return {false, "Platform not supported"};
#endif
}

} // namespace ida_chat
