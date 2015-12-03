/* dclock is an adaptation of blueclock by Jochen Baier <email@Jochen-Baier.de> */

#include <time.h>
#include <sys/time.h>
#include <sys/types.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>


#include "panel.h"
#include "misc.h"
#include "plugin.h"

//#define DEBUGPRN
#include "dbg.h"


#define TOOLTIP_FMT    "%A %x"
#define CLOCK_24H_FMT  "%R"
#define CLOCK_12H_FMT  "%I:%M"

#define CLOCK_24H_FMT     "%R"
#define CLOCK_24H_SEC_FMT "%T"
#define CLOCK_12H_FMT     "%I:%M"
#define CLOCK_12H_SEC_FMT "%I:%M:%S"

#define COLON_WIDTH   7
#define COLON_HEIGHT  5
#define VCOLON_WIDTH   10
#define VCOLON_HEIGHT  6
#define DIGIT_WIDTH   11
#define DIGIT_HEIGHT  15
#define SHADOW 2

#define STR_SIZE  64

enum { DC_24H, DC_12H };

xconf_enum hours_view_enum[] = {
    { .num = DC_24H, .str = "24" },
    { .num = DC_12H, .str = "12" },
    { .num = 0, .str = NULL },
};

typedef struct
{
    plugin_instance plugin;
    GtkWidget *main;
    GtkWidget *calendar_window;
    gchar *tfmt, tstr[STR_SIZE];
    gchar *cfmt, cstr[STR_SIZE];
    char *action;
    int timer;
    GdkPixbuf *glyphs; //vert row of '0'-'9' and ':'
    GdkPixbuf *clock;
    guint32 color;
    gboolean show_seconds;
    gboolean hours_view;
    GtkOrientation orientation;
} dclock_priv;

//static dclock_priv me;

static GtkWidget *
dclock_create_calendar()
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
          
    calendar = gtk_calendar_new();
    gtk_calendar_display_options(
        GTK_CALENDAR(calendar),
        GTK_CALENDAR_SHOW_WEEK_NUMBERS | GTK_CALENDAR_SHOW_DAY_NAMES
        | GTK_CALENDAR_SHOW_HEADING);
    gtk_container_add(GTK_CONTAINER(win), GTK_WIDGET(calendar));
 
    return win;
}  

static gboolean
clicked(GtkWidget *widget, GdkEventButton *event, dclock_priv *dc)
{
    ENTER;
    if (event->type == GDK_BUTTON_PRESS && event->button == 3
            && event->state & GDK_CONTROL_MASK)
    {
        RET(FALSE);
    }
    if (dc->action != NULL)
        g_spawn_command_line_async(dc->action, NULL);
    else
    {
        if (dc->calendar_window == NULL)
        {
            dc->calendar_window = dclock_create_calendar();
            gtk_widget_show_all(dc->calendar_window);
            gtk_widget_set_tooltip_markup(dc->plugin.pwid, NULL);
        }
        else
        {
            gtk_widget_destroy(dc->calendar_window);
            dc->calendar_window = NULL;
        }
    }
    RET(TRUE);
}

