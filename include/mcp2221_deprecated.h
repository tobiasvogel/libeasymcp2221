#ifndef MCP2221_DEPRECATED_H
#define MCP2221_DEPRECATED_H

/* Portable deprecation marker for legacy API names.
 *
 * Deprecated names are compatibility aliases for the 1.x series and may be
 * removed in the next major version. Define MCP2221_NO_DEPRECATED_WARNINGS
 * before including the public headers to keep compatibility declarations
 * without compiler warnings during migration.
 */
#if defined(MCP2221_NO_DEPRECATED_WARNINGS)
#define MCP2221_DEPRECATED(message)
#elif defined(__clang__) || defined(__GNUC__)
#define MCP2221_DEPRECATED(message) __attribute__((deprecated(message)))
#elif defined(_MSC_VER)
#define MCP2221_DEPRECATED(message) __declspec(deprecated(message))
#else
#define MCP2221_DEPRECATED(message)
#endif

#endif /* MCP2221_DEPRECATED_H */
