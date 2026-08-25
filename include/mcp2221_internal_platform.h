#ifndef MCP2221_INTERNAL_PLATFORM_H
#define MCP2221_INTERNAL_PLATFORM_H

#if defined(_MSC_VER)

#include <stdint.h>
#include <windows.h>

typedef SRWLOCK mcp2221_platform_mutex_t;
#define MCP2221_PLATFORM_MUTEX_INITIALIZER SRWLOCK_INIT

static inline void mcp2221_platform_mutex_lock(
	mcp2221_platform_mutex_t *mutex) {
	AcquireSRWLockExclusive(mutex);
}

static inline void mcp2221_platform_mutex_unlock(
	mcp2221_platform_mutex_t *mutex) {
	ReleaseSRWLockExclusive(mutex);
}

static inline double mcp2221_platform_monotonic_seconds(void) {
	LARGE_INTEGER frequency;
	LARGE_INTEGER counter;

	if (QueryPerformanceFrequency(&frequency) &&
	    QueryPerformanceCounter(&counter)) {
		return (double)counter.QuadPart / (double)frequency.QuadPart;
	}

	return (double)GetTickCount64() / 1000.0;
}

static inline void mcp2221_platform_sleep_ms(unsigned int milliseconds) {
	Sleep((DWORD)milliseconds);
}

static inline double mcp2221_platform_wall_time_seconds(void) {
	static const ULONGLONG filetime_unix_epoch_100ns =
		116444736000000000ULL;
	static const double filetime_ticks_per_second = 10000000.0;
	FILETIME filetime;
	ULARGE_INTEGER ticks;

	GetSystemTimeAsFileTime(&filetime);
	ticks.LowPart = filetime.dwLowDateTime;
	ticks.HighPart = filetime.dwHighDateTime;

	if (ticks.QuadPart < filetime_unix_epoch_100ns)
		return 0.0;

	return (double)(ticks.QuadPart - filetime_unix_epoch_100ns) /
	       filetime_ticks_per_second;
}

#else

#include <pthread.h>
#include <time.h>

typedef pthread_mutex_t mcp2221_platform_mutex_t;
#define MCP2221_PLATFORM_MUTEX_INITIALIZER PTHREAD_MUTEX_INITIALIZER

static inline void mcp2221_platform_mutex_lock(
	mcp2221_platform_mutex_t *mutex) {
	pthread_mutex_lock(mutex);
}

static inline void mcp2221_platform_mutex_unlock(
	mcp2221_platform_mutex_t *mutex) {
	pthread_mutex_unlock(mutex);
}

static inline double mcp2221_platform_monotonic_seconds(void) {
	struct timespec ts;
	clock_gettime(CLOCK_MONOTONIC, &ts);
	return ts.tv_sec + ts.tv_nsec / 1e9;
}

static inline void mcp2221_platform_sleep_ms(unsigned int milliseconds) {
	struct timespec ts = {
		(time_t)(milliseconds / 1000u),
		(long)(milliseconds % 1000u) * 1000L * 1000L,
	};
	nanosleep(&ts, NULL);
}

static inline double mcp2221_platform_wall_time_seconds(void) {
#if defined(CLOCK_REALTIME)
	struct timespec ts;
	clock_gettime(CLOCK_REALTIME, &ts);
	return ts.tv_sec + ts.tv_nsec / 1e9;
#else
	return (double)time(NULL);
#endif
}

#endif

#endif /* MCP2221_INTERNAL_PLATFORM_H */
