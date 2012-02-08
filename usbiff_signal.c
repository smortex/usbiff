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

#include <sys/types.h>

#include <sys/event.h>

#include <signal.h>
#include <string.h>
#include <stdlib.h>
#include <syslog.h>

#include "usbiff_signal.h"

struct signal *
signal_new (const struct config *config, int signal)
{
    struct signal *res;

    if ((res = malloc (sizeof (*res)))) {
	res->signal = signal;
	res->color  = config->default_settings.color;
	res->ignore = config->default_settings.ignore;
	res->next   = NULL;
    }

    return res;
}

int
signal_register (struct signal *signal, int kq)
{
    struct kevent ke;

    struct sigaction sa;
    memset (&sa, '\0', sizeof (sa));
    sa.sa_handler = SIG_IGN;

    if (sigaction (signal->signal, &sa, NULL) < 0) {
	syslog (LOG_ERR, "sigaction");
	exit (EXIT_FAILURE);
    }

    if (signal->ignore)
	return 0;

    EV_SET (&ke, signal->signal, EVFILT_SIGNAL, EV_ADD, 0, 0, signal);
    return kevent (kq, &ke, 1, NULL, 0, NULL);
}

int
signal_unregister (struct signal *signal, int kq)
{
    struct kevent ke;

    struct sigaction sa;
    memset (&sa, '\0', sizeof (sa));
    sa.sa_handler = SIG_DFL;

    if (sigaction (signal->signal, &sa, NULL) < 0) {
	syslog (LOG_ERR, "sigaction");
	exit (EXIT_FAILURE);
    }

    EV_SET (&ke, signal->signal, EVFILT_SIGNAL, EV_DELETE, 0, 0, signal);
    return kevent (kq, &ke, 1, NULL, 0, NULL);
}

void
signal_free (struct signal *signal)
{
    free (signal);
}
