%{
#include <err.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <signal.h>

#include "common.h"

#include "config.h"
#include "usbnotifier.h"

extern int yylex (void);
extern int yyerror (char *, ...);

struct parsed_conf *prg;

enum parsed_statement_type {
    PST_MAILBOXES,
    PST_FLASH_OPTIONS,
    PST_GLOBAL,
    PST_MAILBOX_HOOK,
    PST_SIGNAL_HOOK
};

struct parsed_conf {
    struct parsed_statement *statements;
};

struct parsed_statement {
    enum parsed_statement_type type;
    void *data;
    struct parsed_statement *next;
};

struct parsed_flash_delay {
    int short_delay;
    int long_delay;
};

struct parsed_mbox {
    char *filename;
    struct parsed_mbox *next;
};

struct parsed_set_statement {
    int color;
    int flash;
    int ignore;
    int priority;
};

struct parsed_mailbox_hook {
    char *filename;
    struct parsed_set_statement *set_statements;
};

struct parsed_signal_hook {
    int signal;
    struct parsed_set_statement *set_statements;
};

void		 parsed_flash_delay_free (struct parsed_flash_delay *flash_delay);
void		 parsed_mbox_free (struct parsed_mbox *mbox);
void		 parsed_set_statement_free (struct parsed_set_statement *statement);
void		 parsed_mailbox_hook_free (struct parsed_mailbox_hook *mbox);
void		 parsed_signal_hook_free (struct parsed_signal_hook *signal);

struct parsed_conf *
parsed_conf_new (void)
{
    struct parsed_conf *res;

    if ((res = malloc (sizeof (*res)))) {
	res->statements = NULL;
    }

    prg = res;
    return res;
}

struct parsed_conf *
parsed_conf_add_statement (struct parsed_conf *conf, struct parsed_statement *statement)
{
    struct parsed_statement **pstatement = &conf->statements;

    while (*pstatement)
	pstatement = &(*pstatement)->next;

    *pstatement = statement;

    return conf;
}

void
parsed_conf_free (struct parsed_conf *conf)
{
    if (!conf)
        return;

    struct parsed_statement *statement = conf->statements;
    while (statement) {
	struct parsed_statement *curr = statement;
	statement = statement->next;
	switch (curr->type) {
	case PST_FLASH_OPTIONS:
	    parsed_flash_delay_free (curr->data);
	    break;
	case PST_GLOBAL:
	    parsed_set_statement_free (curr->data);
	    break;
	case PST_MAILBOXES:
	    parsed_mbox_free (curr->data);
	    break;
	case PST_MAILBOX_HOOK:
	    parsed_mailbox_hook_free (curr->data);
	    break;
	case PST_SIGNAL_HOOK:
	    parsed_signal_hook_free (curr->data);
	    break;
	}
	free (curr);
    }
    free (conf);
}

struct parsed_statement *
parsed_statement_new (enum parsed_statement_type type, void *data)
{
    struct parsed_statement *res;

    if ((res = malloc (sizeof (*res)))) {
	res->type = type;
	res->data = data;
	res->next = NULL;
    }

    return res;
}

struct parsed_flash_delay *
parsed_flash_delay_new (int short_delay, int long_delay)
{
    struct parsed_flash_delay *res;

    if (short_delay < 0)
	yyerror ("\"%d\" is not a valid short delay (must be >= 0)", short_delay);

    if (long_delay < 0)
	yyerror ("\"%d\" is not a valid long delay (must be >= 0)", long_delay);

    if ((res = malloc (sizeof (*res)))) {
	res->short_delay = short_delay;
	res->long_delay  = long_delay;
    }

    return res;
}


void
parsed_flash_delay_free (struct parsed_flash_delay *flash_delay)
{
    free (flash_delay);
}

struct parsed_mbox *
parsed_mbox_new (char *filename)
{
    struct parsed_mbox *res;

    if ((res = malloc (sizeof (*res)))) {
	res->filename = filename;
	res->next = NULL;
    }

    return res;
}

struct parsed_mbox *
parsed_mbox_new_with_command (char *cmd)
{
    FILE *f;
    struct parsed_mbox *res = NULL, *p = NULL, *new;

    if ((f = popen (cmd, "r"))) {
	char buffer[BUFSIZ];
	while (fgets (buffer, sizeof (buffer), f)) {
	    int n = strlen (buffer) - 1;
	    if (buffer[n] == '\n')
		buffer[n] = '\0';
	    new = parsed_mbox_new (strdup (buffer));
	    if (!res) {
		res = p = new;
	    } else {
		p->next = new;
		p = new;
	    }
	}
	pclose (f);
    } else {
	errx (EXIT_FAILURE, "Error running \"%s\".", cmd);
    }

    free (cmd);
    return res;
}

struct parsed_mbox *
parsed_mbox_merge (struct parsed_mbox *left, struct parsed_mbox *right)
{
    struct parsed_mbox *p = left;
    while (p->next)
	p = p->next;
    p->next = right;

    return left;
}

void
parsed_mbox_free (struct parsed_mbox *mbox)
{
    free (mbox->filename);
    if (mbox->next)
	parsed_mbox_free (mbox->next);
    free (mbox);
}

struct parsed_set_statement *
parsed_set_statement_new (void)
{
    struct parsed_set_statement *res;

    if ((res = malloc (sizeof (*res)))) {
	res->color = -1;
	res->flash = -1;
	res->ignore = -1;
	res->priority = PRIORITY_UNDEFINED;
    }

    return res;
}

struct parsed_set_statement *
parsed_set_statement_new_with_color (int color)
{
    struct parsed_set_statement *res;

    if ((res = parsed_set_statement_new ())) {
	res->color = color;
    }

    return res;
}

struct parsed_set_statement *
parsed_set_statement_new_with_flash (int flash)
{
    struct parsed_set_statement *res;

    if ((res = parsed_set_statement_new ())) {
	res->flash = flash;
    }

    return res;
}

struct parsed_set_statement *
parsed_set_statement_new_with_ignore (int ignore)
{
    struct parsed_set_statement *res;

    if ((res = parsed_set_statement_new ())) {
	res->ignore = ignore;
    }

    return res;
}

struct parsed_set_statement *
parsed_set_statement_new_with_priority (int priority)
{
    struct parsed_set_statement *res;

    if ((priority < -20) || (priority > 20))
	yyerror ("\"%d\" is not a valid priority in range [-20..+20]", priority);

    if ((res = parsed_set_statement_new ())) {
	res->priority = priority;
    }

    return res;
}

struct parsed_set_statement *
parsed_set_statement_merge (struct parsed_set_statement *left, struct parsed_set_statement *right)
{
    if (right->color >= 0)
	left->color = right->color;
    if (right->flash >= 0)
	left->flash = right->flash;
    if (right->ignore >= 0)
	left->ignore = right->ignore;
    if (right->priority != PRIORITY_UNDEFINED)
	left->priority = right->priority;

    free (right);
    return left;
}

void
parsed_set_statement_free (struct parsed_set_statement *statement)
{
    free (statement);
}

struct parsed_mailbox_hook *
parsed_mailbox_hook_new (char *filename, struct parsed_set_statement *set_statements)
{
    struct parsed_mailbox_hook *res;

    if ((res = malloc (sizeof (*res)))) {
	res->filename = filename;
	res->set_statements = set_statements;
    }

    return res;
}

void
parsed_mailbox_hook_free (struct parsed_mailbox_hook *mbox)
{
    free (mbox->filename);
    parsed_set_statement_free (mbox->set_statements);
    free (mbox);
}

struct parsed_signal_hook *
parsed_signal_hook_new (int signal, struct parsed_set_statement *set_statements)
{
    struct parsed_signal_hook *res;

    if ((res = malloc (sizeof (*res)))) {
	res->signal = signal;
	res->set_statements = set_statements;
    }

    return res;
}

void
parsed_signal_hook_free (struct parsed_signal_hook *signal)
{
    parsed_set_statement_free (signal->set_statements);
    free (signal);
}

void
yyconfigure (struct config *config)
{
    struct parsed_statement *s = NULL, *p = NULL;
    if (prg) {
	s = p = prg->statements;
    }

    while (p) {
	switch (p->type) {
	case PST_GLOBAL:
	    {
		struct parsed_set_statement *set_statements = (struct parsed_set_statement *)p->data;
		config_update_default_settings (config, set_statements->color, set_statements->flash, set_statements->ignore, set_statements->priority);
	    }
	    break;
	case PST_FLASH_OPTIONS:
	    {
		struct parsed_flash_delay *flash_delay = (struct parsed_flash_delay *)p->data;
		config_update_flash_delay (config, flash_delay->short_delay, flash_delay->long_delay);
	    }
	    break;
	default:
	    break;
	}
	p = p->next;
    }

    p = s;
    while (p) {
	switch (p->type) {
	case PST_MAILBOXES:
	    {
		struct parsed_mbox *mbox = (struct parsed_mbox *)p->data;
		while (mbox) {
		    config_add_mbox (config, mbox->filename);
		    mbox = mbox->next;
		}
	    }
	    break;
	default:
	    break;
	}
	p = p->next;
    }

    config_add_signal (config, SIGINT);
    config_add_signal (config, SIGTERM);
    config_add_signal (config, SIGUSR1);
    config_add_signal (config, SIGUSR2);

    p = s;
    while (p) {
	switch (p->type) {
	case PST_MAILBOX_HOOK:
	    {
		struct parsed_mailbox_hook *hook = (struct parsed_mailbox_hook *)p->data;
		config_update_mbox (config, hook->filename, hook->set_statements->color, hook->set_statements->flash, hook->set_statements->ignore, hook->set_statements->priority);
	    }
	    break;
	case PST_SIGNAL_HOOK:
	    {
		struct parsed_signal_hook *hook = (struct parsed_signal_hook *)p->data;
		config_update_signal (config, hook->signal, hook->set_statements->color, hook->set_statements->ignore);
	    }
	default:
	    break;
	}
	p = p->next;
    }
}

%}

