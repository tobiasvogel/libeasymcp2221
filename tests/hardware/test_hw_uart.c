#define _POSIX_C_SOURCE 200809L
#define _XOPEN_SOURCE 700

#include <errno.h>
#ifdef __linux__
#include <dirent.h>
#include <limits.h>
#endif
#include <fcntl.h>
#include <poll.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#ifdef __linux__
#include <strings.h>
#endif
#include <termios.h>
#include <time.h>
#include <unistd.h>

#include "mcp2221_constants.h"

#define UART_TIMEOUT_MS 2000
#define UART_SETTLE_MS 100
#define UART_MAX_TEST_SIZE 512
#define CTEST_SKIP_RETURN_CODE 77


#ifdef __linux__
#define SYS_TTY_DIR "/sys/class/tty"
#define DEV_DIR "/dev"
#define SERIAL_BY_ID_DIR "/dev/serial/by-id"

static int read_attr(const char *path, char *buf, size_t size) {
	FILE *f = fopen(path, "r");
	if (!f)
		return -1;

	if (!fgets(buf, size, f)) {
		fclose(f);
		return -1;
	}
	fclose(f);

	buf[strcspn(buf, "\r\n")] = '\0';
	return 0;
}

static int parse_env_u16(const char *name, uint16_t fallback, uint16_t *value) {
	const char *text = getenv(name);
	if (!text || !*text) {
		*value = fallback;
		return 0;
	}

	errno = 0;
	char *end = NULL;
	unsigned long parsed = strtoul(text, &end, 0);
	if (errno != 0 || end == text || *end != '\0' || parsed > UINT16_MAX)
		return -1;

	*value = (uint16_t)parsed;
	return 0;
}

static int find_usb_parent(
	const char *tty, char *parent, size_t parent_size) {
	char path[PATH_MAX];
	char resolved[PATH_MAX];

	if (snprintf(path, sizeof(path), SYS_TTY_DIR "/%s/device", tty) >=
	    (int)sizeof(path))
		return -1;
	if (!realpath(path, resolved))
		return -1;
	if (strlen(resolved) + 1 > parent_size)
		return -1;

	strcpy(parent, resolved);

	for (;;) {
		char vendor_path[PATH_MAX];
		char product_path[PATH_MAX];
		char *slash;

		if (snprintf(vendor_path, sizeof(vendor_path),
		             "%s/idVendor", parent) >= (int)sizeof(vendor_path))
			return -1;
		if (snprintf(product_path, sizeof(product_path),
		             "%s/idProduct", parent) >= (int)sizeof(product_path))
			return -1;

		if (access(vendor_path, R_OK) == 0 &&
		    access(product_path, R_OK) == 0)
			return 0;

		slash = strrchr(parent, '/');
		if (!slash || slash == parent)
			break;
		*slash = '\0';
	}

	return -1;
}

static int tty_matches_device(
	const char *tty, uint16_t wanted_vid, uint16_t wanted_pid,
	const char *wanted_serial) {
	char parent[PATH_MAX];
	char path[PATH_MAX];
	char text[128];
	char *end = NULL;

	if (find_usb_parent(tty, parent, sizeof(parent)) != 0)
		return 0;

	if (snprintf(path, sizeof(path), "%s/idVendor", parent) >= (int)sizeof(path))
		return 0;
	if (read_attr(path, text, sizeof(text)) != 0)
		return 0;

	errno = 0;
	unsigned long vid = strtoul(text, &end, 16);
	if (errno != 0 || end == text || *end != '\0' || vid != wanted_vid)
		return 0;

	if (snprintf(path, sizeof(path), "%s/idProduct", parent) >= (int)sizeof(path))
		return 0;
	if (read_attr(path, text, sizeof(text)) != 0)
		return 0;

	errno = 0;
	end = NULL;
	unsigned long pid = strtoul(text, &end, 16);
	if (errno != 0 || end == text || *end != '\0' || pid != wanted_pid)
		return 0;

	if (wanted_serial && *wanted_serial) {
		if (snprintf(path, sizeof(path), "%s/serial", parent) >=
		    (int)sizeof(path))
			return 0;
		if (read_attr(path, text, sizeof(text)) != 0)
			return 0;
		if (strcmp(text, wanted_serial) != 0)
			return 0;
	}

	return 1;
}

/*
 * Return 0 for one match, 1 for none, 2 for multiple matches, and -1 on an
 * internal scanning error.
 */
