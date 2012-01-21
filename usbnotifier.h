#ifndef _USBNOTIFIER_H
#define _USBNOTIFIER_H

#define COLOR_NONE	0
#define COLOR_BLUE	1
#define COLOR_RED	2
#define COLOR_GREEN	3
#define COLOR_CYAN	4
#define COLOR_MAGENTA	5
#define COLOR_YELLOW	6
#define COLOR_WHITE	7

struct usbnotifier *usbnotifier_new (void);
int		 usbnotifier_set_color (struct usbnotifier *, uint8_t);
int		 usbnotifier_flash (struct usbnotifier *, uint8_t);
int		 usbnotifier_flash_to (struct usbnotifier *, uint8_t);
void		 usbnotifier_free (struct usbnotifier *);

#endif /* !_USBNOTIFIER_H */
