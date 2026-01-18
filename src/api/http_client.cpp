/**
 * @file http_client.cpp
 * @brief HTTP client implementation using libcurl.
 */

#include <ida_chat/api/http_client.hpp>

#include <curl/curl.h>

#include <mutex>
#include <atomic>

namespace ida_chat {

// ============================================================================
// Implementation Details
// ============================================================================

struct HttpClient::Impl {
    CURL* curl = nullptr;
    std::string base_url;
    std::map<std::string, std::string> default_headers;
    int timeout_ms = 120000;
    int connect_timeout_ms = 30000;
    std::atomic<bool> cancelled{false};
    std::atomic<bool> busy{false};
    std::mutex mutex;
    
    Impl() {
        curl_global_init(CURL_GLOBAL_DEFAULT);
        curl = curl_easy_init();
    }
    
    ~Impl() {
        if (curl) {
            curl_easy_cleanup(curl);
        }
        curl_global_cleanup();
    }
    
    static size_t write_callback(char* ptr, size_t size, size_t nmemb, void* userdata) {
        auto* response = static_cast<std::string*>(userdata);
        size_t total = size * nmemb;
        response->append(ptr, total);
        return total;
    }
    
    static size_t header_callback(char* buffer, size_t size, size_t nitems, void* userdata) {
        auto* headers = static_cast<std::map<std::string, std::string>*>(userdata);
        size_t total = size * nitems;
        std::string line(buffer, total);
        
        // Parse header line
        auto colon = line.find(':');
        if (colon != std::string::npos) {
            std::string name = line.substr(0, colon);
            std::string value = line.substr(colon + 1);
            // Trim whitespace
            while (!value.empty() && (value.front() == ' ' || value.front() == '\t')) {
                value.erase(0, 1);
            }
            while (!value.empty() && (value.back() == '\r' || value.back() == '\n' || 
                                       value.back() == ' ' || value.back() == '\t')) {
                value.pop_back();
            }
            (*headers)[name] = value;
        }
        return total;
    }
    
    static size_t stream_callback(char* ptr, size_t size, size_t nmemb, void* userdata) {
        auto* ctx = static_cast<std::pair<StreamCallback*, std::atomic<bool>*>*>(userdata);
        size_t total = size * nmemb;
        
        // Check for cancellation
        if (ctx->second->load()) {
            return 0; // Abort transfer
        }
        
        std::string chunk(ptr, total);
        if (!(*ctx->first)(chunk)) {
            return 0; // Callback requested abort
        }
        
        return total;
    }
    
