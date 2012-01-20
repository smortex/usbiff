/*-
 * Copyright (c) 2012 Romain Tartière <romain@blogreen.org>.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#include <err.h>
#include <libusb.h>
#include <stdlib.h>

#include "usbnotifier.h"

struct usbnotifier {
    libusb_device_handle *handle;
};

struct usbnotifier *
usbnotifier_new (void)
{
    struct usbnotifier *res = NULL;

    if ((res = malloc (sizeof (*res)))) {
	libusb_context *ctx;
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
    uint8_t data[] = { color, 0, 0, 0, 0 };
    int n;
    if (0 != libusb_interrupt_transfer (notifier->handle, 2, data, sizeof (data), &n, 1000))
	err (EXIT_FAILURE, "libusb_interrupt_transfer");

    return (n == sizeof (data)) ? 0 : -1;
}
