/* batterytext_priv.c -- Generic monitor plugin for fbpanel
 *
 * Copyright (C) 2017 Fred Stober <mail@fredstober.de>
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

typedef struct {
    plugin_instance plugin;
    int design;
    int time;
    char *textsize;
    char *battery;
    int timer;
    GtkWidget *main;
} batterytext_priv;

static int
text_update(batterytext_priv *gm)
{
    FILE *fp_info;
    FILE *fp_state;
    char battery_state[256];
    char battery_info[256];
    char *markup;
    char buffer[256];
    int capacity_design = -1;
    int capacity_last = -1;
    int discharging = 0;
    int charge_present = -1;
    int chrg_rate = -1;
    float charge_ratio = 0;

    ENTER;
    g_snprintf(battery_info, sizeof(battery_info), "%s/info", gm->battery);
    g_snprintf(battery_state, sizeof(battery_state), "%s/state", gm->battery);

    snprintf(battery_info, sizeof(battery_info), "%s/info", "/proc/acpi/battery/BAT1");
    snprintf(battery_state, sizeof(battery_state), "%s/state", "/proc/acpi/battery/BAT1");

    fp_info = fopen(battery_info, "r");
    if (fp_info != NULL) {
        while ((fgets(buffer, sizeof(buffer), fp_info)) != NULL) {
            if (sscanf(buffer, "design capacity: %d", &capacity_design));
            else if (sscanf(buffer, "last full capacity: %d", &capacity_last));
        }
        fclose(fp_info);
    }

    fp_state = fopen(battery_state, "r");
    if (fp_state != NULL) {
        while ((fgets(buffer, sizeof(buffer), fp_state)) != NULL) {
            if (strstr(buffer, "discharging\n") != NULL)
                discharging = 1;
            if (sscanf(buffer, "remaining capacity: %d", &charge_present));
            else if (sscanf(buffer, "present rate: %d", &chrg_rate));
        }
        fclose(fp_state);
    }

    if ((capacity_design >= 0) && (charge_present >= 0)) {
        if (gm->design)
            charge_ratio = 100 * charge_present / (double)capacity_design;
        else
            charge_ratio = 100 * charge_present / (double)capacity_last;
        if (discharging)
            markup = g_markup_printf_escaped("<span size='%s' foreground='red'><b>%.2f-</b></span>",
                gm->textsize, charge_ratio);
        else
            markup = g_markup_printf_escaped("<span size='%s' foreground='green'><b>%.2f+</b></span>",
                gm->textsize, charge_ratio);
        gtk_label_set_markup (GTK_LABEL(gm->main), markup);
        g_free(markup);
    }
    else
        gtk_label_set_markup (GTK_LABEL(gm->main), "N/A");
    RET(TRUE);
}

static void
batterytext_destructor(plugin_instance *p)
{
    batterytext_priv *gm = (batterytext_priv *) p;

    ENTER;
    if (gm->timer) {
        g_source_remove(gm->timer);
    }
    RET();
}

static int
batterytext_constructor(plugin_instance *p)
{
    batterytext_priv *gm;

    ENTER;
    gm = (batterytext_priv *) p;
    gm->design = False;
    gm->time = 500;
    gm->textsize = "medium";
    gm->battery = "/proc/acpi/battery/BAT1";

    XCG(p->xc, "DesignCapacity", &gm->design, enum, bool_enum);
    XCG(p->xc, "PollingTimeMs", &gm->time, int);
    XCG(p->xc, "TextSize", &gm->textsize, str);
    XCG(p->xc, "BatteryPath", &gm->battery, str);

    gm->main = gtk_label_new(NULL);
    text_update(gm);
    gtk_container_set_border_width (GTK_CONTAINER (p->pwid), 1);
    gtk_container_add(GTK_CONTAINER(p->pwid), gm->main);
    gtk_widget_show_all(p->pwid);
    gm->timer = g_timeout_add((guint) gm->time,
        (GSourceFunc) text_update, (gpointer) gm);

    RET(1);
}


static plugin_class class = {
    .count       = 0,
    .type        = "batterytext",
    .name        = "Generic Monitor",
    .version     = "0.1",
    .description = "Display battery usage in text form",
    .priv_size   = sizeof(batterytext_priv),

    .constructor = batterytext_constructor,
    .destructor  = batterytext_destructor,
};
static plugin_class *class_ptr = (plugin_class *) &class;
