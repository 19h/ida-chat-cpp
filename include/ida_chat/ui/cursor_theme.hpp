/**
 * @file cursor_theme.hpp
 * @brief Cursor-style dark theme color palette.
 * 
 * Colors extracted from Cursor IDE mockups for consistent styling.
 */

#pragma once

#include <QString>
#include <QStringList>

namespace ida_chat {
namespace theme {

// ============================================================================
// Background Colors (darkest to lightest)
// ============================================================================

inline const QString BG_BASE = "#0a0a0a";           // Main window background
inline const QString BG_SIDEBAR = "#0f0f0f";        // Sidebar background  
inline const QString BG_CARD = "#141414";           // Card/item background
inline const QString BG_CARD_HOVER = "#1a1a1a";     // Card hover state
inline const QString BG_ELEVATED = "#1e1e1e";       // Elevated surfaces, modals
inline const QString BG_INPUT = "#141414";          // Input field background
inline const QString BG_CODE = "#0d0d0d";           // Code block background

// Message bubbles
inline const QString BG_USER_MESSAGE = "#0f2a4a";   // User message (dark blue tint)
inline const QString BG_ASSISTANT = "transparent";  // Assistant message (no bg)
inline const QString BG_TOOL_ACTION = "#1a1a1a";    // Tool action indicator
inline const QString BG_FILE_BLOCK = "#141414";     // File change block

// ============================================================================
// Text Colors
// ============================================================================

inline const QString TEXT_PRIMARY = "#f5f5f5";      // Primary text (headings, important)
inline const QString TEXT_SECONDARY = "#a0a0a0";    // Secondary text (body)
inline const QString TEXT_MUTED = "#606060";        // Muted text (timestamps, hints)
inline const QString TEXT_PLACEHOLDER = "#4a4a4a";  // Placeholder text
inline const QString TEXT_CODE = "#e5e5e5";         // Code text

// ============================================================================
// Accent Colors
// ============================================================================

inline const QString ACCENT_GREEN = "#22c55e";      // Additions, success, checkmarks
inline const QString ACCENT_RED = "#ef4444";        // Deletions, errors
inline const QString ACCENT_BLUE = "#3b82f6";       // Links, selections, user messages
inline const QString ACCENT_YELLOW = "#eab308";     // Warnings
inline const QString ACCENT_PURPLE = "#a855f7";     // Special highlights

// ============================================================================
// Border Colors
// ============================================================================

inline const QString BORDER_SUBTLE = "#1f1f1f";     // Subtle borders (separators)
inline const QString BORDER_DEFAULT = "#2a2a2a";    // Default borders
inline const QString BORDER_FOCUS = "#3b82f6";      // Focus ring

// ============================================================================
// Semantic Colors
// ============================================================================

inline const QString COLOR_SUCCESS = ACCENT_GREEN;
inline const QString COLOR_ERROR = ACCENT_RED;
inline const QString COLOR_WARNING = ACCENT_YELLOW;
inline const QString COLOR_INFO = ACCENT_BLUE;

// ============================================================================
// Spinner Animation Frames
// ============================================================================

inline const QStringList SPINNER_FRAMES = {
    "⠋", "⠙", "⠹", "⠸", "⠼", "⠴", "⠦", "⠧", "⠇", "⠏"
};

// ============================================================================
// Style Helpers
// ============================================================================

// Common border radius
inline const QString RADIUS_SM = "4px";
inline const QString RADIUS_MD = "6px";
inline const QString RADIUS_LG = "8px";

// Font sizes
inline const QString FONT_XS = "11px";
inline const QString FONT_SM = "12px";
inline const QString FONT_BASE = "13px";
inline const QString FONT_LG = "14px";

} // namespace theme
} // namespace ida_chat
