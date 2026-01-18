/**
 * @file types.cpp
 * @brief Implementation of core type utilities.
 */

#include <ida_chat/core/types.hpp>

#include <chrono>
#include <random>
#include <sstream>
#include <iomanip>
#include <fstream>
#include <cstdlib>
#include <cstring>
#include <algorithm>

#ifdef IDA_CHAT_WINDOWS
#include <windows.h>
#include <shlobj.h>
#else
#include <sys/stat.h>
#include <dirent.h>
#include <pwd.h>
#include <unistd.h>
#endif

namespace ida_chat {

// ============================================================================
// Timestamp Utilities
// ============================================================================

std::string get_iso_timestamp() {
    auto now = std::chrono::system_clock::now();
    auto time_t_now = std::chrono::system_clock::to_time_t(now);
    auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(
        now.time_since_epoch()) % 1000;
    
    std::tm tm_now;
#ifdef IDA_CHAT_WINDOWS
    gmtime_s(&tm_now, &time_t_now);
#else
    gmtime_r(&time_t_now, &tm_now);
#endif
    
    std::ostringstream oss;
    oss << std::put_time(&tm_now, "%Y-%m-%dT%H:%M:%S")
        << '.' << std::setfill('0') << std::setw(3) << ms.count()
        << 'Z';
    return oss.str();
}

std::int64_t get_timestamp_ms() {
    auto now = std::chrono::system_clock::now();
    auto duration = now.time_since_epoch();
    return std::chrono::duration_cast<std::chrono::milliseconds>(duration).count();
}

// ============================================================================
// UUID Generation
// ============================================================================

std::string generate_uuid() {
    static std::random_device rd;
    static std::mt19937_64 gen(rd());
    static std::uniform_int_distribution<uint64_t> dis;
    
    uint64_t a = dis(gen);
    uint64_t b = dis(gen);
    
    // Set version 4 (random) and variant 1
    a = (a & 0xFFFFFFFFFFFF0FFFULL) | 0x0000000000004000ULL;
    b = (b & 0x3FFFFFFFFFFFFFFFULL) | 0x8000000000000000ULL;
    
    std::ostringstream oss;
    oss << std::hex << std::setfill('0')
        << std::setw(8) << ((a >> 32) & 0xFFFFFFFF) << '-'
        << std::setw(4) << ((a >> 16) & 0xFFFF) << '-'
        << std::setw(4) << (a & 0xFFFF) << '-'
        << std::setw(4) << ((b >> 48) & 0xFFFF) << '-'
        << std::setw(12) << (b & 0xFFFFFFFFFFFFULL);
    return oss.str();
}

// ============================================================================
// Path Utilities
// ============================================================================

std::string get_home_directory() {
#ifdef IDA_CHAT_WINDOWS
    char path[MAX_PATH];
    if (SUCCEEDED(SHGetFolderPathA(NULL, CSIDL_PROFILE, NULL, 0, path))) {
        return std::string(path);
    }
    // Fallback
    const char* home = std::getenv("USERPROFILE");
    if (home) return std::string(home);
    return "C:\\";
#else
    const char* home = std::getenv("HOME");
    if (home) return std::string(home);
    
    struct passwd* pw = getpwuid(getuid());
    if (pw) return std::string(pw->pw_dir);
    
    return "/tmp";
#endif
}

std::string get_config_directory() {
    return get_home_directory() + IDA_CHAT_PATH_SEP_STR ".ida-chat";
}

std::string get_sessions_directory() {
    return get_config_directory() + IDA_CHAT_PATH_SEP_STR "sessions";
}

bool ensure_directory_exists(const std::string& path) {
#ifdef IDA_CHAT_WINDOWS
    DWORD attrs = GetFileAttributesA(path.c_str());
    if (attrs != INVALID_FILE_ATTRIBUTES && (attrs & FILE_ATTRIBUTE_DIRECTORY)) {
        return true;
    }
    
    // Create parent directories recursively
    size_t pos = path.find_last_of("\\/");
    if (pos != std::string::npos && pos > 0) {
        ensure_directory_exists(path.substr(0, pos));
    }
    
    return CreateDirectoryA(path.c_str(), NULL) || GetLastError() == ERROR_ALREADY_EXISTS;
#else
    struct stat st;
    if (stat(path.c_str(), &st) == 0 && S_ISDIR(st.st_mode)) {
        return true;
    }
    
    // Create parent directories recursively
    size_t pos = path.find_last_of('/');
    if (pos != std::string::npos && pos > 0) {
        (void)ensure_directory_exists(path.substr(0, pos));
    }
    
    return mkdir(path.c_str(), 0755) == 0 || errno == EEXIST;
#endif
}

// ============================================================================
// Base64 URL Encoding
// ============================================================================

static const char base64_url_chars[] =
    "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789-_";

std::string base64_url_encode(const std::string& input) {
    std::string result;
    result.reserve(((input.size() + 2) / 3) * 4);
    
    const unsigned char* data = reinterpret_cast<const unsigned char*>(input.data());
    size_t i = 0;
    
    while (i < input.size()) {
        uint32_t octet_a = i < input.size() ? data[i++] : 0;
        uint32_t octet_b = i < input.size() ? data[i++] : 0;
        uint32_t octet_c = i < input.size() ? data[i++] : 0;
        
        uint32_t triple = (octet_a << 16) + (octet_b << 8) + octet_c;
        
        result += base64_url_chars[(triple >> 18) & 0x3F];
        result += base64_url_chars[(triple >> 12) & 0x3F];
        result += base64_url_chars[(triple >> 6) & 0x3F];
        result += base64_url_chars[triple & 0x3F];
    }
    
    // Remove padding (URL-safe base64 doesn't need padding)
    size_t padding = (3 - input.size() % 3) % 3;
    if (padding > 0) {
        result.resize(result.size() - padding);
    }
    
    return result;
}

std::string base64_url_decode(const std::string& input) {
    static const int decode_table[256] = {
        -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
        -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
        -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,62,-1,-1,
        52,53,54,55,56,57,58,59,60,61,-1,-1,-1,-1,-1,-1,
        -1, 0, 1, 2, 3, 4, 5, 6, 7, 8, 9,10,11,12,13,14,
        15,16,17,18,19,20,21,22,23,24,25,-1,-1,-1,-1,63,
        -1,26,27,28,29,30,31,32,33,34,35,36,37,38,39,40,
        41,42,43,44,45,46,47,48,49,50,51,-1,-1,-1,-1,-1,
        -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
        -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
        -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
        -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
        -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
        -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
        -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
        -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
    };
    
    // Pad input to multiple of 4
    std::string padded = input;
    while (padded.size() % 4 != 0) {
        padded += '=';
    }
    
    std::string result;
    result.reserve((padded.size() / 4) * 3);
    
    for (size_t i = 0; i < padded.size(); i += 4) {
        int a = decode_table[static_cast<unsigned char>(padded[i])];
        int b = decode_table[static_cast<unsigned char>(padded[i + 1])];
        int c = decode_table[static_cast<unsigned char>(padded[i + 2])];
        int d = decode_table[static_cast<unsigned char>(padded[i + 3])];
        
        if (a < 0 || b < 0) break;
        
        result += static_cast<char>((a << 2) | (b >> 4));
        
        if (c >= 0) {
            result += static_cast<char>(((b & 0xF) << 4) | (c >> 2));
            if (d >= 0) {
                result += static_cast<char>(((c & 0x3) << 6) | d);
            }
        }
    }
    
    return result;
}

// ============================================================================
// String Utilities
// ============================================================================

std::string trim(const std::string& s) {
    auto start = s.find_first_not_of(" \t\n\r\f\v");
    if (start == std::string::npos) return "";
    auto end = s.find_last_not_of(" \t\n\r\f\v");
    return s.substr(start, end - start + 1);
}

std::vector<std::string> split(const std::string& s, char delimiter) {
    std::vector<std::string> result;
    std::istringstream iss(s);
    std::string token;
    while (std::getline(iss, token, delimiter)) {
        result.push_back(token);
    }
    return result;
}

std::string join(const std::vector<std::string>& parts, const std::string& delimiter) {
    std::string result;
    for (size_t i = 0; i < parts.size(); ++i) {
        if (i > 0) result += delimiter;
        result += parts[i];
    }
    return result;
}

std::string replace_all(const std::string& s, const std::string& from, const std::string& to) {
    if (from.empty()) return s;
    std::string result = s;
    size_t pos = 0;
    while ((pos = result.find(from, pos)) != std::string::npos) {
        result.replace(pos, from.length(), to);
        pos += to.length();
    }
    return result;
}

std::string html_escape(const std::string& s) {
    std::string result;
    result.reserve(s.size() * 1.1);
    for (char c : s) {
        switch (c) {
            case '&':  result += "&amp;"; break;
            case '<':  result += "&lt;"; break;
            case '>':  result += "&gt;"; break;
            case '"':  result += "&quot;"; break;
            case '\'': result += "&#39;"; break;
            default:   result += c; break;
        }
    }
    return result;
}

// ============================================================================
// File I/O Utilities
// ============================================================================

std::optional<std::string> read_file(const std::string& path) {
    std::ifstream file(path, std::ios::binary);
    if (!file) return std::nullopt;
    
    std::ostringstream oss;
    oss << file.rdbuf();
    return oss.str();
}

bool write_file(const std::string& path, const std::string& contents) {
    std::ofstream file(path, std::ios::binary | std::ios::trunc);
    if (!file) return false;
    file << contents;
    return file.good();
}

bool append_to_file(const std::string& path, const std::string& contents) {
    std::ofstream file(path, std::ios::binary | std::ios::app);
    if (!file) return false;
    file << contents;
    return file.good();
}

bool file_exists(const std::string& path) {
#ifdef IDA_CHAT_WINDOWS
    DWORD attrs = GetFileAttributesA(path.c_str());
    return attrs != INVALID_FILE_ATTRIBUTES;
#else
    struct stat st;
    return stat(path.c_str(), &st) == 0;
#endif
}

std::vector<std::string> list_files(const std::string& directory, const std::string& extension) {
    std::vector<std::string> result;
    
#ifdef IDA_CHAT_WINDOWS
    std::string pattern = directory + "\\*" + extension;
    WIN32_FIND_DATAA fd;
    HANDLE hFind = FindFirstFileA(pattern.c_str(), &fd);
    if (hFind != INVALID_HANDLE_VALUE) {
        do {
            if (!(fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)) {
                result.push_back(directory + "\\" + fd.cFileName);
            }
        } while (FindNextFileA(hFind, &fd));
        FindClose(hFind);
    }
#else
    DIR* dir = opendir(directory.c_str());
    if (dir) {
        struct dirent* entry;
        while ((entry = readdir(dir)) != nullptr) {
            std::string name = entry->d_name;
            if (name == "." || name == "..") continue;
            if (!extension.empty() && !ends_with(name, extension)) continue;
            result.push_back(directory + "/" + name);
        }
        closedir(dir);
    }
#endif
    
    // Sort by modification time (newest first)
    std::sort(result.begin(), result.end(), [](const std::string& a, const std::string& b) {
#ifdef IDA_CHAT_WINDOWS
        WIN32_FILE_ATTRIBUTE_DATA dataA, dataB;
        if (GetFileAttributesExA(a.c_str(), GetFileExInfoStandard, &dataA) &&
            GetFileAttributesExA(b.c_str(), GetFileExInfoStandard, &dataB)) {
            return CompareFileTime(&dataA.ftLastWriteTime, &dataB.ftLastWriteTime) > 0;
        }
        return false;
#else
        struct stat stA, stB;
        if (stat(a.c_str(), &stA) == 0 && stat(b.c_str(), &stB) == 0) {
            return stA.st_mtime > stB.st_mtime;
        }
        return false;
#endif
    });
    
    return result;
}

} // namespace ida_chat
