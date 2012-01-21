#ifndef _MBOX_H
#define _MBOX_H

struct mbox	*mbox_new (const char *);
int		 mbox_register (struct mbox *, int);
int		 mbox_get_color (struct mbox *);
void		 mbox_set_color (struct mbox *, int);
int		 mbox_check (struct mbox *);
int		 mbox_has_new_mail (struct mbox *);
void		 mbox_free (struct mbox *);

#endif