static int find_matching_tty(
	char *device, size_t device_size,
	uint16_t vid, uint16_t pid, const char *serial) {
	DIR *dir = opendir(SYS_TTY_DIR);
	if (!dir)
		return -1;

	int matches = 0;
	struct dirent *ent;
	while ((ent = readdir(dir)) != NULL) {
		if (strncmp(ent->d_name, "ttyACM", 6) != 0)
			continue;
		if (!tty_matches_device(ent->d_name, vid, pid, serial))
			continue;

		++matches;
		if (matches == 1) {
			if (snprintf(device, device_size, DEV_DIR "/%s", ent->d_name) >=
			    (int)device_size) {
				closedir(dir);
				return -1;
			}
		}
	}

	closedir(dir);
	if (matches == 0)
		return 1;
	if (matches > 1)
		return 2;
	return 0;
}

static int find_by_id_for_device(
	const char *device, char *result, size_t result_size) {
	char target_device[PATH_MAX];
	if (!realpath(device, target_device))
		return -1;

	DIR *dir = opendir(SERIAL_BY_ID_DIR);
	if (!dir) {
		if (errno == ENOENT)
			return 1;
		return -1;
	}

	struct dirent *ent;
	while ((ent = readdir(dir)) != NULL) {
		if (strcmp(ent->d_name, ".") == 0 ||
		    strcmp(ent->d_name, "..") == 0)
			continue;

		char link_path[PATH_MAX];
		char resolved[PATH_MAX];
		if (snprintf(link_path, sizeof(link_path),
		             SERIAL_BY_ID_DIR "/%s", ent->d_name) >=
		    (int)sizeof(link_path))
			continue;
		if (!realpath(link_path, resolved))
			continue;
		if (strcmp(resolved, target_device) != 0)
			continue;
		if (strlen(link_path) + 1 > result_size) {
			closedir(dir);
			return -1;
		}

		strcpy(result, link_path);
		closedir(dir);
		return 0;
	}

	closedir(dir);
	return 1;
}

/*
 * Return 0 for success, 1 for no match, 2 for ambiguous selection,
 * 3 for invalid VID/PID environment configuration, and -1 on scan failure.
 */
static int discover_uart_path(char *result, size_t result_size) {
	uint16_t vid;
	uint16_t pid;
	if (parse_env_u16(
	        "LIBEASYMCP2221_HW_VID",
	        (uint16_t)MCP2221_DEV_DEFAULT_VID, &vid) != 0 ||
	    parse_env_u16(
	        "LIBEASYMCP2221_HW_PID",
	        (uint16_t)MCP2221_DEV_DEFAULT_PID, &pid) != 0)
		return 3;

	const char *serial = getenv("LIBEASYMCP2221_HW_SERIAL");
	char tty_device[PATH_MAX];
	int rc = find_matching_tty(
		tty_device, sizeof(tty_device), vid, pid, serial);
	if (rc != 0)
		return rc;

	rc = find_by_id_for_device(tty_device, result, result_size);
	if (rc == 0)
		return 0;
	if (rc < 0)
		return -1;

	if (strlen(tty_device) + 1 > result_size)
		return -1;
	strcpy(result, tty_device);
	return 0;
}
#endif

static int64_t monotonic_ms(void) {
	struct timespec ts;
	if (clock_gettime(CLOCK_MONOTONIC, &ts) != 0)
		return -1;
	return (int64_t)ts.tv_sec * 1000 + ts.tv_nsec / 1000000;
}

static int sleep_ms(long milliseconds) {
	struct timespec req = {
		.tv_sec = milliseconds / 1000,
		.tv_nsec = (milliseconds % 1000) * 1000000L,
	};
	while (nanosleep(&req, &req) != 0) {
		if (errno != EINTR)
			return -1;
	}
	return 0;
}

static int wait_ready(int fd, short events, int64_t deadline_ms) {
	for (;;) {
		int64_t now = monotonic_ms();
		if (now < 0)
			return -1;
		if (now >= deadline_ms) {
			errno = ETIMEDOUT;
			return -1;
		}

		int64_t remaining = deadline_ms - now;
		int timeout = remaining > INT32_MAX ? INT32_MAX : (int)remaining;
		struct pollfd pfd = {
			.fd = fd,
			.events = events,
			.revents = 0,
		};

		int rc = poll(&pfd, 1, timeout);
		if (rc < 0) {
			if (errno == EINTR)
				continue;
			return -1;
		}
		if (rc == 0) {
			errno = ETIMEDOUT;
			return -1;
		}
		if (pfd.revents & (POLLERR | POLLHUP | POLLNVAL)) {
			errno = EIO;
			return -1;
		}
		if (pfd.revents & events)
			return 0;
	}
}

