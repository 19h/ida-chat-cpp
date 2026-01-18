/**
 * @file json.hpp
 * @brief Wrapper for nlohmann/json that handles IDA SDK macro conflicts.
 * 
 * IDA's fpro.h redefines standard C I/O functions as macros (fgetc, snprintf, etc.)
 * which breaks nlohmann/json. This header saves these macros, includes json,
 * then restores them.
 *
 * IMPORTANT: This header can be included either before OR after IDA headers.
 */

#pragma once

// Save all the macros that IDA might have defined
#ifdef fgetc
#define IDA_CHAT_HAD_FGETC
#pragma push_macro("fgetc")
#undef fgetc
#endif

#ifdef fputc
#define IDA_CHAT_HAD_FPUTC
#pragma push_macro("fputc")
#undef fputc
#endif

#ifdef fread
#define IDA_CHAT_HAD_FREAD
#pragma push_macro("fread")
#undef fread
#endif

#ifdef fwrite
#define IDA_CHAT_HAD_FWRITE
#pragma push_macro("fwrite")
#undef fwrite
#endif

#ifdef fgets
#define IDA_CHAT_HAD_FGETS
#pragma push_macro("fgets")
#undef fgets
#endif

#ifdef fputs
#define IDA_CHAT_HAD_FPUTS
#pragma push_macro("fputs")
#undef fputs
#endif

#ifdef fprintf
#define IDA_CHAT_HAD_FPRINTF
#pragma push_macro("fprintf")
#undef fprintf
#endif

#ifdef snprintf
#define IDA_CHAT_HAD_SNPRINTF
#pragma push_macro("snprintf")
#undef snprintf
#endif

#ifdef vsnprintf
#define IDA_CHAT_HAD_VSNPRINTF
#pragma push_macro("vsnprintf")
#undef vsnprintf
#endif

// Now include nlohmann/json with clean standard library
#include <nlohmann/json.hpp>

// Restore all the macros
#ifdef IDA_CHAT_HAD_FGETC
#pragma pop_macro("fgetc")
#undef IDA_CHAT_HAD_FGETC
#endif

#ifdef IDA_CHAT_HAD_FPUTC
#pragma pop_macro("fputc")
#undef IDA_CHAT_HAD_FPUTC
#endif

#ifdef IDA_CHAT_HAD_FREAD
#pragma pop_macro("fread")
#undef IDA_CHAT_HAD_FREAD
#endif

#ifdef IDA_CHAT_HAD_FWRITE
#pragma pop_macro("fwrite")
#undef IDA_CHAT_HAD_FWRITE
#endif

#ifdef IDA_CHAT_HAD_FGETS
#pragma pop_macro("fgets")
#undef IDA_CHAT_HAD_FGETS
#endif

#ifdef IDA_CHAT_HAD_FPUTS
#pragma pop_macro("fputs")
#undef IDA_CHAT_HAD_FPUTS
#endif

#ifdef IDA_CHAT_HAD_FPRINTF
#pragma pop_macro("fprintf")
#undef IDA_CHAT_HAD_FPRINTF
#endif

#ifdef IDA_CHAT_HAD_SNPRINTF
#pragma pop_macro("snprintf")
#undef IDA_CHAT_HAD_SNPRINTF
#endif

#ifdef IDA_CHAT_HAD_VSNPRINTF
#pragma pop_macro("vsnprintf")
#undef IDA_CHAT_HAD_VSNPRINTF
#endif
