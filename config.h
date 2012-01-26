#ifndef _CONFIG_H
#define _CONFIG_H

struct config {
    struct mbox *mailboxes;
    struct signal *signals;
    struct {
	int color;
	int flash;
	int ignore;
	int priority;
    } default_settings;
    struct {
	int short_delay;
	int long_delay;
    } flash_delay;
};

void		 config_set_filename (char *);
const char	*config_get_filename (void);
struct config   *config_new (void);
struct config	*config_load (void);
void		 config_update_flash_delay (struct config *, int, int);
void		 config_update_default_settings (struct config *, int, int, int, int);
void		 config_add_mbox (struct config *, char *);
int		 config_update_mbox (struct config *, char *, int, int, int, int);
void		 config_add_signal (struct config *, int);
int		 config_update_signal (struct config *, int, int, int);
void		 config_register (struct config *, int);
void		 config_unregister (struct config *, int);
void		 config_free (struct config *);

#endif /* !_CONFIG_H */