static int write_all(int fd, const uint8_t *data, size_t len) {
	int64_t start = monotonic_ms();
	if (start < 0)
		return -1;
	int64_t deadline = start + UART_TIMEOUT_MS;

	size_t offset = 0;
	while (offset < len) {
		if (wait_ready(fd, POLLOUT, deadline) != 0)
			return -1;

		ssize_t n = write(fd, data + offset, len - offset);
		if (n < 0) {
			if (errno == EINTR || errno == EAGAIN || errno == EWOULDBLOCK)
				continue;
			return -1;
		}
		if (n == 0)
			continue;
		offset += (size_t)n;
	}
	return 0;
}

static int read_exact(int fd, uint8_t *data, size_t len) {
	int64_t start = monotonic_ms();
	if (start < 0)
		return -1;
	int64_t deadline = start + UART_TIMEOUT_MS;

	size_t offset = 0;
	while (offset < len) {
		if (wait_ready(fd, POLLIN, deadline) != 0)
			return -1;

		ssize_t n = read(fd, data + offset, len - offset);
		if (n < 0) {
			if (errno == EINTR || errno == EAGAIN || errno == EWOULDBLOCK)
				continue;
			return -1;
		}
		if (n == 0)
			continue;
		offset += (size_t)n;
	}
	return 0;
}

static int configure_uart_115200_8n1(int fd) {
	struct termios tio;
	if (tcgetattr(fd, &tio) != 0)
		return -1;

	tio.c_iflag &= ~(IGNBRK | BRKINT | PARMRK | ISTRIP |
	                 INLCR | IGNCR | ICRNL | IXON);
	tio.c_oflag &= ~OPOST;
	tio.c_lflag &= ~(ECHO | ECHONL | ICANON | ISIG | IEXTEN);
	tio.c_cflag &= ~(CSIZE | PARENB | CSTOPB);
	tio.c_cflag |= CS8 | CLOCAL | CREAD;
#ifdef CRTSCTS
	tio.c_cflag &= ~CRTSCTS;
#endif
	tio.c_cc[VMIN] = 0;
	tio.c_cc[VTIME] = 0;

	if (cfsetispeed(&tio, B115200) != 0 ||
	    cfsetospeed(&tio, B115200) != 0)
		return -1;
	if (tcsetattr(fd, TCSANOW, &tio) != 0)
		return -1;
	if (tcflush(fd, TCIOFLUSH) != 0)
		return -1;

	/*
	 * The CDC SET_LINE_CODING request and the UART bridge can settle
	 * asynchronously. Give the device a short, fixed settling interval,
	 * then discard anything that arrived while the new line coding took
	 * effect. This is setup, not a retry of failed test data.
	 */
	if (sleep_ms(UART_SETTLE_MS) != 0)
		return -1;
	if (tcflush(fd, TCIOFLUSH) != 0)
		return -1;
	return 0;
}

static void fill_pattern(uint8_t *data, size_t len, unsigned seed) {
	for (size_t i = 0; i < len; ++i)
		data[i] = (uint8_t)((i * 73u + seed * 29u) & 0xFFu);
}

static void report_mismatch(
	const char *label,
	const uint8_t *tx,
	const uint8_t *rx,
	size_t len) {
	size_t mismatch = 0;
	while (mismatch < len && tx[mismatch] == rx[mismatch])
		++mismatch;

	fprintf(stderr,
	        "test_hw_uart: %s mismatch at byte %zu/%zu: sent 0x%02x, received 0x%02x\n",
	        label, mismatch, len, tx[mismatch], rx[mismatch]);

	size_t start = mismatch > 4 ? mismatch - 4 : 0;
	size_t end = mismatch + 8 < len ? mismatch + 8 : len;
	fprintf(stderr, "test_hw_uart: sent    [%zu..%zu):", start, end);
	for (size_t i = start; i < end; ++i)
		fprintf(stderr, " %02x", tx[i]);
	fputc('\n', stderr);
	fprintf(stderr, "test_hw_uart: received[%zu..%zu):", start, end);
	for (size_t i = start; i < end; ++i)
		fprintf(stderr, " %02x", rx[i]);
	fputc('\n', stderr);
}

