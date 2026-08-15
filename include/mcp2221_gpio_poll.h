/**
 * @file mcp2221_gpio_poll.h
 * @brief Stateful GPIO change and edge-event polling helpers.
 */

#ifndef MCP2221_GPIO_POLL_H
#define MCP2221_GPIO_POLL_H

#include <stdint.h>
#include <stddef.h>

#include "mcp2221.h"

MCP2221_BEGIN_DECLS

/**
 * @brief Change information for one GP pin.
 */
typedef struct {
	/**
	 * @brief Previous sampled state.
	 *
	 * A value of -1 means the pin was not configured as GPIO; otherwise the
	 * value is 0 or 1.
	 */
	int old_value;

	/**
	 * @brief Current sampled state.
	 *
	 * A value of -1 means the pin is not configured as GPIO; otherwise the
	 * value is 0 or 1.
	 */
	int new_value;

	/** @brief Nonzero when @ref old_value and @ref new_value differ. */
	int changed;
} mcp2221_gpio_change_t;

/**
 * @brief Persistent state used by the GPIO polling helpers.
 *
 * Initialize this structure with mcp2221_gpio_poll_init() before its first
 * use. Applications should treat the members as polling state and avoid
 * modifying them directly except through the public helper functions.
 */
typedef struct {
	/**
	 * @brief Previous GP0 through GP3 samples.
	 *
	 * Entries are maintained by the polling functions. A value of -1 represents
	 * a pin that was not configured as GPIO.
	 */
	int prev[4];

	/** @brief Nonzero after the first successful poll has initialized @ref prev. */
	int initialized;

	/**
	 * @brief Wall-clock timestamp of the previous event poll, in seconds.
	 *
	 * This field is used by mcp2221_gpio_poll_events(). It is initialized on
	 * the first successful event poll and updated after subsequent polls.
	 */
	double last_time;

	/**
	 * @brief Persistent edge-event filter mask.
	 *
	 * A value of 0 accepts all events. Otherwise the mask uses alternating
	 * rise/fall bits for GP0 through GP3 as produced by
	 * MCP2221_GPIO_POLL_MASK_RISE() and MCP2221_GPIO_POLL_MASK_FALL().
	 */
	uint16_t filter_mask;
} mcp2221_gpio_poll_state_t;

/**
 * @brief GPIO edge-event type.
 */
typedef enum {
	MCP2221_GPIO_EVENT_RISE = 0, /**< Low-to-high transition. */
	MCP2221_GPIO_EVENT_FALL = 1, /**< High-to-low transition. */
} mcp2221_gpio_event_type_t;

/**
 * @brief One GPIO edge event produced by mcp2221_gpio_poll_events().
 */
typedef struct {
	/**
	 * @brief Null-terminated event identifier.
	 *
	 * The current implementation formats identifiers as `"GPIOx_RISE"` or
	 * `"GPIOx_FALL"`.
	 */
	char id[12];

	/** @brief GP pin number from 0 through 3. */
	uint8_t gpio;

	/** @brief Detected edge type. */
	mcp2221_gpio_event_type_t type;

	/** @brief Wall-clock time of the current poll, in seconds. */
	double time;

	/** @brief Wall-clock time of the previous event poll, in seconds. */
	double last_time;
} mcp2221_gpio_event_t;

/**
 * @brief Build the filter-mask bit for a rising edge on a GP pin.
 * @param pin GP pin number from 0 through 3.
 */
#define MCP2221_GPIO_POLL_MASK_RISE(pin) (1u << ((pin) * 2))

/**
 * @brief Build the filter-mask bit for a falling edge on a GP pin.
 * @param pin GP pin number from 0 through 3.
 */
#define MCP2221_GPIO_POLL_MASK_FALL(pin) (1u << ((pin) * 2 + 1))

/**
 * @brief Initialize a GPIO polling state object.
 *
 * The filter is reset to 0, which accepts all edge events. Passing `NULL` is
 * a no-op.
 *
 * @param[out] st Polling state to initialize.
 */
MCP2221_API void mcp2221_gpio_poll_init(mcp2221_gpio_poll_state_t *st);

/**
 * @brief Set the persistent edge-event filter mask.
 *
 * A mask of 0 accepts all events. Passing `NULL` is a no-op.
 *
 * @param[in,out] st Polling state whose filter is updated.
 * @param[in] mask New event filter mask.
 */
MCP2221_API void mcp2221_gpio_poll_set_filter_mask(mcp2221_gpio_poll_state_t *st, uint16_t mask);

/**
 * @brief Poll GP0 through GP3 and report per-pin state changes.
 *
 * On the first successful call, the function initializes the previous-state
 * snapshot and reports `changed == 0` for every pin.
 *
 * A sampled value of -1 indicates that the corresponding pin is not currently
 * configured as GPIO.
 *
 * @param[in] dev Open MCP2221 device handle.
 * @param[in,out] st Initialized polling state.
 * @param[out] out Four-element array receiving GP0 through GP3 changes.
 *
 * @return MCP2221_ERR_OK on success, MCP2221_ERR_INVALID for invalid
 *         arguments, or another mcp2221_error_code_t value on failure.
 */
MCP2221_API mcp2221_error_code_t mcp2221_gpio_poll(
    mcp2221_t *dev,
    mcp2221_gpio_poll_state_t *st,
    mcp2221_gpio_change_t out[4]
);

/**
 * @brief Poll GPIO changes and emit filtered rise/fall events.
 *
 * The first successful call initializes the polling snapshot and returns zero
 * events. Transitions are emitted only when both the previous and current
 * samples are valid GPIO states.
 *
 * If @p filter_mask_opt is `NULL`, the filter already stored in @p st is
 * preserved. Otherwise `*filter_mask_opt` becomes the new persistent filter;
 * a value of 0 accepts all events.
 *
 * At most @p max_events events are written. The polling state advances after
 * every successful call even when more matching transitions occurred than fit
 * in @p out_events, so excess events are discarded rather than returned by a
 * later poll.
 *
 * @param[in] dev Open MCP2221 device handle.
 * @param[in,out] st Initialized polling state.
 * @param[in] filter_mask_opt Optional replacement filter mask, or `NULL` to
 *                            preserve the existing filter.
 * @param[out] out_events Event buffer. May be `NULL` only when
 *                        @p max_events is 0.
 * @param[in] max_events Maximum number of events that may be written.
 *
 * @return Number of events written, from 0 through @p max_events, on success;
 *         otherwise a negative mcp2221_error_code_t value.
 *
 * @note Event timestamps use wall-clock time rather than a monotonic elapsed
 *       time source.
 */
MCP2221_API int mcp2221_gpio_poll_events(mcp2221_t *dev, mcp2221_gpio_poll_state_t *st, const uint16_t *filter_mask_opt,
							mcp2221_gpio_event_t *out_events, size_t max_events);

MCP2221_END_DECLS
#endif	// MCP2221_GPIO_POLL_H
