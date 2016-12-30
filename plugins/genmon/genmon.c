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
    unsigned int max_tooltip_line_len;
    unsigned int max_tooltip_lines_count;
    char *command;
    char *textsize;
    char *textcolor;
    char* tooltip;
    GtkWidget *main;
} genmon_priv;

static int
text_update(genmon_priv *gm)
{
    FILE *fp;  
    char text[256];
    char tooltip_line[gm->max_tooltip_lines_count];
    char tooltip_text[gm->max_tooltip_line_len*gm->max_tooltip_lines_count];
    char *markup;
    tooltip_text[0] = 0;
    tooltip_line[0] = 0;
    int len;
    int count = 0;

    ENTER;
    fp = popen(gm->command, "r");
    if (fgets(text, sizeof(text), fp));
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
    fp = popen(gm->tooltip, "r");
    while((fgets(tooltip_line,gm->max_tooltip_line_len, fp) != NULL) && (count < gm->max_tooltip_lines_count))
    {
        tooltip_line[strlen(tooltip_line)-1] = '\n';
        strcat(tooltip_text,tooltip_line);
        count++;
    };
    pclose(fp);
    len = strlen(tooltip_text) - 1;
    if (len >=0)
        {
            if(tooltip_text[len] == '\n')
                tooltip_text[len] = 0;
            gtk_widget_set_tooltip_markup(gm->plugin.pwid, tooltip_text);
        }
    else
        {
            gtk_widget_set_tooltip_markup(gm->plugin.pwid, "");
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
    gm->max_tooltip_line_len = 100;
    gm->max_tooltip_lines_count = 50;    
        
    XCG(p->xc, "Command", &gm->command, str);
    XCG(p->xc, "TextSize", &gm->textsize, str);
    XCG(p->xc, "TextColor", &gm->textcolor, str);
    XCG(p->xc, "PollingTime", &gm->time, int);
    XCG(p->xc, "MaxTextLength", &gm->max_text_len, int);
    XCG(p->xc, "ToolTip", &gm->tooltip, str);
    XCG(p->xc, "ToolTipLineLenght", (int*) &gm->max_tooltip_line_len, int);
    XCG(p->xc, "ToolTipLinesCount", (int*) &gm->max_tooltip_lines_count, int);
    
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
