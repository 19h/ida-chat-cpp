# IDA Chat C++ Plugin

A C++ port of the ida-chat-plugin - an AI-powered chat assistant for IDA Pro using Claude.

## Overview

IDA Chat provides a dockable Qt-based chat interface for interacting with Claude AI during reverse engineering sessions. The agent can execute Python scripts in IDA's environment, analyze binary code, and provide intelligent assistance.

## Features

- **Dockable Chat UI**: Qt-based interface that integrates with IDA Pro
- **Claude Integration**: Direct communication with Claude API via streaming
- **Script Execution**: Execute Python code in IDA's scripting environment
- **Agentic Loop**: Autonomous multi-turn conversations with script feedback
- **Message History**: Persistent JSONL-based session storage
- **Transcript Export**: Export conversations to HTML

## Building

### Prerequisites

- CMake 3.20+
- C++20 compatible compiler (GCC 10+, Clang 12+, MSVC 2019+)
- IDA Pro SDK (9.0+)
- libcurl
- Qt 5.15+ or Qt 6 (provided by IDA)

### Build Steps

```bash
# Set IDA SDK path
export IDASDK=/path/to/idasdk

# Create build directory
mkdir build && cd build

# Configure
cmake ..

# Build
cmake --build .

# Install to IDA plugins directory
cmake --install . --prefix /path/to/ida/plugins
```

### Build Options

- `IDA_CHAT_BUILD_TESTS`: Build unit tests (default: OFF)
- `IDA_CHAT_USE_SYSTEM_CURL`: Use system libcurl (default: ON)
- `IDA_CHAT_USE_SYSTEM_JSON`: Use system nlohmann_json (default: OFF)

## Usage

1. Load the plugin in IDA Pro
2. Press `Ctrl+Shift+C` or use View menu to toggle the chat panel
3. Configure authentication in the setup wizard
4. Start chatting with Claude about your binary!

### Authentication

IDA Chat supports three authentication methods:

1. **System**: Uses existing Claude Code authentication
2. **OAuth**: OAuth token-based authentication
3. **API Key**: Direct Anthropic API key

## Architecture

The plugin follows a layered architecture:

1. **Plugin Layer** (`plugin/`): IDA plugin registration and action handlers
2. **UI Layer** (`ui/`): Qt widgets for the chat interface
3. **Core Layer** (`core/`): Chat engine with agentic loop
4. **API Layer** (`api/`): Claude API client with streaming support
5. **History Layer** (`history/`): JSONL session persistence

Key design patterns:
- PIMPL idiom for ABI stability
- Qt signals/slots for thread-safe UI updates
- Streaming HTTP for real-time responses
- SSE parsing for Claude's streaming format

## License

MIT License - see LICENSE file.

## Credits

Ported from [ida-chat-plugin](https://github.com/hex-rays/ida-chat-plugin) (Python).

Reference projects:
- [chernobog](https://github.com/example/chernobog) - IDA deobfuscation plugin patterns
- [structor](https://github.com/example/structor) - IDA structure synthesis patterns