%union {
    int number;
    char *string;
    struct parsed_statement *statement;
    struct parsed_conf *conf;
    struct parsed_mbox *mailboxes;
    struct parsed_set_statement *set_statement;
}

%token COMMAND FILENAME NUMBER
%token FLASH_DELAY MAILBOXES MAILBOX_HOOK SET SIGNAL_HOOK UNSET
%token COLOR FLASH NOFLASH PRIORITY IGNORE NOIGNORE
%token SIGNAL_USR1 SIGNAL_USR2
%token YES NO
%token RED GREEN BLUE CYAN MAGENTA YELLOW WHITE NONE

%type <conf> conf
%type <number> bool color signal NUMBER
%type <string> FILENAME COMMAND
%type <statement> statement
%type <mailboxes> filename mailbox_list
%type <set_statement> set_statements set_statement unset_statements unset_statement

%%

conf: conf statement { $$ = parsed_conf_add_statement ($1, $2); }
    |		     { $$ = parsed_conf_new (); }
    ;

statement: MAILBOXES mailbox_list                   { $$ = parsed_statement_new (PST_MAILBOXES, $2); }
	 | FLASH_DELAY NUMBER NUMBER                { $$ = parsed_statement_new (PST_FLASH_OPTIONS, parsed_flash_delay_new ($2, $3)); }
	 | SET set_statements                       { $$ = parsed_statement_new (PST_GLOBAL, $2); }
	 | UNSET unset_statements                   { $$ = parsed_statement_new (PST_GLOBAL, $2); }
	 | MAILBOX_HOOK FILENAME SET set_statements { $$ = parsed_statement_new (PST_MAILBOX_HOOK, parsed_mailbox_hook_new ($2, $4)); }
	 | SIGNAL_HOOK signal SET set_statements    { $$ = parsed_statement_new (PST_SIGNAL_HOOK, parsed_signal_hook_new ($2, $4)); }
	 ;

