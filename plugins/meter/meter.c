
#include "plugin.h"
#include "panel.h"
#include "meter.h"


//#define DEBUGPRN
#include "dbg.h"
float roundf(float x);

/* level - per cent level from 0 to 100 */
static void
meter_set_level(meter_priv *m, int level)
{
    int i;
    GdkPixbuf *pb;

    ENTER;
    if (m->level == level)
        RET();
    if (!m->num)
        RET();
    if (level < 0 || level > 100) {
        ERR("meter: illegal level %d\n", level);
        RET();
    }
    i = roundf((gfloat) level / 100 * (m->num - 1));
    DBG("level=%f icon=%d\n", level, i);
    if (i != m->cur_icon) {
        m->cur_icon = i;
        pb = gtk_icon_theme_load_icon(icon_theme, m->icons[i],
            m->size, GTK_ICON_LOOKUP_FORCE_SIZE, NULL);
        DBG("loading icon '%s' %s\n", m->icons[i], pb ? "ok" : "failed");
        gtk_image_set_from_pixbuf(GTK_IMAGE(m->meter), pb);
        if (pb)
            g_object_unref(G_OBJECT(pb));
    }
    m->level = level;
    RET();
}

static void
meter_set_icons(meter_priv *m, gchar **icons)
{
    gchar **s;

    ENTER;
    if (m->icons == icons)
        RET();
    for (s = icons; *s; s++)
        DBG("icon %s\n", *s);
    m->num = (s - icons);
    DBG("total %d icons\n", m->num);
    m->icons = icons;
    m->cur_icon = -1;
    m->level = -1;
    RET();
}
static void
update_view(meter_priv *m)
{
    ENTER;
    m->cur_icon = -1;
    meter_set_level(m, m->level);
    RET();
}

static int
meter_constructor(plugin_instance *p)
{
    meter_priv *m;
    
    ENTER;
    m = (meter_priv *) p;
    m->meter = gtk_image_new();
    gtk_misc_set_alignment(GTK_MISC(m->meter), 0.5, 0.5);
    gtk_misc_set_padding(GTK_MISC(m->meter), 0, 0);
    gtk_widget_show(m->meter);
    gtk_container_add(GTK_CONTAINER(p->pwid), m->meter);
    m->cur_icon = -1;
    m->size = p->panel->max_elem_height;
    m->itc_id = g_signal_connect_swapped(G_OBJECT(icon_theme),
        "changed", (GCallback) update_view, m);
    RET(1);
}

static void
meter_destructor(plugin_instance *p)
{
    meter_priv *m = (meter_priv *) p;

    ENTER;
    g_signal_handler_disconnect(G_OBJECT(icon_theme), m->itc_id);
    RET();
}

static meter_class class = {
    .plugin = {
        .type        = "meter",
        .name        = "Meter",
        .description = "Basic meter plugin",
        .version     = "1.0",
        .priv_size   = sizeof(meter_priv),

        .constructor = meter_constructor,
        .destructor  = meter_destructor,
    },
    .set_level = meter_set_level,
    .set_icons = meter_set_icons,
};


static plugin_class *class_ptr = (plugin_class *) &class;
