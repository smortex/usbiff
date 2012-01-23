#ifndef _SIGNAL_H
#define _SIGNAL_H

#include "config.h"

struct signal {
    int signal;
    int color;
    int ignore;
    struct signal *next;
};

struct signal	*signal_new (const struct config *config, int signal);
int		 signal_register (struct signal *signal, int kq);
void		 signal_free (struct signal *);

#endif /* !_SIGNAL_H */
