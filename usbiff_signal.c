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
	res->toggle = config->default_settings.toggle;
	res->toggle_state = 0;
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
