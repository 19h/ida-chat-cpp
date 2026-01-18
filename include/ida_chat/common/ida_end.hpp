/**
 * @file ida_end.hpp
 * @brief End IDA SDK header inclusion section.
 *
 * Restores compiler warning settings and undoes symbol renames.
 */

// Always undo IDA symbol renames (we always define them in ida_begin.hpp)
#ifdef IDA_CHAT_QT_CONFLICT_MODE
#undef qstrlen
#undef qstrncmp
#undef qsnprintf
#undef qvsnprintf
#undef qstrdup
#undef qstrncpy
#undef IDA_CHAT_QT_CONFLICT_MODE
#endif

// Restore warnings
#ifdef _MSC_VER
#pragma warning(pop)
#endif

#ifdef __GNUC__
#pragma GCC diagnostic pop
#endif
