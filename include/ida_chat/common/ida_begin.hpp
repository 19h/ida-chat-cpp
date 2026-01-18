/**
 * @file ida_begin.hpp
 * @brief Begin IDA SDK header inclusion section.
 *
 * This header handles the Qt/IDA symbol conflicts.
 * Both Qt and IDA define qstrlen, qstrncmp, qsnprintf, etc.
 * We rename IDA's versions using macros before including IDA headers.
 *
 * IMPORTANT: Include ALL Qt headers BEFORE this header!
 *
 * Usage:
 *   #include <QWidget>  // Qt first!
 *   #include <QString>
 *
 *   #include <ida_chat/common/ida_begin.hpp>
 *   #include <ida.hpp>
 *   #include <kernwin.hpp>
 *   #include <ida_chat/common/ida_end.hpp>
 */

// Disable warnings (same as warn_off.hpp)
#ifdef _MSC_VER
#pragma warning(push)
#pragma warning(disable: 4244)
#pragma warning(disable: 4267)
#pragma warning(disable: 4146)
#pragma warning(disable: 4018)
#pragma warning(disable: 4458)
#pragma warning(disable: 4100)
#pragma warning(disable: 4127)
#pragma warning(disable: 4996)
#endif

#ifdef __GNUC__
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wswitch"
#pragma GCC diagnostic ignored "-Wpedantic"
#pragma GCC diagnostic ignored "-Wunused-parameter"
#pragma GCC diagnostic ignored "-Wsign-compare"
#pragma GCC diagnostic ignored "-Wconversion"
#pragma GCC diagnostic ignored "-Wdeprecated-declarations"
#ifdef __clang__
#pragma clang diagnostic ignored "-Wshorten-64-to-32"
#pragma clang diagnostic ignored "-Wdocumentation"
#pragma clang diagnostic ignored "-Wdocumentation-unknown-command"
#endif
#endif

// Always rename IDA's conflicting symbols to avoid any conflict with Qt
// regardless of whether Qt has been included yet.
// This is the ONLY reliable way to avoid the Qt/IDA symbol conflicts.
#define IDA_CHAT_QT_CONFLICT_MODE 1

// Rename IDA's inline functions to avoid conflict with Qt's
// IDA's pro.h defines these as inline functions
#define qstrlen     ida_qstrlen
#define qstrncmp    ida_qstrncmp

// IDA's pro.h exports these functions  
#define qsnprintf   ida_qsnprintf
#define qvsnprintf  ida_qvsnprintf
#define qstrdup     ida_qstrdup
#define qstrncpy    ida_qstrncpy
