/**
 * @file plugin.hpp
 * @brief IDA plugin main entry point.
 */

#pragma once

// IMPORTANT: ALL Qt headers must be included BEFORE any IDA headers.
// ida_chat_form.hpp and its dependencies use many Qt headers.
// Include the main Qt headers that define the conflicting symbols first.
#include <QWidget>
#include <QObject>
#include <QString>
#include <QByteArray>
#include <QThread>
#include <QMutex>
#include <QScrollArea>
#include <QFrame>
#include <QTextEdit>

// Now we can include ida_chat_form.hpp which uses Qt
#include <ida_chat/ui/ida_chat_form.hpp>

// Finally include IDA headers with our compatibility wrapper
#include <ida_chat/common/ida_begin.hpp>
#include <ida.hpp>
#include <idp.hpp>
#include <loader.hpp>
#include <kernwin.hpp>
#include <ida_chat/common/ida_end.hpp>

#include <memory>

namespace ida_chat {

/**
 * @brief Main IDA plugin class.
 */
class IDAChatPlugin : public plugmod_t {
public:
    IDAChatPlugin();
    ~IDAChatPlugin() override;
    
    /**
     * @brief Plugin run callback.
     * @param arg Argument passed to the plugin
     * @return true if run was successful
     */
    bool idaapi run(size_t arg) override;
    
    /**
     * @brief Toggle the chat widget visibility.
     */
    void toggle_widget();
    
    /**
     * @brief Show the chat widget.
     */
    void show_widget();
    
    /**
     * @brief Hide the chat widget.
     */
    void hide_widget();

private:
    std::unique_ptr<IDAChatForm> form_;
    bool initialized_ = false;
};

/**
 * @brief Plugin initialization function.
 * @return Plugin module pointer or nullptr to skip
 */
plugmod_t* idaapi init();

} // namespace ida_chat

// Global plugin descriptor
extern plugin_t PLUGIN;
