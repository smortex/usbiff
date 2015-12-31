#include <libusb.h>
#include <stdlib.h>
#include <time.h>
#include <syslog.h>

#include "usbnotifier.h"

#include "usbiff_common.h"

static int current_color = -1;
static libusb_context *ctx;

struct usbnotifier {
    libusb_device_handle *handle;
};

struct usbnotifier *
usbnotifier_new (void)
{
    struct usbnotifier *res = NULL;

    if ((res = malloc (sizeof (*res)))) {
	if (0 != libusb_init (&ctx))
	    goto error;

	res->handle = libusb_open_device_with_vid_pid (ctx, 0x1294, 0x1320);
	if (!res->handle)
	    goto error;
    }

    return res;

error:
    free (res);
    return NULL;
}

int
usbnotifier_set_color (struct usbnotifier *notifier, uint8_t color)
{
    if (current_color == color)
	return 0;

    uint8_t data[] = { color, 0, 0, 0, 0 };
    int n;
    if (0 != libusb_interrupt_transfer (notifier->handle, 2, data, sizeof (data), &n, 1000)) {
	syslog (LOG_ERR, "libusb_interrupt_transfer");
	exit (EXIT_FAILURE);
    }

    current_color = color;

    return (n == sizeof (data)) ? 0 : -1;
}

int
usbnotifier_flash (struct usbnotifier *notifier, uint8_t color, struct config *config)
{
    if (current_color == color)
	return 0;

    int original_color = current_color;

    struct timespec ts = {
	.tv_sec  = config->flash_delay.long_delay / 1000,
	.tv_nsec = (config->flash_delay.long_delay % 1000) * 1000000,
    };

    usbnotifier_flash_to (notifier, color, config);
    nanosleep (&ts, 0);
    usbnotifier_set_color (notifier, original_color);

    return 0;
}

int
usbnotifier_flash_to (struct usbnotifier *notifier, uint8_t color, struct config *config)
{
    if (current_color == color)
	return 0;

    int original_color = current_color;

    struct timespec ts = {
	.tv_sec  = config->flash_delay.short_delay / 1000,
	.tv_nsec = (config->flash_delay.short_delay % 1000) * 1000000,
    };

    usbnotifier_set_color (notifier, color);
    if (COLOR_NONE == color)
	return 0; /* Just switch the light off */

    nanosleep (&ts, 0);
    usbnotifier_set_color (notifier, original_color);
    nanosleep (&ts, 0);
    usbnotifier_set_color (notifier, color);
    nanosleep (&ts, 0);
    usbnotifier_set_color (notifier, original_color);
    nanosleep (&ts, 0);
    usbnotifier_set_color (notifier, color);

    return 0;
}

void
usbnotifier_free (struct usbnotifier *notifier)
{
    libusb_close (notifier->handle);
    libusb_exit (ctx);
    free (notifier);
}
