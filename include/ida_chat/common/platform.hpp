/**
 * @file platform.hpp
 * @brief Platform detection and compatibility macros.
 */

#pragma once

// Platform detection
#if defined(_WIN32) || defined(_WIN64)
    #define IDA_CHAT_WINDOWS 1
    #define IDA_CHAT_PLATFORM_NAME "Windows"
#elif defined(__APPLE__)
    #define IDA_CHAT_MACOS 1
    #define IDA_CHAT_PLATFORM_NAME "macOS"
#elif defined(__linux__)
    #define IDA_CHAT_LINUX 1
    #define IDA_CHAT_PLATFORM_NAME "Linux"
#else
    #error "Unsupported platform"
#endif

// Architecture detection
#if defined(__x86_64__) || defined(_M_X64)
    #define IDA_CHAT_X64 1
    #define IDA_CHAT_ARCH_NAME "x64"
#elif defined(__aarch64__) || defined(_M_ARM64)
    #define IDA_CHAT_ARM64 1
    #define IDA_CHAT_ARCH_NAME "arm64"
#else
    #error "Unsupported architecture"
#endif

// Portable file system functions
#ifdef IDA_CHAT_WINDOWS
    #include <io.h>
    #include <direct.h>
    #define IDA_CHAT_PATH_SEP '\\'
    #define IDA_CHAT_PATH_SEP_STR "\\"
    #define IDA_CHAT_MKDIR(path) _mkdir(path)
    #define IDA_CHAT_ACCESS(path, mode) _access(path, mode)
    #define IDA_CHAT_GETENV(name) _dupenv_s
#else
    #include <unistd.h>
    #include <sys/stat.h>
    #define IDA_CHAT_PATH_SEP '/'
    #define IDA_CHAT_PATH_SEP_STR "/"
    #define IDA_CHAT_MKDIR(path) mkdir(path, 0755)
    #define IDA_CHAT_ACCESS(path, mode) access(path, mode)
#endif

// Thread-local storage
#ifdef IDA_CHAT_WINDOWS
    #define IDA_CHAT_THREAD_LOCAL __declspec(thread)
#else
    #define IDA_CHAT_THREAD_LOCAL __thread
#endif

// Export/import macros
#ifdef IDA_CHAT_WINDOWS
    #ifdef IDA_CHAT_BUILDING
        #define IDA_CHAT_API __declspec(dllexport)
    #else
        #define IDA_CHAT_API __declspec(dllimport)
    #endif
#else
    #define IDA_CHAT_API __attribute__((visibility("default")))
#endif

// Force inline
#ifdef _MSC_VER
    #define IDA_CHAT_FORCE_INLINE __forceinline
#else
    #define IDA_CHAT_FORCE_INLINE __attribute__((always_inline)) inline
#endif

// Likely/unlikely branch hints
#ifdef __GNUC__
    #define IDA_CHAT_LIKELY(x)   __builtin_expect(!!(x), 1)
    #define IDA_CHAT_UNLIKELY(x) __builtin_expect(!!(x), 0)
#else
    #define IDA_CHAT_LIKELY(x)   (x)
    #define IDA_CHAT_UNLIKELY(x) (x)
#endif
