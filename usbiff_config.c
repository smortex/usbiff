/*-
 * Copyright (c) 2012 Romain Tartière <romain@blogreen.org>.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#include <string.h>
#include <stdlib.h>
#include <syslog.h>

#include "usbiff_config.h"
#include "usbiff_mbox.h"
#include "usbiff_signal.h"
#include "usbnotifier.h"

#include "usbiff_common.h"

static char *config_filename = NULL;
static int config_filename_provided = 0;

void
config_set_filename (char *filename)
{
    free (config_filename);
    config_filename = strdup (filename);
    config_filename_provided = 1;
}

const char *
config_get_filename (void)
{
    if (!config_filename)
	return "<stdin>";
    else
	return config_filename;
}

struct config *
config_new (void)
{
    struct config *res;
    if ((res = malloc (sizeof (*res)))) {
	res->mailboxes = NULL;
	res->signals   = NULL;

	res->default_settings.color    = COLOR_BLUE;
	res->default_settings.flash    = 1;
	res->default_settings.ignore   = 0;
	res->default_settings.priority = 10;
	res->default_settings.toggle   = 0;

	res->flash_delay.short_delay = 250;
	res->flash_delay.long_delay  = 1500;
    }

    return res;
}

struct config *
config_load (void)
{
    struct config *res = config_new ();

    if (!config_filename)
	asprintf (&config_filename, "%s/.usbiffrc", getenv ("HOME"));

    if (strcmp ("-", config_filename)) {
	yyin = fopen (config_filename, "r");
	if (!yyin) {
	    if (config_filename_provided) {
		syslog (LOG_ERR, "Cannot read configuration file \"%s\"", config_filename);
		exit (EXIT_FAILURE);
	    } else
		goto no_configuration_file;
	}
    }
    if (yyparse () != 0) {
	syslog (LOG_ERR, "Cannot parse configuration file");
	exit (EXIT_FAILURE);
    }

no_configuration_file:
    yyconfigure (res);
    parsed_conf_free (prg);
    yyfree ();

    if (yyin)
	fclose (yyin);

    return res;
}

void
config_update_flash_delay (struct config *config, int short_delay, int long_delay)
{
    config->flash_delay.short_delay = short_delay;
    config->flash_delay.long_delay  = long_delay;
}

void
config_update_default_settings (struct config *config, int color, int flash, int ignore, int priority, int toggle)
{
    if (color >= 0)
	config->default_settings.color = color;
    if (flash >= 0)
	config->default_settings.flash = flash;
    if (ignore >= 0)
	config->default_settings.ignore = ignore;
    if (priority != PRIORITY_UNDEFINED)
	config->default_settings.priority = priority;
    if (toggle >= 0)
	config->default_settings.toggle = toggle;
}

void
config_add_mbox (struct config *config, char *filename)
{
    struct mbox **p = &config->mailboxes;
    while (*p) {
	if (0 == strcmp ((*p)->filename, filename)) {
	    return;
	}
	p = &((*p)->next);
    }

    *p = mbox_new (config, filename);
}

int
config_update_mbox (struct config *config, char *filename, int color, int flash, int ignore, int priority)
{
    struct mbox **p = &config->mailboxes;
    while (*p) {
	if (0 == strcmp ((*p)->filename, filename)) {
	    if (color >= 0)
		(*p)->color = color;
	    if (flash >= 0)
		(*p)->flash = flash;
	    if (ignore >= 0)
		(*p)->ignore = ignore;
	    if (priority != PRIORITY_UNDEFINED)
		(*p)->priority = priority;
	    return 0;
	}
	p = &((*p)->next);
    }

    return -1;
}

void
config_add_signal (struct config *config, int signal)
{
    struct signal **p = &config->signals;
    while (*p) {
	if ((*p)->signal == signal) {
	    return;
	}
	p = &((*p)->next);
    }

    *p = signal_new (config, signal);
}

int
config_update_signal (struct config *config, int signal, int color, int ignore, int toggle)
{
    struct signal **p = &config->signals;
    while (*p) {
	if ((*p)->signal == signal) {
	    if (color >= 0)
		(*p)->color = color;
	    if (ignore >= 0)
		(*p)->ignore = ignore;
	    if (toggle >= 0)
		(*p)->toggle = toggle;
	    return 0;
	}
	p = &((*p)->next);
    }

    return -1;
}

void
config_register (struct config *config, int kq)
{
    struct mbox *mbox = config->mailboxes;

    while (mbox) {
	if (!mbox->ignore)
	    mbox_register (mbox, kq);
	mbox = mbox->next;
    }

    struct signal *signal = config->signals;

    while (signal) {
	signal_register (signal, kq);
	signal = signal->next;
    }
}

void
config_unregister (struct config *config, int kq)
{
    struct mbox *mbox = config->mailboxes;

    while (mbox) {
	if (mbox_unregister (mbox, kq) < 0)
	    syslog (LOG_WARNING, "Cannot unregister mailbox");
	mbox = mbox->next;
    }

    struct signal *signal = config->signals;

    while (signal) {
	signal_unregister (signal, kq);
	signal = signal->next;
    }
}

void
config_free (struct config *config)
{
    struct mbox *mbox = config->mailboxes, *mbox_o;
    while (mbox) {
	mbox_o = mbox;
	mbox = mbox->next;
	mbox_free (mbox_o);
    }
    struct signal *signal = config->signals, *signal_o;
    while (signal) {
	signal_o = signal;
	signal = signal->next;
	signal_free (signal_o);
    }
    free (config_filename);
    config_filename = NULL;
    free (config);
}
