/**
 * @file mcp2221_export.h
 * @brief Public symbol-visibility and C/C++ linkage helpers.
 */

#ifndef MCP2221_EXPORT_H
#define MCP2221_EXPORT_H

/*
 * Public symbol visibility.
 *
 * LIBEASYMCP2221_BUILDING_LIBRARY is defined only while compiling the shared
 * library. LIBEASYMCP2221_STATIC is defined for static builds and must also be
 * defined by Windows consumers that link the static library directly.
 */

/**
 * @def MCP2221_API
 * @brief Export or import a public libeasymcp2221 API symbol.
 *
 * The exact expansion depends on the compiler, platform, and whether the
 * shared or static library is being built or consumed.
 */
#if defined(_WIN32) || defined(__CYGWIN__)
#  if defined(LIBEASYMCP2221_BUILDING_LIBRARY)
#    define MCP2221_API __declspec(dllexport)
#  elif defined(LIBEASYMCP2221_STATIC)
#    define MCP2221_API
#  else
#    define MCP2221_API __declspec(dllimport)
#  endif
#elif defined(__GNUC__) || defined(__clang__)
#  define MCP2221_API __attribute__((visibility("default")))
#else
#  define MCP2221_API
#endif

/**
 * @def MCP2221_BEGIN_DECLS
 * @brief Begin a block of declarations using C linkage.
 *
 * Expands to `extern "C" {` when included from C++ and to nothing in C.
 */

/**
 * @def MCP2221_END_DECLS
 * @brief End a block started by MCP2221_BEGIN_DECLS.
 *
 * Expands to `}` when included from C++ and to nothing in C.
 */
#ifdef __cplusplus
#  define MCP2221_BEGIN_DECLS extern "C" {
#  define MCP2221_END_DECLS }
#else
#  define MCP2221_BEGIN_DECLS
#  define MCP2221_END_DECLS
#endif

#endif /* MCP2221_EXPORT_H */