static int run_loopback_payload(
	int fd,
	const char *label,
	const uint8_t *tx,
	size_t len) {
	uint8_t rx[UART_MAX_TEST_SIZE];
	if (len > sizeof(rx)) {
		errno = EINVAL;
		return -1;
	}
	memset(rx, 0, len);

	if (tcflush(fd, TCIFLUSH) != 0) {
		perror("test_hw_uart: tcflush input");
		return -1;
	}
	if (write_all(fd, tx, len) != 0) {
		perror("test_hw_uart: write");
		return -1;
	}
	if (read_exact(fd, rx, len) != 0) {
		int read_errno = errno;
		perror("test_hw_uart: read");
		if (read_errno == ETIMEDOUT) {
			if (strcmp(label, "ascii-sanity") == 0) {
				fprintf(stderr,
				        "test_hw_uart: timed out waiting for the initial "
				        "loopback frame; check the UTX-to-URX loopback "
				        "connection\n");
			} else {
				fprintf(stderr,
				        "test_hw_uart: UART loopback stalled while waiting "
				        "for data; check the UTX-to-URX connection and "
				        "physical wiring\n");
			}
		}
		errno = read_errno;
		return -1;
	}
	if (memcmp(tx, rx, len) != 0) {
		report_mismatch(label, tx, rx, len);
		return -1;
	}

	printf("UART loopback %s (%zu bytes): OK\n", label, len);
	return 0;
}

static int run_loopback_case(int fd, size_t len, unsigned seed) {
	uint8_t tx[UART_MAX_TEST_SIZE];
	fill_pattern(tx, len, seed);

	char label[32];
	int n = snprintf(label, sizeof(label), "binary-%zu", len);
	if (n < 0 || (size_t)n >= sizeof(label)) {
		errno = EINVAL;
		return -1;
	}
	return run_loopback_payload(fd, label, tx, len);
}

int main(void) {
	const char *path = getenv("LIBEASYMCP2221_HW_UART");
#ifdef __linux__
	char discovered_path[PATH_MAX];
#endif

	if (!path || !*path) {
#ifdef __linux__
		int rc = discover_uart_path(discovered_path, sizeof(discovered_path));
		if (rc == 0) {
			path = discovered_path;
			printf("test_hw_uart: auto-detected UART device: %s\n", path);
		} else if (rc == 1) {
			fprintf(stderr,
			        "test_hw_uart: no matching MCP2221 CDC UART found; skipping\n");
			return CTEST_SKIP_RETURN_CODE;
		} else if (rc == 2) {
			fprintf(stderr,
			        "test_hw_uart: multiple matching MCP2221 CDC UARTs found; "
			        "set LIBEASYMCP2221_HW_SERIAL or LIBEASYMCP2221_HW_UART; "
			        "skipping\n");
			return CTEST_SKIP_RETURN_CODE;
		} else if (rc == 3) {
			fprintf(stderr,
			        "test_hw_uart: invalid LIBEASYMCP2221_HW_VID or "
			        "LIBEASYMCP2221_HW_PID\n");
			return 1;
		} else {
			perror("test_hw_uart: auto-discovery");
			return 1;
		}
#else
		fprintf(stderr,
		        "test_hw_uart: automatic UART discovery is Linux-only; "
		        "set LIBEASYMCP2221_HW_UART; skipping\n");
		return CTEST_SKIP_RETURN_CODE;
#endif
	}

	int fd = open(path, O_RDWR | O_NOCTTY | O_NONBLOCK);
	if (fd < 0) {
		perror("test_hw_uart: open");
		return 1;
	}

	if (configure_uart_115200_8n1(fd) != 0) {
		perror("test_hw_uart: configure 115200 8N1");
		close(fd);
		return 1;
	}

	static const uint8_t sanity[] = "MCP2221-UART-LOOPBACK";
	if (run_loopback_payload(
	        fd, "ascii-sanity", sanity, sizeof(sanity) - 1u) != 0) {
		close(fd);
		return 1;
	}

	static const size_t lengths[] = {1, 63, 64, 65, 255, 512};
	for (size_t i = 0; i < sizeof(lengths) / sizeof(lengths[0]); ++i) {
		if (run_loopback_case(fd, lengths[i], (unsigned)i + 1u) != 0) {
			close(fd);
			return 1;
		}
	}

	if (close(fd) != 0) {
		perror("test_hw_uart: close");
		return 1;
	}

	return 0;
}