static gint
clock_update(dclock_priv *dc)
{
    char output[STR_SIZE], *tmp, *utf8;
    time_t now;
    struct tm * detail;
    int i, x, y;
    
    ENTER;
    time(&now);
    detail = localtime(&now);

    if (!strftime(output, sizeof(output), dc->cfmt, detail))
        strcpy(output, "  :  ");
    if (strcmp(dc->cstr, output))
    {
        strncpy(dc->cstr, output, sizeof(dc->cstr));
        x = y = SHADOW;
        for (tmp = output; *tmp; tmp++)
        {
            DBGE("%c", *tmp);
            if (isdigit(*tmp))
            {
                i = *tmp - '0';
                gdk_pixbuf_copy_area(dc->glyphs, i * 20, 0,
                    DIGIT_WIDTH, DIGIT_HEIGHT,
                    dc->clock, x, y);
                x += DIGIT_WIDTH;
            }
            else if (*tmp == ':')
            {
                if (dc->orientation == GTK_ORIENTATION_HORIZONTAL) {
                    gdk_pixbuf_copy_area(dc->glyphs, 10 * 20, 0,
                        COLON_WIDTH, DIGIT_HEIGHT - 2,
                        dc->clock, x, y + 2);
                    x += COLON_WIDTH;
                } else {
                    x = SHADOW;
                    y += DIGIT_HEIGHT;
                    gdk_pixbuf_copy_area(dc->glyphs, 10 * 20, 0,
                        VCOLON_WIDTH, VCOLON_HEIGHT,
                        dc->clock, x + DIGIT_WIDTH / 2, y);
                    y += VCOLON_HEIGHT;
                }
            }
            else
            {
                ERR("dclock: got %c while expecting for digit or ':'\n", *tmp);
            }
        }
        DBG("\n");
        gtk_widget_queue_draw(dc->main);
    }
    
    if (dc->calendar_window || !strftime(output, sizeof(output),
            dc->tfmt, detail))
        output[0] = 0;    
    if (strcmp(dc->tstr, output))
    {
        strcpy(dc->tstr, output);
        if (dc->tstr[0] && (utf8 = g_locale_to_utf8(output, -1,
                    NULL, NULL, NULL)))
        {
            gtk_widget_set_tooltip_markup(dc->plugin.pwid, utf8);
            g_free(utf8);
        }
        else
            gtk_widget_set_tooltip_markup(dc->plugin.pwid, NULL);        
    }
    RET(TRUE);
}

static void
dclock_set_color(GdkPixbuf *glyphs, guint32 color)
{
    guchar *p1, *p2;
    int w, h;
    guint r, g, b;
    
    ENTER;
    p1 = gdk_pixbuf_get_pixels(glyphs);
    h = gdk_pixbuf_get_height(glyphs);
    r = (color & 0x00ff0000) >> 16;
    g = (color & 0x0000ff00) >> 8;
    b = (color & 0x000000ff);
    DBG("%dx%d: %02x %02x %02x\n",
        gdk_pixbuf_get_width(glyphs), gdk_pixbuf_get_height(glyphs), r, g, b);
    while (h--)
    {
        for (p2 = p1, w = gdk_pixbuf_get_width(glyphs); w; w--, p2 += 4)
        {
            DBG("here %02x %02x %02x %02x\n", p2[0], p2[1], p2[2], p2[3]);
            if (p2[3] == 0 || !(p2[0] || p2[1] || p2[2]))
                continue;
            p2[0] = r;
            p2[1] = g;
            p2[2] = b;
        }
        p1 += gdk_pixbuf_get_rowstride(glyphs);
    }
    DBG("here\n");
    RET();
}

static void
dclock_create_pixbufs(dclock_priv *dc)
{
    int width, height;
    GdkPixbuf *ch, *cv;
    
    ENTER;
    width = height = SHADOW;
    width += COLON_WIDTH + 4 * DIGIT_WIDTH;
    height += DIGIT_HEIGHT;
    if (dc->show_seconds)
        width += COLON_WIDTH + 2 * DIGIT_WIDTH;
    if (dc->orientation == GTK_ORIENTATION_VERTICAL) {
        DBG("width=%d height=%d aw=%d\n", width, height, dc->plugin.panel->aw);
        if (width < dc->plugin.panel->aw) {
            dc->orientation = GTK_ORIENTATION_HORIZONTAL;
            goto done;
        }
        width = height = SHADOW;
        ch = gdk_pixbuf_new_subpixbuf(dc->glyphs, 200, 0, 8, 8);
        cv = gdk_pixbuf_rotate_simple(ch, 270);
        gdk_pixbuf_copy_area(cv, 0, 0, 8, 8, ch, 0, 0);
        g_object_unref(cv);
        g_object_unref(ch);
        height += DIGIT_HEIGHT * 2 + VCOLON_HEIGHT;
        width += DIGIT_WIDTH * 2;
        if (dc->show_seconds)
            height += VCOLON_HEIGHT + DIGIT_HEIGHT;
    }
done:
    dc->clock = gdk_pixbuf_new(GDK_COLORSPACE_RGB, TRUE, 8, width, height);
    DBG("width=%d height=%d\n", width, height);
    gdk_pixbuf_fill(dc->clock, 0);
    RET();
}

