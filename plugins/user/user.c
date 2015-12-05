
#include "misc.h"
#include "run.h"
#include "../menu/menu.h"
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <signal.h>


//#define DEBUGPRN
#include "dbg.h"

typedef struct {
    menu_priv chart;
    gint dummy;
    guint sid;
    GPid pid;
} user_priv;

static menu_class *k;


static void user_destructor(plugin_instance *p);

#define GRAVATAR_LEN  300

static void
fetch_gravatar_done(GPid pid, gint status, gpointer data)
{
    user_priv *c G_GNUC_UNUSED = data;
    plugin_instance *p G_GNUC_UNUSED = data;
    gchar *image = NULL, *icon = NULL;

    ENTER;
    DBG("status %d\n", status);
    g_spawn_close_pid(c->pid);
    c->pid = 0;
    c->sid = 0;

    if (status)
        RET();
    DBG("rebuild menu\n");
    XCG(p->xc, "icon", &icon, strdup);
    XCG(p->xc, "image", &image, strdup);
    XCS(p->xc, "image", image, value);
    xconf_del(xconf_find(p->xc, "icon", 0), FALSE);
    PLUGIN_CLASS(k)->destructor(p);
    PLUGIN_CLASS(k)->constructor(p);
    if (image) {
        XCS(p->xc, "image", image, value);
        g_free(image);
    }
    if (icon) {
        XCS(p->xc, "icon", icon, value);
        g_free(icon);
    }
    RET();
}


static gboolean
fetch_gravatar(gpointer data)
{
    user_priv *c G_GNUC_UNUSED = data;
    plugin_instance *p G_GNUC_UNUSED = data;
    GChecksum *cs;
    gchar *gravatar = NULL;
    gchar buf[GRAVATAR_LEN];
    // FIXME: select more secure path
    gchar *image = "/tmp/gravatar";
    gchar *argv[] = { "wget", "-q", "-O", image, buf, NULL };

    ENTER;
    cs = g_checksum_new(G_CHECKSUM_MD5);
    XCG(p->xc, "gravataremail", &gravatar, str);
    g_checksum_update(cs, (guchar *) gravatar, -1);
    snprintf(buf, sizeof(buf), "http://www.gravatar.com/avatar/%s",
        g_checksum_get_string(cs));
    g_checksum_free(cs);
    DBG("gravatar '%s'\n", buf);
    c->pid = run_app_argv(argv);
    c->sid = g_child_watch_add(c->pid, fetch_gravatar_done, data);
    RET(FALSE);
}


static int
user_constructor(plugin_instance *p)
{
    user_priv *c G_GNUC_UNUSED = (user_priv *) p;
    gchar *image = NULL;
    gchar *icon = NULL;
    gchar *gravatar = NULL;

    ENTER;
    if (!(k = class_get("menu")))
        RET(0);
    XCG(p->xc, "image", &image, str);
    XCG(p->xc, "icon", &icon, str);
    if (!(image || icon))
        XCS(p->xc, "icon", "avatar-default", value);
    if (!PLUGIN_CLASS(k)->constructor(p))
        RET(0);
    XCG(p->xc, "gravataremail", &gravatar, str);
    DBG("gravatar email '%s'\n", gravatar);
    if (gravatar)
        g_timeout_add(300, fetch_gravatar, p);
    gtk_widget_set_tooltip_markup(p->pwid, "<b>User</b>");
    RET(1);
}


static void
user_destructor(plugin_instance *p)
{
    user_priv *c G_GNUC_UNUSED = (user_priv *) p;

    ENTER;
    PLUGIN_CLASS(k)->destructor(p);
    if (c->pid)
        kill(c->pid, SIGKILL);
    if (c->sid)
        g_source_remove(c->sid);
    class_put("menu");
    RET();
}


static plugin_class class = {
    .count       = 0,
    .type        = "user",
    .name        = "User menu",
    .version     = "1.0",
    .description = "User photo and menu of user actions",
    .priv_size   = sizeof(user_priv),

    .constructor = user_constructor,
    .destructor  = user_destructor,
};
static plugin_class *class_ptr = (plugin_class *) &class;
