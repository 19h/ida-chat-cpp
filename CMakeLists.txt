cmake_minimum_required(VERSION 3.20)
project(ida_chat VERSION 0.2.6 LANGUAGES CXX)

# ============================================================================
# Build options
# ============================================================================
option(IDA_CHAT_BUILD_TESTS "Build unit tests" OFF)
option(IDA_CHAT_USE_SYSTEM_CURL "Use system libcurl instead of bundled" ON)
option(IDA_CHAT_USE_SYSTEM_JSON "Use system nlohmann_json instead of bundled" OFF)

# ============================================================================
# C++ Standard
# ============================================================================
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# ============================================================================
# IDA SDK Integration via ida-cmake
# ============================================================================
if(NOT DEFINED ENV{IDASDK})
    message(FATAL_ERROR "IDASDK environment variable must be set to the IDA SDK path")
endif()
set(IDASDK $ENV{IDASDK})
include(${CMAKE_CURRENT_LIST_DIR}/ida-cmake/bootstrap.cmake)
find_package(idasdk REQUIRED)

# ============================================================================
# Dependencies
# ============================================================================

# libcurl for HTTP requests
if(IDA_CHAT_USE_SYSTEM_CURL)
    find_package(CURL REQUIRED)
else()
    include(FetchContent)
    FetchContent_Declare(
        curl
        GIT_REPOSITORY https://github.com/curl/curl.git
        GIT_TAG curl-8_5_0
    )
    set(BUILD_CURL_EXE OFF CACHE BOOL "" FORCE)
    set(BUILD_SHARED_LIBS OFF CACHE BOOL "" FORCE)
    set(CURL_STATICLIB ON CACHE BOOL "" FORCE)
    FetchContent_MakeAvailable(curl)
endif()

# nlohmann/json for JSON parsing
if(IDA_CHAT_USE_SYSTEM_JSON)
    find_package(nlohmann_json REQUIRED)
else()
    include(FetchContent)
    FetchContent_Declare(
        json
        GIT_REPOSITORY https://github.com/nlohmann/json.git
        GIT_TAG v3.11.3
    )
    FetchContent_MakeAvailable(json)
endif()

# ============================================================================
# Source files
# ============================================================================
set(IDA_CHAT_SOURCES
    # Qt compatibility
    src/common/qt_compat.cpp
    
    # Core types and utilities
    src/core/types.cpp
    src/core/script_executor.cpp
    src/core/chat_core.cpp
    src/core/chat_callback.cpp
    
    # API layer (Claude API client)
    src/api/http_client.cpp
    src/api/claude_client.cpp
    src/api/claude_types.cpp
    src/api/streaming_parser.cpp
    src/api/keychain.cpp
    src/api/cli_transport.cpp
    
    # Message history persistence
    src/history/message_history.cpp
    src/history/session_manager.cpp
    
    # UI components
    src/ui/chat_message.cpp
    src/ui/chat_history_widget.cpp
    src/ui/chat_input_widget.cpp
    src/ui/progress_timeline.cpp
    src/ui/collapsible_section.cpp
    src/ui/onboarding_panel.cpp
    src/ui/ida_chat_form.cpp
    src/ui/markdown_renderer.cpp
    src/ui/agent_worker.cpp
    
    # Cursor-style UI (new)
    src/ui/task_sidebar.cpp
    src/ui/cursor_chat_view.cpp
    src/ui/cursor_input.cpp
    
    # Plugin entry point
    src/plugin/plugin.cpp
    src/plugin/action_handlers.cpp
    src/plugin/settings.cpp
)

