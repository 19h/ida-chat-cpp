/**
 * @file claude_client.cpp
 * @brief Claude API client implementation.
 */

#include <ida_chat/api/claude_client.hpp>
#include <ida_chat/api/streaming_parser.hpp>
#include <ida_chat/api/keychain.hpp>

#include <cstdlib>

namespace ida_chat {

// ============================================================================
// Constants
// ============================================================================

static constexpr const char* DEFAULT_API_BASE = "https://api.anthropic.com";
static constexpr const char* API_VERSION = "2023-06-01";
static constexpr const char* DEFAULT_MODEL = "claude-sonnet-4-20250514";

// Pricing per million tokens (as of 2024)
static constexpr double INPUT_PRICE_PER_M = 3.0;   // $3/M input tokens
static constexpr double OUTPUT_PRICE_PER_M = 15.0; // $15/M output tokens

// ============================================================================
// Implementation
// ============================================================================

struct ClaudeClient::Impl {
    HttpClient http;
    AuthCredentials credentials;
    std::string model = DEFAULT_MODEL;
    TokenUsage total_usage;
    std::atomic<bool> cancelled{false};
    
    Impl() = default;
    
    explicit Impl(const AuthCredentials& creds) : credentials(creds) {
        // For System auth, try to get credentials from Claude Code
        if (credentials.type == AuthType::System) {
            if (!try_claude_code_auth()) {
                try_env_auth();
            }
        }
        setup_client();
    }
    
    void setup_client() {
        std::string base_url = credentials.api_base_url.empty() 
            ? DEFAULT_API_BASE 
            : credentials.api_base_url;
        
        http.set_base_url(base_url);
        http.set_header("Content-Type", "application/json");
        http.set_header("anthropic-version", API_VERSION);
        http.set_header("anthropic-beta", "prompt-caching-2024-07-31,pdfs-2024-09-25");
        
        // Set authorization header if we have an API key
        if (!credentials.api_key.empty()) {
            http.set_header("x-api-key", credentials.api_key);
        }
    }
    
    bool try_claude_code_auth() {
        // Try to read credentials from Claude Code's keychain entry
        auto creds = read_claude_code_credentials();
        if (!creds.has_value()) {
            return false;
        }
        
        if (creds->access_token.empty()) {
            return false;
        }
        
        // Check if token is expired
        if (creds->is_expired()) {
            return false;
        }
        
        credentials.api_key = creds->access_token;
        credentials.type = AuthType::OAuth;  // It's actually an OAuth token
        return true;
    }
    
