/**
 * @file http_client.hpp
 * @brief HTTP client wrapper using libcurl.
 */

#pragma once

#include <ida_chat/core/types.hpp>

#include <string>
#include <vector>
#include <functional>
#include <optional>
#include <memory>
#include <map>

namespace ida_chat {

/**
 * @brief HTTP request method.
 */
enum class HttpMethod {
    GET,
    POST,
    PUT,
    DELETE,
    PATCH
};

/**
 * @brief HTTP response structure.
 */
struct HttpResponse {
    int status_code = 0;
    std::string body;
    std::map<std::string, std::string> headers;
    std::string error;
    bool success = false;
    
    [[nodiscard]] bool is_success() const noexcept {
        return success && status_code >= 200 && status_code < 300;
    }
};

/**
 * @brief Callback for streaming response data.
 * @param data Chunk of response data
 * @return true to continue, false to abort
 */
using StreamCallback = std::function<bool(const std::string& data)>;

/**
 * @brief HTTP client for making API requests.
 * 
 * Thread-safe wrapper around libcurl.
 */
class HttpClient {
public:
    HttpClient();
    ~HttpClient();
    
    // Non-copyable, movable
    HttpClient(const HttpClient&) = delete;
    HttpClient& operator=(const HttpClient&) = delete;
    HttpClient(HttpClient&&) noexcept;
    HttpClient& operator=(HttpClient&&) noexcept;
    
    /**
     * @brief Set the base URL for all requests.
     */
    void set_base_url(const std::string& url);
    
    /**
     * @brief Set a default header for all requests.
     */
    void set_header(const std::string& name, const std::string& value);
    
    /**
     * @brief Remove a default header.
     */
    void remove_header(const std::string& name);
    
    /**
     * @brief Clear all default headers.
     */
    void clear_headers();
    
    /**
     * @brief Set request timeout in milliseconds.
     */
    void set_timeout(int timeout_ms);
    
    /**
     * @brief Set connection timeout in milliseconds.
     */
    void set_connect_timeout(int timeout_ms);
    
    /**
     * @brief Perform a synchronous HTTP request.
     */
    HttpResponse request(HttpMethod method,
                        const std::string& path,
                        const std::string& body = "",
                        const std::map<std::string, std::string>& headers = {});
    
    /**
     * @brief Perform a streaming HTTP request.
     * 
     * @param method HTTP method
     * @param path Request path (appended to base URL)
     * @param body Request body
     * @param callback Called with each chunk of response data
     * @param headers Additional headers for this request
     * @return HTTP response (body may be empty if streamed)
     */
    HttpResponse stream_request(HttpMethod method,
                               const std::string& path,
                               const std::string& body,
                               StreamCallback callback,
                               const std::map<std::string, std::string>& headers = {});
    
    /**
     * @brief Cancel any ongoing request.
     */
    void cancel();
    
    /**
     * @brief Check if there's an ongoing request.
     */
    [[nodiscard]] bool is_busy() const noexcept;
    
    /**
     * @brief Convenience method for GET request.
     */
    HttpResponse get(const std::string& path,
                    const std::map<std::string, std::string>& headers = {}) {
        return request(HttpMethod::GET, path, "", headers);
    }
    
    /**
     * @brief Convenience method for POST request.
     */
    HttpResponse post(const std::string& path,
                     const std::string& body,
                     const std::map<std::string, std::string>& headers = {}) {
        return request(HttpMethod::POST, path, body, headers);
    }

private:
    struct Impl;
    std::unique_ptr<Impl> impl_;
};

/**
 * @brief URL encode a string.
 */
[[nodiscard]] std::string url_encode(const std::string& s);

/**
 * @brief URL decode a string.
 */
[[nodiscard]] std::string url_decode(const std::string& s);

} // namespace ida_chat
