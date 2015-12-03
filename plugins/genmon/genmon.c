/* genmon_priv.c -- Generic monitor plugin for fbpanel
 *
 * Copyright (C) 2007 Davide Truffa <davide@catoblepa.org>
 * 
 * This plugin is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; version 2 dated June, 1991.
 * 
 * It is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 */

#include <sys/types.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "panel.h"
#include "misc.h"
#include "plugin.h"

//#define DEBUG
#include "dbg.h"

#define FMT "<span size='%s' foreground='%s'>%s</span>"

typedef struct {
    plugin_instance plugin;
    int time;
    int timer;
    int max_text_len;
    char *command;
    char *textsize;
    char *textcolor;
    GtkWidget *main;
} genmon_priv;

static int
text_update(genmon_priv *gm)
{
    FILE *fp;  
    char text[256];
    char *markup;
    int len;

    ENTER;
    fp = popen(gm->command, "r");
    fgets(text, sizeof(text), fp);
    pclose(fp);
    len = strlen(text) - 1;
    if (len >= 0) {
        if (text[len] == '\n')
            text[len] = 0;
        
        markup = g_markup_printf_escaped(FMT, gm->textsize, gm->textcolor,
            text);
        gtk_label_set_markup (GTK_LABEL(gm->main), markup);
        g_free(markup);
    }
    RET(TRUE);
}

static void
genmon_destructor(plugin_instance *p)
{
    genmon_priv *gm = (genmon_priv *) p;

    ENTER;
    if (gm->timer) {
        g_source_remove(gm->timer);
    }
    RET();
}

static int
genmon_constructor(plugin_instance *p)
{
    genmon_priv *gm;

    ENTER;
    gm = (genmon_priv *) p;
    gm->command = "date +%R";
    gm->time = 1;
    gm->textsize = "medium";
    gm->textcolor = "darkblue";
    gm->max_text_len = 30;
    
    XCG(p->xc, "Command", &gm->command, str);
    XCG(p->xc, "TextSize", &gm->textsize, str);
    XCG(p->xc, "TextColor", &gm->textcolor, str);
    XCG(p->xc, "PollingTime", &gm->time, int);
    XCG(p->xc, "MaxTextLength", &gm->max_text_len, int);
    
    gm->main = gtk_label_new(NULL);
    gtk_label_set_max_width_chars(GTK_LABEL(gm->main), gm->max_text_len);
    text_update(gm);
    gtk_container_set_border_width (GTK_CONTAINER (p->pwid), 1);
    gtk_container_add(GTK_CONTAINER(p->pwid), gm->main);
    gtk_widget_show_all(p->pwid);
    gm->timer = g_timeout_add((guint) gm->time * 1000,
        (GSourceFunc) text_update, (gpointer) gm);
    
    RET(1);
}


static plugin_class class = {
    .count       = 0,
    .type        = "genmon",
    .name        = "Generic Monitor",
    .version     = "0.3",
    .description = "Display the output of a program/script into the panel",
    .priv_size   = sizeof(genmon_priv),

    .constructor = genmon_constructor,
    .destructor  = genmon_destructor,
};
static plugin_class *class_ptr = (plugin_class *) &class;
