/**
 * @file warn_off.hpp
 * @brief Disable compiler warnings before including IDA SDK headers.
 * 
 * Usage:
 *   #include <ida_chat/common/warn_off.hpp>
 *   #include <ida.hpp>
 *   #include <hexrays.hpp>
 *   #include <ida_chat/common/warn_on.hpp>
 */

#pragma once

#ifdef _MSC_VER
#pragma warning(push)
#pragma warning(disable: 4244)  // conversion, possible loss of data
#pragma warning(disable: 4267)  // conversion from size_t
#pragma warning(disable: 4146)  // unary minus on unsigned
#pragma warning(disable: 4018)  // signed/unsigned mismatch
#pragma warning(disable: 4458)  // declaration hides class member
#pragma warning(disable: 4100)  // unreferenced formal parameter
#pragma warning(disable: 4127)  // conditional expression is constant
#pragma warning(disable: 4996)  // deprecated functions
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