set(IDA_CHAT_HEADERS
    # Common includes
    include/ida_chat/common/warn_off.hpp
    include/ida_chat/common/warn_on.hpp
    include/ida_chat/common/platform.hpp
    
    # Core
    include/ida_chat/core/fwd.hpp
    include/ida_chat/core/types.hpp
    include/ida_chat/core/script_executor.hpp
    include/ida_chat/core/chat_core.hpp
    include/ida_chat/core/chat_callback.hpp
    
    # API
    include/ida_chat/api/http_client.hpp
    include/ida_chat/api/claude_client.hpp
    include/ida_chat/api/claude_types.hpp
    include/ida_chat/api/streaming_parser.hpp
    include/ida_chat/api/keychain.hpp
    include/ida_chat/api/cli_transport.hpp
    
    # History
    include/ida_chat/history/message_history.hpp
    include/ida_chat/history/session_manager.hpp
    
    # UI
    include/ida_chat/ui/chat_message.hpp
    include/ida_chat/ui/chat_history_widget.hpp
    include/ida_chat/ui/chat_input_widget.hpp
    include/ida_chat/ui/progress_timeline.hpp
    include/ida_chat/ui/collapsible_section.hpp
    include/ida_chat/ui/onboarding_panel.hpp
    include/ida_chat/ui/ida_chat_form.hpp
    include/ida_chat/ui/markdown_renderer.hpp
    include/ida_chat/ui/agent_worker.hpp
    include/ida_chat/ui/agent_signals.hpp
    
    # Cursor-style UI (new)
    include/ida_chat/ui/cursor_theme.hpp
    include/ida_chat/ui/task_sidebar.hpp
    include/ida_chat/ui/cursor_chat_view.hpp
    include/ida_chat/ui/cursor_input.hpp
    
    # Plugin
    include/ida_chat/plugin/plugin.hpp
    include/ida_chat/plugin/action_handlers.hpp
    include/ida_chat/plugin/settings.hpp
)

# ============================================================================
# Qt Configuration - Use Qt 6.8.3 to match IDA's bundled Qt
# ============================================================================
# IDA bundles Qt 6.8.x. We MUST use matching Qt headers to avoid ABI issues.

# Use Qt 6.8.3 from user installation (matches IDA's Qt version)
set(QT_6_8_DIR "/Users/int/Qt-6.8.3")
if(EXISTS "${QT_6_8_DIR}/lib/cmake/Qt6")
    list(INSERT CMAKE_PREFIX_PATH 0 "${QT_6_8_DIR}/lib/cmake")
    message(STATUS "Using Qt 6.8.3 from: ${QT_6_8_DIR}")
endif()

# Find Qt 6.8.3
find_package(Qt6 COMPONENTS Core Gui Widgets REQUIRED)
set(QT_VERSION_MAJOR 6)
message(STATUS "Found Qt ${Qt6_VERSION}")

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC OFF)
set(CMAKE_AUTOUIC OFF)

# On macOS, link against IDA's bundled Qt frameworks for runtime compatibility
if(APPLE)
    set(IDA_QT_DIR "/Applications/IDA Professional 9.3.app/Contents/Frameworks")
    if(EXISTS "${IDA_QT_DIR}/QtCore.framework")
        set(USE_IDA_QT ON)
        message(STATUS "Will link against IDA's Qt from: ${IDA_QT_DIR}")
    endif()
endif()

# ============================================================================
# Plugin target
# ============================================================================
# Include MOC headers explicitly for AUTOMOC to process them
ida_add_plugin(ida_chat
    SOURCES 
        ${IDA_CHAT_SOURCES}
        # Qt MOC headers (these contain Q_OBJECT and need MOC processing)
        include/ida_chat/ui/chat_message.hpp
        include/ida_chat/ui/chat_history_widget.hpp
        include/ida_chat/ui/chat_input_widget.hpp
        include/ida_chat/ui/progress_timeline.hpp
        include/ida_chat/ui/collapsible_section.hpp
        include/ida_chat/ui/onboarding_panel.hpp
        include/ida_chat/ui/ida_chat_form.hpp
        include/ida_chat/ui/agent_worker.hpp
        include/ida_chat/ui/agent_signals.hpp
        # Cursor-style UI (new)
        include/ida_chat/ui/task_sidebar.hpp
        include/ida_chat/ui/cursor_chat_view.hpp
        include/ida_chat/ui/cursor_input.hpp
)