static void
dclock_destructor(plugin_instance *p)
{
    dclock_priv *dc = (dclock_priv *)p;

    ENTER;
    if (dc->timer)
        g_source_remove(dc->timer);
    gtk_widget_destroy(dc->main);
    RET();
}

static int
dclock_constructor(plugin_instance *p)
{
    gchar *color_str;
    dclock_priv *dc;
    //int width;
    
    ENTER;
    DBG("dclock: use 'tclock' plugin for text version of a time and date\n");
    dc = (dclock_priv *) p;
    dc->glyphs = gdk_pixbuf_new_from_file(IMGPREFIX "/dclock_glyphs.png", NULL);
    if (!dc->glyphs)
        RET(0);

    dc->cfmt = NULL;
    dc->tfmt = TOOLTIP_FMT;
    dc->action = NULL;
    dc->color = 0xff000000;
    dc->show_seconds = FALSE;
    dc->hours_view = DC_24H;
    dc->orientation = p->panel->orientation;
    color_str = NULL;
    XCG(p->xc, "TooltipFmt", &dc->tfmt, str);
    XCG(p->xc, "ClockFmt", &dc->cfmt, str);
    XCG(p->xc, "ShowSeconds", &dc->show_seconds, enum, bool_enum);
    XCG(p->xc, "HoursView", &dc->hours_view, enum, hours_view_enum);
    XCG(p->xc, "Action", &dc->action, str);
    XCG(p->xc, "Color", &color_str, str);
    if (dc->cfmt)
    {
        ERR("dclock: ClockFmt option is deprecated. Please use\n"
            "following options instead\n"
            "  ShowSeconds = false | true\n"
            "  HoursView = 12 | 24\n");
        xconf_del(xconf_get(p->xc, "ClockFmt"), FALSE);
        dc->cfmt = NULL;
    }
    if (color_str)
    {
        GdkColor color;
        if (gdk_color_parse (color_str, &color)) 
            dc->color = gcolor2rgb24(&color);
    }
    if (dc->hours_view == DC_24H)
        dc->cfmt = (dc->show_seconds) ? CLOCK_24H_SEC_FMT : CLOCK_24H_FMT;
    else
        dc->cfmt = (dc->show_seconds) ? CLOCK_12H_SEC_FMT : CLOCK_12H_FMT;
    dclock_create_pixbufs(dc);
    if (dc->color != 0xff000000)
        dclock_set_color(dc->glyphs, dc->color);
  
    dc->main = gtk_image_new_from_pixbuf(dc->clock);
    gtk_misc_set_alignment(GTK_MISC(dc->main), 0.5, 0.5);
    gtk_misc_set_padding(GTK_MISC(dc->main), 1, 1);
    gtk_container_add(GTK_CONTAINER(p->pwid), dc->main);
    //gtk_widget_show(dc->clockw);
    g_signal_connect (G_OBJECT (p->pwid), "button_press_event",
            G_CALLBACK (clicked), (gpointer) dc);
    gtk_widget_show_all(dc->main);
    dc->timer = g_timeout_add(1000, (GSourceFunc) clock_update, (gpointer)dc);
    clock_update(dc);
    
    RET(1);
}


static plugin_class class = {
    .fname       = NULL,
    .count       = 0,
    .type        = "dclock",
    .name        = "Digital Clock",
    .version     = "1.0",
    .description = "Digital clock with tooltip",
    .priv_size   = sizeof(dclock_priv),

    .constructor = dclock_constructor,
    .destructor  = dclock_destructor,
};
static plugin_class *class_ptr = (plugin_class *) &class;
