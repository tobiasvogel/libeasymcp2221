#ifndef MCP2221_EXPORT_H
#define MCP2221_EXPORT_H

/*
 * Public symbol visibility.
 *
 * LIBEASYMCP2221_BUILDING_LIBRARY is defined only while compiling the shared
 * library. LIBEASYMCP2221_STATIC is defined for static builds and must also be
 * defined by Windows consumers that link the static library directly.
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

#ifdef __cplusplus
#  define MCP2221_BEGIN_DECLS extern "C" {
#  define MCP2221_END_DECLS }
#else
#  define MCP2221_BEGIN_DECLS
#  define MCP2221_END_DECLS
#endif

#endif /* MCP2221_EXPORT_H */
