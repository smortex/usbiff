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

#include <err.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "mbox.h"
#include "usbnotifier.h"

int verbose = 0;

static void
update_status (struct usbnotifier *notifier, struct mbox **mboxes, int mbox_count)
{
    int color = 0;
    for (int i = mbox_count -1; i >= 0; i--) {
	if (mbox_has_new_mail (mboxes[i]))
	    color = mbox_get_color (mboxes[i]);
    }

    usbnotifier_flash_to (notifier, color);
}

static void
usage (void)
{
    fprintf (stderr, "usage: usbiff [options]+ [filename]+\n");
    fprintf (stderr, "\n");
    fprintf (stderr, "Options:\n");
    fprintf (stderr, "  -f    Do not fork and detach from the shell\n");
    fprintf (stderr, "  -v    Increase verbosity (implies -f)\n");
}

int
main (int argc, char *argv[])
{
    struct mbox **mboxes;
    struct usbnotifier *notifier;
    int mbox_count = 0;
    int daemonize  = 1;
    int quit = 0;

    int ch;
    while ((ch = getopt (argc, argv, "fv")) != -1) {
	switch (ch) {
	case 'v':
	    ++verbose;
	case 'f':
	    daemonize = 0;
	    break;
	case '?':
	    usage ();
	    exit (EXIT_FAILURE);
	    break;
	}
    }

    argv += optind;
    argc -= optind;

    if (daemonize)
	if (daemon (0, 0) < 0)
	    err (EXIT_FAILURE, "daemon");

    notifier = usbnotifier_new ();
    usbnotifier_set_color (notifier, COLOR_NONE);

    if (argc == 0) {
	mbox_count = 1;
	mboxes = malloc (sizeof (*mboxes));

	char *mbox = getenv ("MAIL");
	if (!mbox || (0 == strlen (mbox))) {
	    char *user = getenv ("USER");
	    asprintf (&mbox, "/var/mail/%s", user);
	}

	if (!mbox)
	    err (EXIT_FAILURE, "No mbox provided");

	mboxes[0] = mbox_new (mbox);

    } else {
	mbox_count = argc;
	mboxes = malloc (mbox_count * sizeof (*mboxes));

	for (int i = 0; i < mbox_count; i++) {
	    mboxes[i] = mbox_new (argv[i]);
	    mbox_set_color (mboxes[i], COLOR_BLUE);
	}
    }
    mbox_set_color (mboxes[0], COLOR_RED);

    int kq = kqueue ();
    if (kq < 0)
	err (EXIT_FAILURE, "kqueue");

    for (int i = 0; i < mbox_count; i++)
	mbox_register (mboxes[i], kq);

    update_status (notifier, mboxes, mbox_count);

    struct kevent ke;

    struct sigaction sa;
    memset (&sa, '\0', sizeof (sa));
    sa.sa_handler = SIG_IGN;
    if (sigaction (SIGINT, &sa, NULL) < 0)
	err (EXIT_FAILURE, "sigaction");

    EV_SET (&ke, SIGINT, EVFILT_SIGNAL, EV_ADD, 0, 0, NULL);
    if (kevent (kq, &ke, 1, NULL, 0, NULL) < 0)
	err (EXIT_FAILURE, "kevent");

    if (sigaction (SIGTERM, &sa, NULL) < 0)
	err (EXIT_FAILURE, "sigaction");

    EV_SET (&ke, SIGTERM, EVFILT_SIGNAL, EV_ADD, 0, 0, NULL);
    if (kevent (kq, &ke, 1, NULL, 0, NULL) < 0)
	err (EXIT_FAILURE, "kevent");

    while (!quit) {
	int i = kevent (kq, NULL, 0, &ke, 1, NULL);
	if (i < 0)
	    err (EXIT_FAILURE, "kevent");

	switch (ke.filter) {
	case EVFILT_SIGNAL:
	    switch (ke.ident) {
	    case SIGINT:
	    case SIGTERM:
		quit = 1;
		break;
	    }
	    break;
	case EVFILT_VNODE:
	    {
		struct mbox *mbox = (struct mbox *) ke.udata;

		mbox_register (mbox, kq);
		mbox_check (mbox);

		update_status (notifier, mboxes, mbox_count);
	    }
	    break;
	}
    }

    usbnotifier_set_color (notifier, COLOR_NONE);
    usbnotifier_free (notifier);

    for (int i = 0; i < mbox_count; i++) {
	mbox_free (mboxes[i]);
    }
    free (mboxes);

    exit(EXIT_SUCCESS);
}
