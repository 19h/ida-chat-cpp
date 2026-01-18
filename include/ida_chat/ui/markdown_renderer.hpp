/**
 * @file markdown_renderer.hpp
 * @brief Markdown to HTML converter for chat display.
 */

#pragma once

#include <QString>
#include <QColor>
#include <QMap>

namespace ida_chat {

/**
 * @brief Color scheme for rendering.
 */
struct ColorScheme {
    QColor window;
    QColor window_text;
    QColor base;
    QColor alternate_base;
    QColor text;
    QColor button;
    QColor button_text;
    QColor highlight;
    QColor highlight_text;
    QColor mid;
    QColor dark;
    QColor light;
    
    /**
     * @brief Get colors from IDA's current palette.
     */
    static ColorScheme from_ida_palette();
    
    /**
     * @brief Get a default dark color scheme.
     */
    static ColorScheme dark_default();
    
    /**
     * @brief Get a default light color scheme.
     */
    static ColorScheme light_default();
};

/**
 * @brief Markdown to HTML renderer.
 */
class MarkdownRenderer {
public:
    explicit MarkdownRenderer(const ColorScheme& colors = ColorScheme::from_ida_palette());
    
    /**
     * @brief Convert markdown text to HTML.
     * @param markdown Input markdown text
     * @return HTML suitable for QLabel rich text display
     */
    [[nodiscard]] QString render(const QString& markdown) const;
    
    /**
     * @brief Set the color scheme.
     */
    void set_colors(const ColorScheme& colors);
    
    /**
     * @brief Get the current color scheme.
     */
    [[nodiscard]] const ColorScheme& colors() const { return colors_; }
    
    /**
     * @brief Render inline code.
     */
    [[nodiscard]] QString render_inline_code(const QString& code) const;
    
    /**
     * @brief Render a code block with syntax highlighting.
     */
    [[nodiscard]] QString render_code_block(const QString& code, 
                                            const QString& language = QString()) const;
    
    /**
     * @brief Escape HTML special characters.
     */
    [[nodiscard]] static QString escape_html(const QString& text);

private:
    QString render_line(const QString& line) const;
    QString render_inline(const QString& text) const;
    
    ColorScheme colors_;
};

/**
 * @brief Global markdown renderer instance.
 */
[[nodiscard]] MarkdownRenderer& get_markdown_renderer();

/**
 * @brief Convenience function to convert markdown to HTML.
 */
[[nodiscard]] inline QString markdown_to_html(const QString& markdown) {
    return get_markdown_renderer().render(markdown);
}

} // namespace ida_chat