    static int progress_callback(void* clientp, curl_off_t dltotal, curl_off_t dlnow,
                                 curl_off_t ultotal, curl_off_t ulnow) {
        auto* cancelled = static_cast<std::atomic<bool>*>(clientp);
        return cancelled->load() ? 1 : 0;
    }
};

// ============================================================================
// HttpClient Implementation
// ============================================================================

HttpClient::HttpClient() : impl_(std::make_unique<Impl>()) {}

HttpClient::~HttpClient() = default;

HttpClient::HttpClient(HttpClient&&) noexcept = default;
HttpClient& HttpClient::operator=(HttpClient&&) noexcept = default;

void HttpClient::set_base_url(const std::string& url) {
    std::lock_guard<std::mutex> lock(impl_->mutex);
    impl_->base_url = url;
}

void HttpClient::set_header(const std::string& name, const std::string& value) {
    std::lock_guard<std::mutex> lock(impl_->mutex);
    impl_->default_headers[name] = value;
}

void HttpClient::remove_header(const std::string& name) {
    std::lock_guard<std::mutex> lock(impl_->mutex);
    impl_->default_headers.erase(name);
}

void HttpClient::clear_headers() {
    std::lock_guard<std::mutex> lock(impl_->mutex);
    impl_->default_headers.clear();
}

void HttpClient::set_timeout(int timeout_ms) {
    impl_->timeout_ms = timeout_ms;
}

void HttpClient::set_connect_timeout(int timeout_ms) {
    impl_->connect_timeout_ms = timeout_ms;
}

HttpResponse HttpClient::request(HttpMethod method,
                                 const std::string& path,
                                 const std::string& body,
                                 const std::map<std::string, std::string>& headers) {
    HttpResponse response;
    
    if (!impl_->curl) {
        response.error = "CURL not initialized";
        return response;
    }
    
    std::lock_guard<std::mutex> lock(impl_->mutex);
    impl_->busy = true;
    impl_->cancelled = false;
    
    curl_easy_reset(impl_->curl);
    
    // Build URL
    std::string url = impl_->base_url + path;
    curl_easy_setopt(impl_->curl, CURLOPT_URL, url.c_str());
    
    // Set method
    switch (method) {
        case HttpMethod::GET:
            curl_easy_setopt(impl_->curl, CURLOPT_HTTPGET, 1L);
            break;
        case HttpMethod::POST:
            curl_easy_setopt(impl_->curl, CURLOPT_POST, 1L);
            break;
        case HttpMethod::PUT:
            curl_easy_setopt(impl_->curl, CURLOPT_CUSTOMREQUEST, "PUT");
            break;
        case HttpMethod::DELETE:
            curl_easy_setopt(impl_->curl, CURLOPT_CUSTOMREQUEST, "DELETE");
            break;
        case HttpMethod::PATCH:
            curl_easy_setopt(impl_->curl, CURLOPT_CUSTOMREQUEST, "PATCH");
            break;
    }
    
    // Set body
    if (!body.empty()) {
        curl_easy_setopt(impl_->curl, CURLOPT_POSTFIELDS, body.c_str());
        curl_easy_setopt(impl_->curl, CURLOPT_POSTFIELDSIZE, static_cast<long>(body.size()));
    }
    
    // Build headers
    struct curl_slist* header_list = nullptr;
    for (const auto& [name, value] : impl_->default_headers) {
        std::string header = name + ": " + value;
        header_list = curl_slist_append(header_list, header.c_str());
    }
    for (const auto& [name, value] : headers) {
        std::string header = name + ": " + value;
        header_list = curl_slist_append(header_list, header.c_str());
    }
    if (header_list) {
        curl_easy_setopt(impl_->curl, CURLOPT_HTTPHEADER, header_list);
    }
    
    // Set callbacks
    std::string response_body;
    curl_easy_setopt(impl_->curl, CURLOPT_WRITEFUNCTION, Impl::write_callback);
    curl_easy_setopt(impl_->curl, CURLOPT_WRITEDATA, &response_body);
    curl_easy_setopt(impl_->curl, CURLOPT_HEADERFUNCTION, Impl::header_callback);
    curl_easy_setopt(impl_->curl, CURLOPT_HEADERDATA, &response.headers);
    
    // Set timeouts
    curl_easy_setopt(impl_->curl, CURLOPT_TIMEOUT_MS, static_cast<long>(impl_->timeout_ms));
    curl_easy_setopt(impl_->curl, CURLOPT_CONNECTTIMEOUT_MS, static_cast<long>(impl_->connect_timeout_ms));
    
    // Enable progress for cancellation
    curl_easy_setopt(impl_->curl, CURLOPT_NOPROGRESS, 0L);
    curl_easy_setopt(impl_->curl, CURLOPT_XFERINFOFUNCTION, Impl::progress_callback);
    curl_easy_setopt(impl_->curl, CURLOPT_XFERINFODATA, &impl_->cancelled);
    
    // SSL options
    curl_easy_setopt(impl_->curl, CURLOPT_SSL_VERIFYPEER, 1L);
    curl_easy_setopt(impl_->curl, CURLOPT_SSL_VERIFYHOST, 2L);
    
    // Perform request
    CURLcode res = curl_easy_perform(impl_->curl);
    
    // Cleanup headers
    if (header_list) {
        curl_slist_free_all(header_list);
    }
    
    impl_->busy = false;
    
    if (res != CURLE_OK) {
        response.error = curl_easy_strerror(res);
        response.success = false;
        return response;
    }
    
    // Get response code
    long status_code;
    curl_easy_getinfo(impl_->curl, CURLINFO_RESPONSE_CODE, &status_code);
    
    response.status_code = static_cast<int>(status_code);
    response.body = std::move(response_body);
    response.success = true;
    
    return response;
}

HttpResponse HttpClient::stream_request(HttpMethod method,
                                        const std::string& path,
                                        const std::string& body,
                                        StreamCallback callback,
                                        const std::map<std::string, std::string>& headers) {
    HttpResponse response;
    
    if (!impl_->curl) {
        response.error = "CURL not initialized";
        return response;
    }
    
    std::lock_guard<std::mutex> lock(impl_->mutex);
    impl_->busy = true;
    impl_->cancelled = false;
    
    curl_easy_reset(impl_->curl);
    
    // Build URL
    std::string url = impl_->base_url + path;
    curl_easy_setopt(impl_->curl, CURLOPT_URL, url.c_str());
    
    // Set method (usually POST for streaming)
    if (method == HttpMethod::POST) {
        curl_easy_setopt(impl_->curl, CURLOPT_POST, 1L);
    }
    
    // Set body
    if (!body.empty()) {
        curl_easy_setopt(impl_->curl, CURLOPT_POSTFIELDS, body.c_str());
        curl_easy_setopt(impl_->curl, CURLOPT_POSTFIELDSIZE, static_cast<long>(body.size()));
    }
    
    // Build headers
    struct curl_slist* header_list = nullptr;
    for (const auto& [name, value] : impl_->default_headers) {
        std::string header = name + ": " + value;
        header_list = curl_slist_append(header_list, header.c_str());
    }
    for (const auto& [name, value] : headers) {
        std::string header = name + ": " + value;
        header_list = curl_slist_append(header_list, header.c_str());
    }
    if (header_list) {
        curl_easy_setopt(impl_->curl, CURLOPT_HTTPHEADER, header_list);
    }
    
    // Set streaming callback
    auto ctx = std::make_pair(&callback, &impl_->cancelled);
    curl_easy_setopt(impl_->curl, CURLOPT_WRITEFUNCTION, Impl::stream_callback);
    curl_easy_setopt(impl_->curl, CURLOPT_WRITEDATA, &ctx);
    curl_easy_setopt(impl_->curl, CURLOPT_HEADERFUNCTION, Impl::header_callback);
    curl_easy_setopt(impl_->curl, CURLOPT_HEADERDATA, &response.headers);
    
    // Set timeouts (longer for streaming)
    curl_easy_setopt(impl_->curl, CURLOPT_TIMEOUT, 0L); // No timeout for streaming
    curl_easy_setopt(impl_->curl, CURLOPT_CONNECTTIMEOUT_MS, static_cast<long>(impl_->connect_timeout_ms));
    
    // Enable progress for cancellation
    curl_easy_setopt(impl_->curl, CURLOPT_NOPROGRESS, 0L);
    curl_easy_setopt(impl_->curl, CURLOPT_XFERINFOFUNCTION, Impl::progress_callback);
    curl_easy_setopt(impl_->curl, CURLOPT_XFERINFODATA, &impl_->cancelled);
    
    // SSL options
    curl_easy_setopt(impl_->curl, CURLOPT_SSL_VERIFYPEER, 1L);
    curl_easy_setopt(impl_->curl, CURLOPT_SSL_VERIFYHOST, 2L);
    
    // Perform request
    CURLcode res = curl_easy_perform(impl_->curl);
    
    // Cleanup headers
    if (header_list) {
        curl_slist_free_all(header_list);
    }
    
    impl_->busy = false;
    
    if (res != CURLE_OK) {
        if (impl_->cancelled) {
            response.error = "Request cancelled";
        } else {
            response.error = curl_easy_strerror(res);
        }
        response.success = false;
        return response;
    }
    
    // Get response code
    long status_code;
    curl_easy_getinfo(impl_->curl, CURLINFO_RESPONSE_CODE, &status_code);
    
    response.status_code = static_cast<int>(status_code);
    response.success = true;
    
    return response;
}

void HttpClient::cancel() {
    impl_->cancelled = true;
}

bool HttpClient::is_busy() const noexcept {
    return impl_->busy;
}

// ============================================================================
// URL Encoding
// ============================================================================

std::string url_encode(const std::string& s) {
    CURL* curl = curl_easy_init();
    if (!curl) return s;
    
    char* encoded = curl_easy_escape(curl, s.c_str(), static_cast<int>(s.size()));
    std::string result = encoded ? encoded : s;
    if (encoded) curl_free(encoded);
    curl_easy_cleanup(curl);
    
    return result;
}

std::string url_decode(const std::string& s) {
    CURL* curl = curl_easy_init();
    if (!curl) return s;
    
    int out_len;
    char* decoded = curl_easy_unescape(curl, s.c_str(), static_cast<int>(s.size()), &out_len);
    std::string result = decoded ? std::string(decoded, out_len) : s;
    if (decoded) curl_free(decoded);
    curl_easy_cleanup(curl);
    
    return result;
}

} // namespace ida_chat
