#ifndef _MBOX_H
#define _MBOX_H

#include <sys/types.h>

#include "config.h"

struct mbox {
    int fd;
    char *filename;
    uint8_t color;
    int flash;
    int ignore;
    int priority;
    int has_new_mail;
    struct mbox *next;
};

struct mbox	*mbox_new (const struct config *, const char *);
int		 mbox_register (struct mbox *, int);
int		 mbox_get_color (struct mbox *);
void		 mbox_set_color (struct mbox *, int);
int		 mbox_check (struct mbox *);
int		 mbox_has_new_mail (struct mbox *);
void		 mbox_free (struct mbox *);

#endif
