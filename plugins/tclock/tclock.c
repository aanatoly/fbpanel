#include <time.h>
#include <sys/time.h>
#include <sys/types.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <locale.h>


#include "panel.h"
#include "misc.h"
#include "plugin.h"

//#define DEBUGPRN
#include "dbg.h"

/* 2010-04 Jared Minch  < jmminch@sourceforge.net >
 *     Calendar and transparency support
 *     See patch "2981313: Enhancements to 'tclock' plugin" on sf.net
 */

#define TOOLTIP_FMT    "%A %x"
#define CLOCK_24H_FMT  "<b>%R</b>"
#define CLOCK_12H_FMT  "%I:%M"

typedef struct {
    plugin_instance plugin;
    GtkWidget *main;
    GtkWidget *clockw;
    GtkWidget *calendar_window;
    char *tfmt;
    char *cfmt;
    char *action;
    short lastDay;
    int timer;
    int show_calendar;
    int show_tooltip;
    int numb_of_zones;
    int curr_zone;
    char **timezones;
    char **locales;
} tclock_priv;

static void tclock_calendar_cleanup (GtkDialog *dialog, gint code, tclock_priv *dc) {
    ENTER2;

    gtk_widget_destroy(dc->calendar_window);
    dc->calendar_window = NULL;

    RET();
}

static GtkWidget *
tclock_create_calendar(tclock_priv *dc)
{
    GtkWidget *calendar, *win;

    win = gtk_window_new(GTK_WINDOW_TOPLEVEL);
    gtk_window_set_default_size(GTK_WINDOW(win), 180, 180);
    gtk_window_set_decorated(GTK_WINDOW(win), FALSE);
    gtk_window_set_resizable(GTK_WINDOW(win), FALSE);
    gtk_container_set_border_width(GTK_CONTAINER(win), 5);
    gtk_window_set_skip_taskbar_hint(GTK_WINDOW(win), TRUE);
    gtk_window_set_skip_pager_hint(GTK_WINDOW(win), TRUE);
    gtk_window_set_position(GTK_WINDOW(win), GTK_WIN_POS_MOUSE);
    gtk_window_set_title(GTK_WINDOW(win), "calendar");
    gtk_window_stick(GTK_WINDOW(win));

    if (dc->numb_of_zones > 0) {
       if (dc->timezones[dc->curr_zone])
          setenv("TZ", dc->timezones[dc->curr_zone], 1);
       if (dc->locales[dc->curr_zone])
          setlocale (LC_ALL, dc->locales[dc->curr_zone]);
    }

    calendar = gtk_calendar_new();
    gtk_calendar_display_options(
        GTK_CALENDAR(calendar),
        GTK_CALENDAR_SHOW_WEEK_NUMBERS | GTK_CALENDAR_SHOW_DAY_NAMES
        | GTK_CALENDAR_SHOW_HEADING);
    gtk_container_add(GTK_CONTAINER(win), GTK_WIDGET(calendar));

    g_signal_connect (win, "delete-event",
                      G_CALLBACK (tclock_calendar_cleanup), dc);
    // for set "dc->calendar_window == NULL" when user close via Alt+F4

    return win;
}

static gint
clock_update(gpointer data)
{
    char output[256];
    time_t now;
    struct tm * detail;
    tclock_priv *dc;
    gchar *utf8;
    size_t rc;

    ENTER;
    g_assert(data != NULL);
    dc = (tclock_priv *)data;

    if (dc->numb_of_zones > 0) {
       if (dc->timezones[dc->curr_zone])
          setenv("TZ", dc->timezones[dc->curr_zone], 1);
       if (dc->locales[dc->curr_zone])
          setlocale (LC_ALL, dc->locales[dc->curr_zone]);
    }

    time(&now);
    detail = localtime(&now);
    rc = strftime(output, sizeof(output), dc->cfmt, detail) ;
    if (rc) {
        gtk_label_set_markup (GTK_LABEL(dc->clockw), output) ;
    }

    if (dc->show_tooltip) {
        if (dc->calendar_window) {
            gtk_widget_set_tooltip_markup(dc->main, NULL);
            dc->lastDay = 0;
        } else {
            if (detail->tm_mday != dc->lastDay) {
                dc->lastDay = detail->tm_mday;

                rc = strftime(output, sizeof(output), dc->tfmt, detail) ;
                if (rc && 
                    (utf8 = g_locale_to_utf8(output, -1, NULL, NULL, NULL))) {
                    gtk_widget_set_tooltip_markup(dc->main, utf8);
                    g_free(utf8);
                }
            }
        }
    }

    RET(TRUE);
}

