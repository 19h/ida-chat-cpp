/**
 * @file action_handlers.cpp
 * @brief IDA action handlers implementation.
 */

// Include Qt headers first to avoid conflicts with IDA
#include <QString>

#include <ida_chat/plugin/action_handlers.hpp>
#include <ida_chat/plugin/plugin.hpp>
#include <ida_chat/core/fwd.hpp>

namespace ida_chat {

// ============================================================================
// ToggleWidgetHandler
// ============================================================================

ToggleWidgetHandler::ToggleWidgetHandler(IDAChatPlugin* plugin)
    : plugin_(plugin) {}

int idaapi ToggleWidgetHandler::activate(action_activation_ctx_t* ctx) {
    if (plugin_) {
        plugin_->toggle_widget();
    }
    return 1;
}

action_state_t idaapi ToggleWidgetHandler::update(action_update_ctx_t* ctx) {
    // Always enabled
    return AST_ENABLE_ALWAYS;
}

// ============================================================================
// Static handler instances
// ============================================================================

static std::unique_ptr<ToggleWidgetHandler> g_toggle_handler;

// ============================================================================
// Registration Functions
// ============================================================================

bool register_actions(IDAChatPlugin* plugin) {
    // Create handler
    g_toggle_handler = std::make_unique<ToggleWidgetHandler>(plugin);
    
    // Register toggle action
    action_desc_t toggle_desc = ACTION_DESC_LITERAL(
        actions::TOGGLE_WIDGET,
        "Toggle IDA Chat",
        g_toggle_handler.get(),
        PLUGIN_HOTKEY,
        "Show or hide the IDA Chat panel",
        -1
    );
    
    if (!register_action(toggle_desc)) {
        return false;
    }
    
    return true;
}

void unregister_actions() {
    unregister_action(actions::TOGGLE_WIDGET);
    g_toggle_handler.reset();
}

void attach_to_menus() {
    // Attach to View menu
    attach_action_to_menu(
        "View/",
        actions::TOGGLE_WIDGET,
        SETMENU_APP
    );
}

void detach_from_menus() {
    detach_action_from_menu(
        "View/",
        actions::TOGGLE_WIDGET
    );
}

} // namespace ida_chat
