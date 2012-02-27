%{
#include <stdarg.h>
#include <string.h>
#include <stdlib.h>

#include "conf_parser.h"

#include "usbiff_common.h"

int lineno = 1;
int yyerror (char *, ...);

static char *current_line;
static char *current_line_pos;

void
update_curent_line (void)
{
    free (current_line);
    current_line = strdup (yytext);
    current_line_pos = yytext;
}

void
yyfree (void)
{
    free (current_line);
    current_line = NULL;
}

%}
%option nounput
%option noinput

%x PARSE

%%

<INITIAL>.*		update_curent_line (); BEGIN(PARSE); yyless(0);

<PARSE>#.*$		;

<PARSE>flash-delay	return FLASH_DELAY;
<PARSE>mailboxes	return MAILBOXES;
<PARSE>set		return SET;
<PARSE>unset		return UNSET;
<PARSE>mbox-hook	return MAILBOX_HOOK;
<PARSE>signal-hook	return SIGNAL_HOOK;
<PARSE>=		return '=';

<PARSE>SIGUSR1		return SIGNAL_USR1;
<PARSE>SIGUSR2		return SIGNAL_USR2;

<PARSE>color		return COLOR;

<PARSE>priority		return PRIORITY;

<PARSE>flash		return FLASH;
<PARSE>noflash		return NOFLASH;

<PARSE>ignore		return IGNORE;
<PARSE>noignore		return NOIGNORE;

<PARSE>toggle		return TOGGLE;
<PARSE>notoggle		return NOTOGGLE;

<PARSE>yes		return YES;
<PARSE>no		return NO;

<PARSE>none		return NONE;
<PARSE>blue		return BLUE;
<PARSE>cyan		return CYAN;
<PARSE>green		return GREEN;
<PARSE>magenta		return MAGENTA;
<PARSE>red		return RED;
<PARSE>white		return WHITE;
<PARSE>yellow		return YELLOW;

<PARSE>[+-]?[0-9]+	yylval.number = atoi (yytext); return NUMBER;
<PARSE>`[^`]*`		{
    yylval.string = malloc (yyleng - 1);
    memcpy (yylval.string, yytext + 1, yyleng - 2);
    yylval.string[yyleng - 2] = '\0';
    return COMMAND;
}
<PARSE>[[:alnum:]/_\.-]+	yylval.string = strdup (yytext); return FILENAME;

<PARSE>[ \t]		;
<*>\n			lineno++; BEGIN(INITIAL);

<*>.			yyerror ("Unexpected character: '%c'", yytext[0]);

%%

int
yyerror (char *msg, ...)
{
    char *s;

    va_list ap;
    va_start (ap, msg);
    vasprintf (&s, msg, ap);
    va_end (ap);

    printf ("\033[1m%s:%d:%ld: \033[31;1merror:\033[0;1m %s\033[0m\n", config_get_filename (), lineno, yytext - current_line_pos + 1, s);
    free (s);
    printf ("%s\n", current_line);
    for (int i = 0; i < yytext - current_line_pos; i++)
	printf (" ");
    printf ("\033[32;1m^");
    for (int i = 1; i < yyleng; i++)
	printf ("~");
    printf ("\033[0m\n");
    return 0;
}

int
yywrap (void)
{
    return 1;
}