target_include_directories(ida_chat
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/include
        ${CMAKE_CURRENT_BINARY_DIR}  # For MOC generated files
)

# Link libraries - use IDA's Qt on macOS, system Qt elsewhere
target_link_libraries(ida_chat
    PRIVATE
        CURL::libcurl
        nlohmann_json::nlohmann_json
)

# Qt linking - link against IDA's bundled Qt (not system Qt)
if(APPLE AND USE_IDA_QT)
    # Create interface library to link IDA's Qt frameworks
    add_library(ida_qt_link INTERFACE)
    target_link_options(ida_qt_link INTERFACE
        "LINKER:-rpath,${IDA_QT_DIR}"
    )
    target_link_libraries(ida_qt_link INTERFACE
        "${IDA_QT_DIR}/QtWidgets.framework/Versions/A/QtWidgets"
        "${IDA_QT_DIR}/QtGui.framework/Versions/A/QtGui"
        "${IDA_QT_DIR}/QtCore.framework/Versions/A/QtCore"
    )
    target_link_libraries(ida_chat PRIVATE ida_qt_link)
    
    # Qt include directories from system Qt (headers only)
    get_target_property(QT_CORE_INCLUDES Qt${QT_VERSION_MAJOR}::Core INTERFACE_INCLUDE_DIRECTORIES)
    get_target_property(QT_GUI_INCLUDES Qt${QT_VERSION_MAJOR}::Gui INTERFACE_INCLUDE_DIRECTORIES)
    get_target_property(QT_WIDGETS_INCLUDES Qt${QT_VERSION_MAJOR}::Widgets INTERFACE_INCLUDE_DIRECTORIES)
    target_include_directories(ida_chat SYSTEM PRIVATE 
        ${QT_CORE_INCLUDES} 
        ${QT_GUI_INCLUDES}
        ${QT_WIDGETS_INCLUDES}
    )
    
    set_target_properties(ida_chat PROPERTIES
        INSTALL_RPATH "@executable_path/../Frameworks"
    )
    message(STATUS "Using IDA's Qt frameworks from: ${IDA_QT_DIR}")
else()
    # Fall back to system Qt (may have issues on macOS)
    target_link_libraries(ida_chat PRIVATE
        Qt${QT_VERSION_MAJOR}::Core
        Qt${QT_VERSION_MAJOR}::Widgets
    )
endif()

# Compiler definitions
# IMPORTANT: IDA's Qt uses QT_NAMESPACE=QT, all symbols are in the QT:: namespace
target_compile_definitions(ida_chat
    PRIVATE
        IDA_CHAT_VERSION="${PROJECT_VERSION}"
        IDA_CHAT_VERSION_MAJOR=${PROJECT_VERSION_MAJOR}
        IDA_CHAT_VERSION_MINOR=${PROJECT_VERSION_MINOR}
        IDA_CHAT_VERSION_PATCH=${PROJECT_VERSION_PATCH}
        QT_NAMESPACE=QT
)

# Platform-specific settings
if(APPLE)
    target_compile_definitions(ida_chat PRIVATE __MAC__)
    target_link_libraries(ida_chat PRIVATE "-framework Security" "-framework CoreFoundation")
elseif(WIN32)
    target_compile_definitions(ida_chat PRIVATE __NT__)
    target_link_libraries(ida_chat PRIVATE ws2_32 crypt32)
else()
    target_compile_definitions(ida_chat PRIVATE __LINUX__)
endif()

# ============================================================================
# Installation
# ============================================================================
# Copy project directory (prompts, docs) alongside the plugin
install(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/project/
    DESTINATION ${CMAKE_INSTALL_PREFIX}/plugins/ida_chat_project
)

# ============================================================================
# Tests (optional)
# ============================================================================
if(IDA_CHAT_BUILD_TESTS)
    enable_testing()
    add_subdirectory(test)
endif()

# ============================================================================
# Documentation
# ============================================================================
set(IDA_CHAT_DOCS
    README.md
    CLAUDE.md
)