mailbox_list: mailbox_list filename { $$ = parsed_mbox_merge ($1, $2); }
	    | filename              { $$ = $1; }
	    ;

filename: FILENAME { $$ = parsed_mbox_new ($1); }
	| COMMAND  { $$ = parsed_mbox_new_with_command ($1); }
	;

set_statements: set_statements set_statement { $$ = parsed_set_statement_merge ($1, $2); }
	      | set_statement                { $$ = $1; }
	      ;

unset_statements: unset_statements unset_statement { $$ = parsed_set_statement_merge ($1, $2); }
		| unset_statement                  { $$ = $1; }
		;

set_statement: COLOR '=' color     { $$ = parsed_set_statement_new_with_color ($3); }
	     | PRIORITY '=' NUMBER { $$ = parsed_set_statement_new_with_priority ($3); }
	     | FLASH		   { $$ = parsed_set_statement_new_with_flash (1); }
	     | NOFLASH             { $$ = parsed_set_statement_new_with_flash (0); }
	     | FLASH '=' bool      { $$ = parsed_set_statement_new_with_flash ($3); }
	     | IGNORE              { $$ = parsed_set_statement_new_with_ignore (1); }
	     | NOIGNORE            { $$ = parsed_set_statement_new_with_ignore (0); }
	     | IGNORE '=' bool     { $$ = parsed_set_statement_new_with_ignore ($3); }
	     ;

unset_statement: FLASH  { $$ = parsed_set_statement_new_with_flash (0); }
	       | IGNORE { $$ = parsed_set_statement_new_with_ignore (0); }
	       ;

color: NONE    { $$ = COLOR_NONE; }
     | BLUE    { $$ = COLOR_BLUE; }
     | CYAN    { $$ = COLOR_CYAN; }
     | GREEN   { $$ = COLOR_GREEN; }
     | MAGENTA { $$ = COLOR_MAGENTA; }
     | RED     { $$ = COLOR_RED; }
     | WHITE   { $$ = COLOR_WHITE; }
     | YELLOW  { $$ = COLOR_YELLOW; }
     ;

bool: YES { $$ = 1; }
    | NO  { $$ = 0; }
    ;

signal: SIGNAL_USR1 { $$ = SIGUSR1; }
      | SIGNAL_USR2 { $$ = SIGUSR2; }
      ;

%%