    bool try_env_auth() {
        // Try environment variables
        const char* key = std::getenv("ANTHROPIC_API_KEY");
        if (!key) {
            key = std::getenv("CLAUDE_API_KEY");
        }
        
        if (key && strlen(key) > 0) {
            credentials.api_key = key;
            return true;
        }
        
        return false;
    }
};

ClaudeClient::ClaudeClient(const AuthCredentials& credentials)
    : impl_(std::make_unique<Impl>(credentials)) {}

ClaudeClient::ClaudeClient() : impl_(std::make_unique<Impl>()) {
    impl_->try_env_auth();
}

ClaudeClient::~ClaudeClient() = default;

std::pair<bool, std::string> ClaudeClient::test_connection() {
    if (!is_configured()) {
        if (impl_->credentials.type == AuthType::System) {
            return {false, "No credentials found. Checked: Claude Code keychain, ANTHROPIC_API_KEY, CLAUDE_API_KEY"};
        }
        return {false, "Not configured - no API key provided"};
    }
    
    // Simple test request
    CreateMessageRequest request;
    request.model = impl_->model;
    request.max_tokens = 100;
    request.stream = false;
    request.messages.push_back(ClaudeMessage::user("Say 'Hello!' and nothing else."));
    
    auto response = send_message(request);
    if (!response.has_value()) {
        return {false, "Failed to connect to Claude API"};
    }
    
    return {true, "Connected: " + response->get_text()};
}

std::optional<CreateMessageResponse> ClaudeClient::send_message(
    const CreateMessageRequest& request) {
    
    if (!is_configured()) {
        return std::nullopt;
    }
    
    nlohmann::json request_json;
    to_json(request_json, request);
    
    auto response = impl_->http.post("/v1/messages", request_json.dump());
    
    if (!response.is_success()) {
        return std::nullopt;
    }
    
    try {
        auto json = nlohmann::json::parse(response.body);
        
        // Check for error response
        if (json.contains("error")) {
            return std::nullopt;
        }
        
        CreateMessageResponse msg_response;
        from_json(json, msg_response);
        
        // Update usage
        impl_->total_usage += msg_response.usage;
        
        return msg_response;
    } catch (const std::exception&) {
        return std::nullopt;
    }
}

std::optional<CreateMessageResponse> ClaudeClient::send_message_streaming(
    const CreateMessageRequest& request,
    StreamEventCallback callback) {
    
    if (!is_configured()) {
        return std::nullopt;
    }
    
    impl_->cancelled = false;
    
    // Ensure streaming is enabled
    CreateMessageRequest streaming_request = request;
    streaming_request.stream = true;
    
    nlohmann::json request_json;
    to_json(request_json, streaming_request);
    
    StreamingParser parser([&callback](const StreamEvent& event) {
        if (callback) {
            callback(event);
        }
    });
    
    auto response = impl_->http.stream_request(
        HttpMethod::POST,
        "/v1/messages",
        request_json.dump(),
        [&parser, this](const std::string& chunk) -> bool {
            if (impl_->cancelled) {
                return false;
            }
            parser.feed(chunk);
            return true;
        }
    );
    
    parser.finish();
    
    if (parser.has_error()) {
        return std::nullopt;
    }
    
    auto final_response = parser.get_response();
    if (final_response.has_value()) {
        impl_->total_usage += final_response->usage;
    }
    
    return final_response;
}

void ClaudeClient::cancel() {
    impl_->cancelled = true;
    impl_->http.cancel();
}

bool ClaudeClient::is_configured() const noexcept {
    // We need an actual API key to be configured
    return !impl_->credentials.api_key.empty();
}

std::string ClaudeClient::get_model() const {
    return impl_->model;
}

void ClaudeClient::set_model(const std::string& model) {
    impl_->model = model;
}

TokenUsage ClaudeClient::get_total_usage() const {
    return impl_->total_usage;
}

void ClaudeClient::reset_usage() {
    impl_->total_usage = TokenUsage{};
}

double ClaudeClient::estimate_cost() const {
    const auto& u = impl_->total_usage;
    double input_cost = (u.input_tokens / 1000000.0) * INPUT_PRICE_PER_M;
    double output_cost = (u.output_tokens / 1000000.0) * OUTPUT_PRICE_PER_M;
    // Cache reads are typically discounted
    double cache_cost = (u.cache_read_tokens / 1000000.0) * (INPUT_PRICE_PER_M * 0.1);
    return input_cost + output_cost + cache_cost;
}

ToolDefinition ClaudeClient::get_idascript_tool() {
    ToolDefinition tool;
    tool.name = "idascript";
    tool.description = R"(Execute Python code in IDA Pro's scripting environment.

The code has access to all IDA Pro APIs and the `db` object from ida-domain for database operations.

Use this tool to:
- Analyze binary code, functions, and data
- Navigate the disassembly
- Query cross-references
- Extract strings and constants
- Decompile functions (if Hex-Rays is available)

The output will be captured and returned. Print results you want to see.)";
    
    tool.input_schema.schema = {
        {"type", "object"},
        {"properties", {
            {"code", {
                {"type", "string"},
                {"description", "Python code to execute in IDA"}
            }}
        }},
        {"required", nlohmann::json::array({"code"})}
    };
    
    return tool;
}

std::vector<ToolDefinition> ClaudeClient::get_default_tools() {
    return {get_idascript_tool()};
}

std::unique_ptr<ClaudeClient> create_client_from_env() {
    auto client = std::make_unique<ClaudeClient>();
    if (client->is_configured()) {
        return client;
    }
    return nullptr;
}

} // namespace ida_chat
