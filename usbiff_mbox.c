#include <sys/types.h>
#include <sys/event.h>
#include <sys/stat.h>

#include <fcntl.h>
#include <string.h>
#include <stdlib.h>
#include <syslog.h>
#include <unistd.h>

#include "usbiff_mbox.h"
#include "usbnotifier.h"

#include "usbiff_common.h"

struct mbox *
mbox_new (const struct config *config, const char *filename)
{
    struct mbox *res;

    if ((res = malloc (sizeof (*res)))) {
	res->fd = 0;
	res->filename = strdup (filename);
	res->color    = config->default_settings.color;
	res->flash    = config->default_settings.flash;
	res->ignore   = config->default_settings.ignore;
	res->priority = config->default_settings.priority;
	res->next     = NULL;
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
    }

    mbox->fd = open (mbox->filename, O_RDONLY);

    EV_SET (&ke, mbox->fd, EVFILT_VNODE, EV_ADD | EV_ONESHOT, NOTE_ATTRIB | NOTE_EXTEND | NOTE_WRITE, 0, mbox);

    return kevent (kq, &ke, 1, NULL, 0, NULL);
}

int
mbox_unregister (struct mbox *mbox, int kq)
{
    (void) kq;

    int res;
    if (0 == (res = close (mbox->fd))) {
	mbox->fd = 0;
    }
    return res;
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
    if (stat (mbox->filename, &sb) < 0) {
	syslog (LOG_ERR, "Can't stat mbox \"%s\"", mbox->filename);
	exit (EXIT_FAILURE);
    }


    if (sb.st_atime < sb.st_mtime) {
	syslog (LOG_DEBUG, "[%s] New mail arrived.\n", mbox->filename);
	return mbox->has_new_mail = 1;
    } else {
	syslog (LOG_DEBUG, "[%s] Mail has been read.\n", mbox->filename);
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
