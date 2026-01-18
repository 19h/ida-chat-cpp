/**
 * @file plugin.cpp
 * @brief IDA plugin entry point and main plugin class.
 */

#include <ida_chat/plugin/plugin.hpp>
#include <ida_chat/plugin/action_handlers.hpp>
#include <ida_chat/plugin/settings.hpp>

namespace ida_chat {

// ============================================================================
// IDAChatPlugin Implementation
// ============================================================================

IDAChatPlugin::IDAChatPlugin() = default;

IDAChatPlugin::~IDAChatPlugin() {
    // Cleanup
    unregister_actions();
    detach_from_menus();
}

bool idaapi IDAChatPlugin::run(size_t arg) {
    // Toggle widget visibility on manual invocation
    toggle_widget();
    return true;
}

void IDAChatPlugin::toggle_widget() {
    if (!form_) {
        form_ = std::make_unique<IDAChatForm>();
    }
    
    if (form_->is_visible()) {
        form_->hide();
    } else {
        form_->show();
    }
}

void IDAChatPlugin::show_widget() {
    if (!form_) {
        form_ = std::make_unique<IDAChatForm>();
    }
    form_->show();
}

void IDAChatPlugin::hide_widget() {
    if (form_) {
        form_->hide();
    }
}

// ============================================================================
// Plugin Initialization
// ============================================================================

plugmod_t* idaapi init() {
    // Check if we're running in IDA (not idat batch mode)
    if (!is_idaq()) {
        msg("IDA Chat: Skipping initialization in batch mode\n");
        return nullptr;
    }
    
    msg("IDA Chat v%s initializing...\n", PLUGIN_VERSION);
    
    auto* plugin = new IDAChatPlugin();
    
    // Register actions
    if (!register_actions(plugin)) {
        msg("IDA Chat: Failed to register actions\n");
        delete plugin;
        return nullptr;
    }
    
    // Attach to menus
    attach_to_menus();
    
    // Apply saved auth settings
    apply_auth_to_environment();
    
    msg("IDA Chat: Ready (press %s to toggle)\n", PLUGIN_HOTKEY);
    
    return plugin;
}

} // namespace ida_chat

// ============================================================================
// Plugin Descriptor
// ============================================================================

plugin_t PLUGIN = {
    IDP_INTERFACE_VERSION,
    PLUGIN_MULTI | PLUGIN_FIX,  // Multi-instance, keep loaded
    ida_chat::init,              // Initialize
    nullptr,                     // Terminate (handled by destructor)
    nullptr,                     // Run (handled by plugmod_t::run)
    ida_chat::PLUGIN_COMMENT,   // Comment
    ida_chat::PLUGIN_HELP,      // Help
    ida_chat::PLUGIN_NAME,      // Wanted name
    ida_chat::PLUGIN_HOTKEY     // Hotkey
};
