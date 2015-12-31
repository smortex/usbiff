#ifndef _USBIFF_COMMON_H
#define _USBIFF_COMMON_H

#include <stdio.h>

#include "usbiff_config.h"

#define PRIORITY_UNDEFINED 42

extern int verbose;

extern FILE *yyin;
extern int yyparse (void);
extern struct parsed_conf *prg;

void		 yyconfigure (struct config *);
void		 yyclean (void);
void		 parsed_conf_free (struct parsed_conf *);

#endif /* !_USBIFF_COMMON_H */
