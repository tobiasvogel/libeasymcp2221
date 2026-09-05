#define _POSIX_C_SOURCE 200809L

#include <errno.h>
#include <fcntl.h>
#include <poll.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <termios.h>
#include <time.h>
#include <unistd.h>

#define UART_TIMEOUT_MS 2000
#define UART_SETTLE_MS 100
#define UART_MAX_TEST_SIZE 512
#define CTEST_SKIP_RETURN_CODE 77

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
		perror("test_hw_uart: read");
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
	if (!path || !*path) {
		fprintf(stderr,
		        "test_hw_uart: LIBEASYMCP2221_HW_UART is not set; skipping\n");
		return CTEST_SKIP_RETURN_CODE;
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
