/**
 * @file keychain.cpp
 * @brief macOS Keychain access for Claude Code credentials.
 */

#include <ida_chat/api/keychain.hpp>
#include <ida_chat/common/json.hpp>

#ifdef __APPLE__
#include <Security/Security.h>
#include <CoreFoundation/CoreFoundation.h>
#endif

namespace ida_chat {

std::optional<ClaudeCodeCredentials> read_claude_code_credentials() {
#ifdef __APPLE__
    // Query for Claude Code credentials in keychain
    CFMutableDictionaryRef query = CFDictionaryCreateMutable(
        kCFAllocatorDefault, 0,
        &kCFTypeDictionaryKeyCallBacks,
        &kCFTypeDictionaryValueCallBacks
    );
    
    if (!query) {
        return std::nullopt;
    }
    
    // Set up the query
    CFDictionarySetValue(query, kSecClass, kSecClassGenericPassword);
    
    CFStringRef service = CFStringCreateWithCString(
        kCFAllocatorDefault, "Claude Code-credentials", kCFStringEncodingUTF8
    );
    CFDictionarySetValue(query, kSecAttrService, service);
    CFDictionarySetValue(query, kSecReturnData, kCFBooleanTrue);
    CFDictionarySetValue(query, kSecMatchLimit, kSecMatchLimitOne);
    
    // Execute the query
    CFDataRef result = nullptr;
    OSStatus status = SecItemCopyMatching(query, (CFTypeRef*)&result);
    
    CFRelease(service);
    CFRelease(query);
    
    if (status != errSecSuccess || !result) {
        if (result) CFRelease(result);
        return std::nullopt;
    }
    
    // Convert CFData to string
    const char* data = reinterpret_cast<const char*>(CFDataGetBytePtr(result));
    CFIndex length = CFDataGetLength(result);
    std::string json_str(data, length);
    CFRelease(result);
    
    // Parse JSON
    try {
        auto json = nlohmann::json::parse(json_str);
        
        if (!json.contains("claudeAiOauth")) {
            return std::nullopt;
        }
        
        auto& oauth = json["claudeAiOauth"];
        
        ClaudeCodeCredentials creds;
        creds.access_token = oauth.value("accessToken", "");
        creds.refresh_token = oauth.value("refreshToken", "");
        creds.expires_at = oauth.value("expiresAt", int64_t(0));
        creds.subscription_type = oauth.value("subscriptionType", "");
        
        if (oauth.contains("scopes") && oauth["scopes"].is_array()) {
            for (const auto& scope : oauth["scopes"]) {
                creds.scopes.push_back(scope.get<std::string>());
            }
        }
        
        if (creds.access_token.empty()) {
            return std::nullopt;
        }
        
        return creds;
        
    } catch (const std::exception&) {
        return std::nullopt;
    }
    
#else
    // Non-macOS platforms not supported yet
    return std::nullopt;
#endif
}

bool ClaudeCodeCredentials::is_expired() const {
    if (expires_at == 0) return false;
    
    auto now = std::chrono::system_clock::now();
    auto now_ms = std::chrono::duration_cast<std::chrono::milliseconds>(
        now.time_since_epoch()
    ).count();
    
    // Consider expired if less than 5 minutes remaining
    return now_ms >= (expires_at - 300000);
}

bool ClaudeCodeCredentials::is_max_subscription() const {
    return subscription_type == "max" || subscription_type == "pro";
}

} // namespace ida_chat
