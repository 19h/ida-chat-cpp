/**
 * @file ida_compat.hpp
 * @brief Compatibility layer for IDA SDK and Qt header conflicts.
 *
 * IDA SDK and Qt define conflicting symbols (qstrlen, qstrncmp, qsnprintf, etc.).
 * IDA SDK also redefines standard C functions as macros (fgetc, snprintf, etc.).
 *
 * This header provides macros to properly isolate these conflicts.
 *
 * Usage for files that need BOTH Qt and IDA:
 *
 *   // FIRST: Include Qt headers (before IDA)
 *   #include <QWidget>
 *   #include <QString>
 *
 *   // THEN: Begin IDA section
 *   #include <ida_chat/common/ida_begin.hpp>
 *   #include <ida.hpp>
 *   #include <kernwin.hpp>
 *   #include <ida_chat/common/ida_end.hpp>
 *
 * For files that ONLY need IDA (no Qt):
 *   #include <ida_chat/common/warn_off.hpp>
 *   #include <ida.hpp>
 *   #include <ida_chat/common/warn_on.hpp>
 *
 * For nlohmann/json:
 *   #include <ida_chat/common/json.hpp>  // Must be before any IDA headers
 */

#pragma once

// Detect if we're being compiled with Qt
#if defined(QT_CORE_LIB) || defined(Q_MOC_OUTPUT_REVISION)
#define IDA_CHAT_HAS_QT 1
#endif

/**
 * IDA SDK defines these as macros in fpro.h which break standard usage:
 */
#define IDA_CHAT_SAVE_MACROS() \
    _Pragma("push_macro(\"fgetc\")") \
    _Pragma("push_macro(\"fputc\")") \
    _Pragma("push_macro(\"fread\")") \
    _Pragma("push_macro(\"fwrite\")") \
    _Pragma("push_macro(\"fseek\")") \
    _Pragma("push_macro(\"ftell\")") \
    _Pragma("push_macro(\"fgets\")") \
    _Pragma("push_macro(\"fputs\")") \
    _Pragma("push_macro(\"fprintf\")") \
    _Pragma("push_macro(\"snprintf\")") \
    _Pragma("push_macro(\"vsnprintf\")")

#define IDA_CHAT_UNDEF_MACROS() \
    do { \
    } while(0)

// Note: The above is empty because we handle undefs individually
// The real undefs happen in ida_begin.hpp after the push

#define IDA_CHAT_RESTORE_MACROS() \
    _Pragma("pop_macro(\"fgetc\")") \
    _Pragma("pop_macro(\"fputc\")") \
    _Pragma("pop_macro(\"fread\")") \
    _Pragma("pop_macro(\"fwrite\")") \
    _Pragma("pop_macro(\"fseek\")") \
    _Pragma("pop_macro(\"ftell\")") \
    _Pragma("pop_macro(\"fgets\")") \
    _Pragma("pop_macro(\"fputs\")") \
    _Pragma("pop_macro(\"fprintf\")") \
    _Pragma("pop_macro(\"snprintf\")") \
    _Pragma("pop_macro(\"vsnprintf\")")

// End of ida_compat.hpp
