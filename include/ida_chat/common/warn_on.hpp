/**
 * @file warn_on.hpp
 * @brief Re-enable compiler warnings after IDA SDK headers.
 * 
 * Usage:
 *   #include <ida_chat/common/warn_off.hpp>
 *   #include <ida.hpp>
 *   #include <hexrays.hpp>
 *   #include <ida_chat/common/warn_on.hpp>
 */

#pragma once

#ifdef _MSC_VER
#pragma warning(pop)
#endif

#ifdef __GNUC__
#pragma GCC diagnostic pop
// Keep some warnings disabled globally
#pragma GCC diagnostic ignored "-Wformat"
#endif
