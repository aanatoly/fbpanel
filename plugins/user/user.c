
#include "misc.h"
#include "../menu/menu.h"
#include <stdlib.h>
#include <string.h>

//#define DEBUGPRN
#include "dbg.h"

typedef struct {
    menu_priv chart;
    gint dummy;
} user_priv;

static menu_class *k;


static void user_destructor(plugin_instance *p);


static int
user_constructor(plugin_instance *p)
{
    user_priv *c G_GNUC_UNUSED = (user_priv *) p;
    gchar *image = NULL;
    gchar *icon = NULL;

    if (!(k = class_get("menu")))
        RET(0);
    // FIXME: if `gravatar` is set, download it and set `image`
    // ...
    XCG(p->xc, "image", &image, str);
    image = expand_tilda(image);
    XCG(p->xc, "icon", &icon, str);
    if (!(image || icon))
        XCS(p->xc, "icon", "avatar-default", value);
    if (!PLUGIN_CLASS(k)->constructor(p))
        RET(0);

    gtk_widget_set_tooltip_markup(p->pwid, "<b>User</b>");
    RET(1);
}


static void
user_destructor(plugin_instance *p)
{
    user_priv *c G_GNUC_UNUSED = (user_priv *) p;

    ENTER;
    PLUGIN_CLASS(k)->destructor(p);
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
