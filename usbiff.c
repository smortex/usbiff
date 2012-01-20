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
#include <sys/stat.h>
#include <sys/time.h>

#include <err.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "usbnotifier.h"

struct mbox {
    int fd;
    char *filename;
    uint8_t color;
};

struct mbox *mboxes;
struct usbnotifier *notifier;

void
register_mbox (int kq, struct mbox *mbox)
{
    struct kevent ke;

    mbox->fd = open (mbox->filename, O_RDONLY);

    EV_SET (&ke, mbox->fd, EVFILT_VNODE, EV_ADD | EV_ONESHOT, NOTE_WRITE, 0, mbox);

    if (kevent (kq, &ke, 1, NULL, 0, NULL) < 0)
	err (EXIT_FAILURE, "kevent %s", mbox->filename);
}

void
mail_status (struct mbox *mbox)
{
    struct stat sb;
    if (stat (mbox->filename, &sb) < 0)
	err (EXIT_FAILURE, "Can't stat mbox \"%s\"", mbox->filename);


    if (sb.st_atime < sb.st_mtime) {
	usbnotifier_set_color (notifier, mbox->color);
	printf ("Lain has new mail in %s\n", mbox->filename);
    } else {
	usbnotifier_set_color (notifier, COLOR_NONE);
	printf ("Lain has no mail in %s\n", mbox->filename);
    }

}

int
main (int argc, char *argv[])
{
    int mbox_count = 0;

    notifier = usbnotifier_new ();
    usbnotifier_set_color (notifier, COLOR_NONE);

    if (argc == 1) {
	mbox_count = 1;
	mboxes = malloc (sizeof (*mboxes));

	const char *s = getenv ("MAIL");
	if (s)
	    mboxes[0].filename = strdup (s);
	else if ((s = getenv ("USER")))
	    asprintf (&mboxes[0].filename, "/var/mail/%s", s);

	if (!mboxes[0].filename)
	    err (EXIT_FAILURE, "No mbox provided");
    } else {
	mbox_count = argc - 1;
	mboxes = malloc (mbox_count * sizeof (*mboxes));

	for (int i = 0; i < mbox_count; i++) {
	    mboxes[i].filename = argv[i+1];
	    mboxes[i].color = COLOR_BLUE;
	}
    }
    mboxes[0].color = COLOR_RED;

    int kq = kqueue ();
    if (kq < 0)
	err (EXIT_FAILURE, "kqueue");

    for (int i = 0; i < mbox_count; i++)
	register_mbox (kq, &mboxes[i]);

    struct kevent ke;

    for (;;) {
	int i = kevent (kq, NULL, 0, &ke, 1, NULL);
	if (i < 0)
	    err (EXIT_FAILURE, "kevent");

	struct mbox *mbox = (struct mbox *) ke.udata;

	close (mbox->fd);
	ke.flags = EV_DELETE;
	kevent (kq, &ke, 1, NULL, 0, NULL);

	register_mbox (kq, mbox);
	mail_status (mbox);
    }

    exit(EXIT_SUCCESS);
}
