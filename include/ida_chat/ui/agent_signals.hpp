/**
 * @file agent_signals.hpp
 * @brief Qt signals for agent callbacks.
 */

#pragma once

#include <ida_chat/core/types.hpp>

#include <QObject>
#include <QString>

namespace ida_chat {

/**
 * @brief Qt signals for agent callbacks.
 * 
 * Used to safely communicate between the agent worker thread
 * and the UI thread in Qt.
 */
class AgentSignals : public QObject {
    Q_OBJECT

public:
    explicit AgentSignals(QObject* parent = nullptr) : QObject(parent) {}

signals:
    /**
     * @brief Emitted at the start of each turn.
     * @param turn Current turn number
     * @param max_turns Maximum turns allowed
     */
    void turn_start(int turn, int max_turns);
    
    /**
     * @brief Emitted when agent starts thinking.
     */
    void thinking();
    
    /**
     * @brief Emitted when agent finishes thinking.
     */
    void thinking_done();
    
    /**
     * @brief Emitted when agent uses a tool.
     * @param tool_name Name of the tool
     * @param details Description of the tool use
     */
    void tool_use(const QString& tool_name, const QString& details);
    
    /**
     * @brief Emitted when agent outputs text.
     * @param text The text output
     */
    void text(const QString& text);
    
    /**
     * @brief Emitted with script code before execution.
     * @param code The Python code
     */
    void script_code(const QString& code);
    
    /**
     * @brief Emitted with script output after execution.
     * @param output The script output
     */
    void script_output(const QString& output);
    
    /**
     * @brief Emitted when an error occurs.
     * @param error The error message
     */
    void error(const QString& error);
    
    /**
     * @brief Emitted when processing is complete.
     * @param num_turns Number of turns taken
     * @param cost Estimated cost in USD
     */
    void result(int num_turns, double cost);
    
    /**
     * @brief Emitted when agent worker finishes.
     */
    void finished();
    
    /**
     * @brief Emitted when connection is ready.
     */
    void connection_ready();
    
    /**
     * @brief Emitted when connection fails.
     * @param error Error message
     */
    void connection_error(const QString& error);
};

} // namespace ida_chat
