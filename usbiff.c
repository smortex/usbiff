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
#include <syslog.h>
#include <unistd.h>

#include "usbiff_config.h"
#include "usbiff_mbox.h"
#include "usbiff_signal.h"
#include "usbnotifier.h"

#include "usbiff_common.h"

int verbose = 0;

static void
update_status (struct usbnotifier *notifier, struct config *config)
{
    int priority = PRIORITY_UNDEFINED;
    int color = COLOR_NONE;
    int flash = 0;

    struct mbox *mbox = config->mailboxes;

    while (mbox) {
	if (!mbox->ignore && mbox_has_new_mail (mbox) && mbox->priority < priority) {
	    priority = mbox->priority;
	    color = mbox->color;
	    flash = mbox->flash;
	}
	mbox = mbox->next;
    }

    if (flash)
	usbnotifier_flash_to (notifier, color, config);
    else
	usbnotifier_set_color (notifier, color);
}

static void
usage (void)
{
    fprintf (stderr, "usage: usbiff [options]+ [filename]+\n");
    fprintf (stderr, "\n");
    fprintf (stderr, "Options:\n");
    fprintf (stderr, "  -c <file>   Specify a configuration file\n");
    fprintf (stderr, "  -f          Do not fork and detach from the shell\n");
    fprintf (stderr, "  -n          Do not read the configuration file\n");
    fprintf (stderr, "  -v          Increase verbosity (implies -f)\n");
}

int
main (int argc, char *argv[])
{
    struct usbnotifier *notifier;
    int daemonize  = 1;
    int quit = 0;
    int ignore_config = 0;
    struct config *config;

    int ch;
    while ((ch = getopt (argc, argv, "c:fnv")) != -1) {
	switch (ch) {
	case 'c':
	    config_set_filename (optarg);
	    break;
	case 'v':
	    ++verbose;
	    /* FALLTHROUGH */
	case 'f':
	    daemonize = 0;
	    break;
	case 'n':
	    ignore_config = 1;
	    break;
	case '?':
	    usage ();
	    exit (EXIT_FAILURE);
	    break;
	}
    }

    argv += optind;
    argc -= optind;

    int log_options = LOG_PID;
    if (!daemonize)
	log_options |= LOG_PERROR;

    openlog ("usbiff", log_options, LOG_USER);

    if (ignore_config)
	config = config_new ();
    else
	config = config_load ();

    if (daemonize)
	if (daemon (0, 0) < 0)
	    err (EXIT_FAILURE, "daemon");

    notifier = usbnotifier_new ();
    if (!notifier) {
	syslog (LOG_ERR, "Cannot connect to notification device");
	exit (EXIT_FAILURE);
    }

    usbnotifier_set_color (notifier, COLOR_NONE);

    if (!config->mailboxes) {
	char *mbox = getenv ("MAIL");
	if (!mbox || (0 == strlen (mbox))) {
	    char *user = getenv ("USER");
	    asprintf (&mbox, "/var/mail/%s", user);
	}

	if (!mbox) {
	    syslog (LOG_ERR, "No mbox provided");
	    exit (EXIT_FAILURE);
	}

	config_add_mbox (config, mbox);
    } else {
	for (int i = 0; i < argc; i++) {
	    config_add_mbox (config, argv[i]);
	}
    }

    int kq = kqueue ();
    if (kq < 0) {
	syslog (LOG_ERR, "Error allocating kernel queue: %m");
	exit (EXIT_FAILURE);
    }

    config_register (config, kq);

    update_status (notifier, config);

    while (!quit) {
	struct kevent ke;
	int i = kevent (kq, NULL, 0, &ke, 1, NULL);
	if (i < 0) {
	    syslog (LOG_ERR, "Error adding kevent: %m");
	    exit (EXIT_FAILURE);
	}

	switch (ke.filter) {
	case EVFILT_SIGNAL:
	    {
		struct signal *signal = (struct signal *) ke.udata;
		switch (ke.ident) {
		case SIGHUP:
		    {
			if (ignore_config)
			    break;

			struct config *new_config = config_load ();
			if (new_config) {
			    config_unregister (config, kq);
			    config_free (config);
			    config = new_config;
			    config_register (config, kq);
			}
		    }
		    break;
		case SIGINFO:
		    {
			struct mbox *mbox = config->mailboxes;
			while (mbox) {
			    if (mbox->has_new_mail) {
				syslog (LOG_INFO, "[%s] has new mail.", mbox->filename);
			    }
			    mbox = mbox->next;
			}
		    }
		    break;
		case SIGINT:
		case SIGTERM:
		    quit = 1;
		    break;
		default:
		    if (!signal->ignore)
			usbnotifier_flash (notifier, signal->color, config);
		}
	    }
	    break;
	case EVFILT_VNODE:
	    {
		struct mbox *mbox = (struct mbox *) ke.udata;

		mbox_register (mbox, kq);
		mbox_check (mbox);

		update_status (notifier, config);
	    }
	    break;
	}
    }

    usbnotifier_set_color (notifier, COLOR_NONE);
    usbnotifier_free (notifier);
    config_unregister (config, kq);
    config_free (config);

    closelog ();

    exit(EXIT_SUCCESS);
}
