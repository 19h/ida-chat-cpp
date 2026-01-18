/**
 * @file action_handlers.hpp
 * @brief IDA action handlers for plugin hotkeys and menu items.
 */

#pragma once

// NOTE: This header includes IDA SDK headers. If you need to include this
// in a file that also uses Qt, you MUST include Qt headers (like <QString>)
// BEFORE including this header.

#include <ida_chat/common/ida_begin.hpp>
#include <ida.hpp>
#include <kernwin.hpp>
#include <ida_chat/common/ida_end.hpp>

namespace ida_chat {

// Forward declaration
class IDAChatPlugin;

/**
 * @brief Action handler to toggle the chat widget.
 */
class ToggleWidgetHandler : public action_handler_t {
public:
    explicit ToggleWidgetHandler(IDAChatPlugin* plugin);
    
    int idaapi activate(action_activation_ctx_t* ctx) override;
    action_state_t idaapi update(action_update_ctx_t* ctx) override;
    
private:
    IDAChatPlugin* plugin_;
};

/**
 * @brief Action names.
 */
namespace actions {
    constexpr const char* TOGGLE_WIDGET = "ida_chat:toggle_widget";
}

/**
 * @brief Register all plugin actions.
 * @param plugin Plugin instance for callbacks
 * @return true if all actions registered successfully
 */
bool register_actions(IDAChatPlugin* plugin);

/**
 * @brief Unregister all plugin actions.
 */
void unregister_actions();

/**
 * @brief Attach actions to IDA menus.
 */
void attach_to_menus();

/**
 * @brief Detach actions from IDA menus.
 */
void detach_from_menus();

} // namespace ida_chat
