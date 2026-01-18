/**
 * @file fwd.hpp
 * @brief Forward declarations for all ida_chat types.
 */

#pragma once

#include <memory>
#include <cstdint>

namespace ida_chat {

// Core types
class Config;
class ScriptExecutor;
class ChatCore;

// Chat callback interface
class ChatCallback;
class PluginCallback;

// API types
class HttpClient;
class ClaudeClient;
class StreamingParser;
struct ClaudeMessage;
struct ClaudeResponse;
struct ClaudeToolUse;
struct ClaudeToolResult;
struct TokenUsage;

// History types
class MessageHistory;
class SessionManager;
struct HistoryMessage;
struct SessionInfo;

// UI types (Qt)
class ChatMessage;
class ChatHistoryWidget;
class ChatInputWidget;
class ProgressTimeline;
class CollapsibleSection;
class OnboardingPanel;
class IDAChatForm;
class AgentWorker;
class AgentSignals;
class MarkdownRenderer;

// Smart pointer aliases
using ConfigPtr = std::shared_ptr<Config>;
using HttpClientPtr = std::shared_ptr<HttpClient>;
using ClaudeClientPtr = std::shared_ptr<ClaudeClient>;
using MessageHistoryPtr = std::shared_ptr<MessageHistory>;
using SessionManagerPtr = std::shared_ptr<SessionManager>;
using ChatCorePtr = std::shared_ptr<ChatCore>;

// Constants
constexpr const char* PLUGIN_NAME = "IDA Chat";
constexpr const char* PLUGIN_COMMENT = "AI Chat Assistant for IDA Pro";
constexpr const char* PLUGIN_HELP = "Claude-powered reverse engineering assistant";
constexpr const char* PLUGIN_HOTKEY = "Ctrl+Shift+C";
constexpr const char* PLUGIN_VERSION = IDA_CHAT_VERSION;

constexpr int DEFAULT_MAX_TURNS = 20;
constexpr int DEFAULT_TIMEOUT_MS = 120000;

} // namespace ida_chat
