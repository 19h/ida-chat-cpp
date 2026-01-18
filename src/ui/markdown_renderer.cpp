/**
 * @file markdown_renderer.cpp
 * @brief Markdown to HTML converter for chat display.
 */

#include <ida_chat/ui/markdown_renderer.hpp>

#include <QApplication>
#include <QPalette>
#include <QRegularExpression>

namespace ida_chat {

// ============================================================================
// ColorScheme
// ============================================================================

ColorScheme ColorScheme::from_ida_palette() {
    ColorScheme colors;
    
    QApplication* app = qApp;
    if (!app) {
        return dark_default();
    }
    
    QPalette palette = app->palette();
    
    colors.window = palette.color(QPalette::Window);
    colors.window_text = palette.color(QPalette::WindowText);
    colors.base = palette.color(QPalette::Base);
    colors.alternate_base = palette.color(QPalette::AlternateBase);
    colors.text = palette.color(QPalette::Text);
    colors.button = palette.color(QPalette::Button);
    colors.button_text = palette.color(QPalette::ButtonText);
    colors.highlight = palette.color(QPalette::Highlight);
    colors.highlight_text = palette.color(QPalette::HighlightedText);
    colors.mid = palette.color(QPalette::Mid);
    colors.dark = palette.color(QPalette::Dark);
    colors.light = palette.color(QPalette::Light);
    
    return colors;
}

ColorScheme ColorScheme::dark_default() {
    ColorScheme colors;
    
    colors.window = QColor("#1e1e1e");
    colors.window_text = QColor("#d4d4d4");
    colors.base = QColor("#252526");
    colors.alternate_base = QColor("#2d2d30");
    colors.text = QColor("#d4d4d4");
    colors.button = QColor("#3c3c3c");
    colors.button_text = QColor("#d4d4d4");
    colors.highlight = QColor("#264f78");
    colors.highlight_text = QColor("#ffffff");
    colors.mid = QColor("#808080");
    colors.dark = QColor("#1e1e1e");
    colors.light = QColor("#3c3c3c");
    
    return colors;
}

ColorScheme ColorScheme::light_default() {
    ColorScheme colors;
    
    colors.window = QColor("#f3f3f3");
    colors.window_text = QColor("#1e1e1e");
    colors.base = QColor("#ffffff");
    colors.alternate_base = QColor("#f5f5f5");
    colors.text = QColor("#1e1e1e");
    colors.button = QColor("#e1e1e1");
    colors.button_text = QColor("#1e1e1e");
    colors.highlight = QColor("#0078d4");
    colors.highlight_text = QColor("#ffffff");
    colors.mid = QColor("#a0a0a0");
    colors.dark = QColor("#d0d0d0");
    colors.light = QColor("#ffffff");
    
    return colors;
}

// ============================================================================
// MarkdownRenderer
// ============================================================================

MarkdownRenderer::MarkdownRenderer(const ColorScheme& colors)
    : colors_(colors) {}

void MarkdownRenderer::set_colors(const ColorScheme& colors) {
    colors_ = colors;
}

QString MarkdownRenderer::escape_html(const QString& text) {
    QString result = text;
    result.replace("&", "&amp;");
    result.replace("<", "&lt;");
    result.replace(">", "&gt;");
    result.replace("\"", "&quot;");
    return result;
}

QString MarkdownRenderer::render_inline_code(const QString& code) const {
    QString escaped = escape_html(code);
    return QString("<code style=\"background-color: %1; color: %2; padding: 2px 4px; border-radius: 3px;\">%3</code>")
        .arg(colors_.dark.name())
        .arg(colors_.text.name())
        .arg(escaped);
}

QString MarkdownRenderer::render_code_block(const QString& code, const QString& /*language*/) const {
    QString escaped = escape_html(code);
    return QString("<pre style=\"background-color: %1; color: %2; padding: 8px; border-radius: 4px; overflow-x: auto;\"><code>%3</code></pre>")
        .arg(colors_.dark.name())
        .arg(colors_.text.name())
        .arg(escaped);
}

QString MarkdownRenderer::render_inline(const QString& text) const {
    QString result = text;
    
    // Inline code (`code`) - must be before other formatting
    // Qt doesn't support replace with lambda, so we use QRegularExpressionMatchIterator
    static QRegularExpression inlineCodeRe("`([^`]+)`");
    QRegularExpressionMatchIterator it = inlineCodeRe.globalMatch(result);
    QList<QPair<int, int>> replacements;  // offset, length pairs
    QList<QString> replaceWith;
    
    while (it.hasNext()) {
        QRegularExpressionMatch match = it.next();
        replacements.append(qMakePair(match.capturedStart(), match.capturedLength()));
        replaceWith.append(render_inline_code(match.captured(1)));
    }
    
    // Apply replacements in reverse order to preserve offsets
    for (int i = replacements.size() - 1; i >= 0; --i) {
        result.replace(replacements[i].first, replacements[i].second, replaceWith[i]);
    }
    
    // Bold (**text** or __text__)
    static QRegularExpression boldRe1("\\*\\*(.+?)\\*\\*");
    static QRegularExpression boldRe2("__(.+?)__");
    result.replace(boldRe1, "<b>\\1</b>");
    result.replace(boldRe2, "<b>\\1</b>");
    
    // Italic (*text* or _text_) - careful not to match inside words
    static QRegularExpression italicRe1("(?<!\\w)\\*([^*]+)\\*(?!\\w)");
    static QRegularExpression italicRe2("(?<!\\w)_([^_]+)_(?!\\w)");
    result.replace(italicRe1, "<i>\\1</i>");
    result.replace(italicRe2, "<i>\\1</i>");
    
    // Links [text](url)
    static QRegularExpression linkRe("\\[([^\\]]+)\\]\\(([^)]+)\\)");
    result.replace(linkRe, "<a href=\"\\2\">\\1</a>");
    
    return result;
}

QString MarkdownRenderer::render_line(const QString& line) const {
    // Headers
    if (line.startsWith("### ")) {
        return QString("<h4>%1</h4>").arg(render_inline(escape_html(line.mid(4))));
    }
    if (line.startsWith("## ")) {
        return QString("<h3>%1</h3>").arg(render_inline(escape_html(line.mid(3))));
    }
    if (line.startsWith("# ")) {
        return QString("<h2>%1</h2>").arg(render_inline(escape_html(line.mid(2))));
    }
    
    // Bullet lists (- item or * item)
    if (line.startsWith("- ") || line.startsWith("* ")) {
        return QString("<li>%1</li>").arg(render_inline(escape_html(line.mid(2))));
    }
    
    // Numbered lists (1. item)
    static QRegularExpression numberedListRe("^(\\d+)\\. (.+)$");
    QRegularExpressionMatch match = numberedListRe.match(line);
    if (match.hasMatch()) {
        return QString("<li>%1</li>").arg(render_inline(escape_html(match.captured(2))));
    }
    
    // Regular text
    return render_inline(escape_html(line));
}

QString MarkdownRenderer::render(const QString& markdown) const {
    QString result;
    QStringList lines = markdown.split('\n');
    
    bool in_code_block = false;
    QString code_block_content;
    QString code_block_language;
    bool in_list = false;
    
    for (int i = 0; i < lines.size(); ++i) {
        const QString& line = lines[i];
        
        // Code blocks (``` ... ```)
        if (line.startsWith("```")) {
            if (in_code_block) {
                // End code block
                result += render_code_block(code_block_content, code_block_language);
                code_block_content.clear();
                code_block_language.clear();
                in_code_block = false;
            } else {
                // Start code block
                code_block_language = line.mid(3).trimmed();
                in_code_block = true;
            }
            continue;
        }
        
        if (in_code_block) {
            if (!code_block_content.isEmpty()) {
                code_block_content += '\n';
            }
            code_block_content += line;
            continue;
        }
        
        // Handle list wrapping
        bool is_list_item = line.startsWith("- ") || line.startsWith("* ") || 
                           QRegularExpression("^\\d+\\. ").match(line).hasMatch();
        
        if (is_list_item && !in_list) {
            result += "<ul>";
            in_list = true;
        } else if (!is_list_item && in_list && !line.trimmed().isEmpty()) {
            result += "</ul>";
            in_list = false;
        }
        
        // Render the line
        QString rendered = render_line(line);
        result += rendered;
        
        // Add line break for non-empty lines (not inside lists)
        if (!is_list_item && !line.isEmpty()) {
            result += "<br>";
        }
    }
    
    // Close any unclosed code block
    if (in_code_block) {
        result += render_code_block(code_block_content, code_block_language);
    }
    
    // Close any unclosed list
    if (in_list) {
        result += "</ul>";
    }
    
    // Clean up multiple <br> tags
    static QRegularExpression multiBrRe("(<br>){3,}");
    result.replace(multiBrRe, "<br><br>");
    
    return result;
}

// ============================================================================
// Global instance
// ============================================================================

MarkdownRenderer& get_markdown_renderer() {
    static MarkdownRenderer instance;
    return instance;
}

} // namespace ida_chat