static gboolean
clicked(GtkWidget *widget, GdkEventButton *event, tclock_priv *dc)
{
    ENTER;
    if (dc->action) {
        g_spawn_command_line_async(dc->action, NULL);
    } else if (dc->show_calendar) {
        if (dc->calendar_window == NULL)
        {
            dc->calendar_window = tclock_create_calendar(dc);
            gtk_widget_show_all(dc->calendar_window);
        }
        else
        {
            gtk_widget_destroy(dc->calendar_window);
            dc->calendar_window = NULL;
        }

        clock_update(dc);
    }
    RET(TRUE);
}

static gboolean
testev(GtkWidget *widget, GdkEventButton *event, tclock_priv *dc)
{
    ENTER;
    dc->curr_zone++;
    if (dc->curr_zone >= dc->numb_of_zones)
        dc->curr_zone=0;
    dc->lastDay = -1; // for refresh tooltip
    clock_update(dc);
    RET(TRUE);
}


static int
tclock_constructor(plugin_instance *p)
{
    tclock_priv *dc;
    int i;
    char buf[32];

    ENTER;
    dc = (tclock_priv *) p;
    dc->cfmt = CLOCK_24H_FMT;
    dc->tfmt = TOOLTIP_FMT;
    dc->action = NULL;
    dc->show_calendar = TRUE;
    dc->show_tooltip = TRUE;
    dc->numb_of_zones = 0;
    dc->curr_zone = 0;
    dc->timezones = NULL;
    dc->locales = NULL;
    XCG(p->xc, "TooltipFmt", &dc->tfmt, str);
    XCG(p->xc, "ClockFmt", &dc->cfmt, str);
    XCG(p->xc, "Action", &dc->action, str);
    XCG(p->xc, "ShowCalendar", &dc->show_calendar, enum, bool_enum);
    XCG(p->xc, "ShowTooltip", &dc->show_tooltip, enum, bool_enum);

    XCG(p->xc, "ZoneNum", &dc->numb_of_zones, int);
    if (dc->numb_of_zones > 0) {
        dc->timezones = calloc(dc->numb_of_zones, sizeof(char*));
        dc->locales = calloc(dc->numb_of_zones, sizeof(char*));

        for (i = 0; i < dc->numb_of_zones; i++) {
            dc->timezones[i] = NULL;
            sprintf(buf, "TimeZone%d", i);
            XCG(p->xc, buf, &(dc->timezones[i]), str);

            dc->locales[i] = NULL;
            sprintf(buf, "Locale%d", i);
            XCG(p->xc, buf, &(dc->locales[i]), str);
        }
    }

    dc->main = gtk_event_box_new();
    gtk_event_box_set_visible_window(GTK_EVENT_BOX(dc->main), FALSE);
    if (dc->action || dc->show_calendar)
        g_signal_connect (G_OBJECT (dc->main), "button_press_event",
              G_CALLBACK (clicked), (gpointer) dc);

    if (dc->numb_of_zones > 1)
        g_signal_connect_after(G_OBJECT(dc->main), "scroll-event",
              G_CALLBACK(testev), (gpointer) dc);

    dc->clockw = gtk_label_new(NULL);

    clock_update(dc);

    gtk_misc_set_alignment(GTK_MISC(dc->clockw), 0.5, 0.5);
    gtk_misc_set_padding(GTK_MISC(dc->clockw), 4, 0);
    gtk_label_set_justify(GTK_LABEL(dc->clockw), GTK_JUSTIFY_CENTER);
    gtk_container_add(GTK_CONTAINER(dc->main), dc->clockw);
    gtk_widget_show_all(dc->main);
    dc->timer = g_timeout_add(1000, (GSourceFunc) clock_update, (gpointer)dc);
    gtk_container_add(GTK_CONTAINER(p->pwid), dc->main);
    RET(1);
}

static void
tclock_destructor( plugin_instance *p )
{
    tclock_priv *dc = (tclock_priv *) p;

    ENTER;
    if (dc->timezones)
        free(dc->timezones);
    if (dc->locales)
        free(dc->locales);

    if (dc->timer)
        g_source_remove(dc->timer);
    gtk_widget_destroy(dc->main);
    RET();
}

static plugin_class class = {
    .count       = 0,
    .type        = "tclock",
    .name        = "Text Clock",
    .version     = "2.0",
    .description = "Text clock/date with tooltip",
    .priv_size   = sizeof(tclock_priv),

    .constructor = tclock_constructor,
    .destructor = tclock_destructor,
};
static plugin_class *class_ptr = (plugin_class *) &class;
