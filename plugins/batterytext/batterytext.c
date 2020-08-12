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

static float
read_bat_value(const char *dn, const char *fn)
{
    FILE *fp;
    float value = -1;
    char value_path[256];
    g_snprintf(value_path, sizeof(value_path), "%s/%s", dn, fn);
    fp = fopen(value_path, "r");
    if (fp != NULL) {
        if (fscanf(fp, "%f", &value) != 1)
            value = -1;
        fclose(fp);
    }
    return value;
}

static int
text_update(batterytext_priv *gm)
{
    FILE *fp_status;
    char battery_status[256];
    char *markup;
    char *tooltip;
    char buffer[256];
    float energy_full_design = -1;
    float energy_full = -1;
    float energy_now = -1;
    float power_now = -1;
    int discharging = 0;
    float charge_ratio = 0;
    int charge_time = 0;

    ENTER;
    energy_full_design = read_bat_value(gm->battery, "energy_full_design");
    energy_full = read_bat_value(gm->battery, "energy_full");
    energy_now = read_bat_value(gm->battery, "energy_now");
    power_now = read_bat_value(gm->battery, "power_now");

    snprintf(battery_status, sizeof(battery_status), "%s/status", gm->battery);
    fp_status = fopen(battery_status, "r");
    if (fp_status != NULL) {
        while ((fgets(buffer, sizeof(buffer), fp_status)) != NULL) {
            if (strstr(buffer, "Discharging") != NULL)
                discharging = 1;
        }
        fclose(fp_status);
    }

    if ((energy_full_design >= 0) && (energy_now >= 0)) {
        if (gm->design)
            charge_ratio = 100 * energy_now / energy_full_design;
        else
            charge_ratio = 100 * energy_now / energy_full;
        if (discharging)
        {
            markup = g_markup_printf_escaped("<span size='%s' foreground='red'><b>%.2f-</b></span>",
                gm->textsize, charge_ratio);
            charge_time = (int)(energy_now / power_now * 3600);
        }
        else
        {
            markup = g_markup_printf_escaped("<span size='%s' foreground='green'><b>%.2f+</b></span>",
                gm->textsize, charge_ratio);
            charge_time = (int)((energy_full - energy_now) / power_now * 3600);
        }
        tooltip = g_markup_printf_escaped("%02d:%02d:%02d",
            charge_time / 3600, (charge_time / 60) % 60, charge_time % 60);
        gtk_label_set_markup (GTK_LABEL(gm->main), markup);
        g_free(markup);
        gtk_widget_set_tooltip_markup (gm->main, tooltip);
        g_free(tooltip);
    }
    else
    {
        gtk_label_set_markup (GTK_LABEL(gm->main), "N/A");
        gtk_widget_set_tooltip_markup (gm->main, "N/A");
    }
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
    gm->battery = "/sys/class/power_supply/BAT0";

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
