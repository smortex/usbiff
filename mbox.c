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

#include <err.h>
#include <fcntl.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>

#include "mbox.h"
#include "usbnotifier.h"

extern int verbose;

struct mbox {
    int fd;
    char *filename;
    uint8_t color;
    int has_new_mail;
};

struct mbox *
mbox_new (const char *filename)
{
    struct mbox *res;

    if ((res = malloc (sizeof (*res)))) {
	res->fd = 0;
	res->filename = strdup (filename);
	res->color    = COLOR_BLUE;
	mbox_check (res);
    }

    return res;
}

int
mbox_register (struct mbox *mbox, int kq)
{
    struct kevent ke;

    if (mbox->fd) {
	close (mbox->fd);
	ke.flags = EV_DELETE;
	kevent (kq, &ke, 1, NULL, 0, NULL);
    }

    mbox->fd = open (mbox->filename, O_RDONLY);

    EV_SET (&ke, mbox->fd, EVFILT_VNODE, EV_ADD | EV_ONESHOT, NOTE_WRITE, 0, mbox);

    return kevent (kq, &ke, 1, NULL, 0, NULL);
}

int
mbox_get_color (struct mbox *mbox)
{
    return mbox->color;
}

void
mbox_set_color (struct mbox *mbox, int color)
{
    mbox->color = color;
}

int
mbox_check (struct mbox *mbox)
{
    struct stat sb;
    if (stat (mbox->filename, &sb) < 0)
	err (EXIT_FAILURE, "Can't stat mbox \"%s\"", mbox->filename);


    if (sb.st_atime < sb.st_mtime) {
	if (verbose)
	    printf ("[%s] New mail arrived.\n", mbox->filename);
	return mbox->has_new_mail = 1;
    } else {
	if (verbose)
	    printf ("[%s] Mail has been read.\n", mbox->filename);
	return mbox->has_new_mail = 0;
    }
}

int
mbox_has_new_mail (struct mbox *mbox)
{
    return mbox->has_new_mail;
}

void
mbox_free (struct mbox *mbox)
{
    free (mbox->filename);
    free (mbox);
}
