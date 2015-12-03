

#include <glib.h>
#include <glib/gstdio.h>
#include <string.h>
#include <time.h>

#include "panel.h"
#include "xconf.h"

//#define DEBUGPRN
#include "dbg.h"

static const char desktop_ent[] = "Desktop Entry";
static const gchar app_dir_name[] = "applications";

typedef struct {
    gchar *name;
    gchar *icon;
    gchar *local_name;
} cat_info;

static cat_info main_cats[] = {
    { "AudioVideo", "applications-multimedia", c_("Audio & Video") },
    { "Education",  "applications-other", c_("Education") },
    { "Game",       "applications-games", c_("Game") },
    { "Graphics",   "applications-graphics", c_("Graphics") },
    { "Network",    "applications-internet", c_("Network") },
    { "Office",     "applications-office", c_("Office") },
    { "Settings",   "preferences-system", c_("Settings") },
    { "System",     "applications-system", c_("System") },
    { "Utility",    "applications-utilities", c_("Utilities") },
    { "Development","applications-development", c_("Development") },
};

static void
do_app_file(GHashTable *ht, const gchar *file)
{
    GKeyFile *f;
    gchar *name, *icon, *action,*dot;
    gchar **cats, **tmp;
    xconf *ixc, *vxc, *mxc;
    
    ENTER;
    DBG("desktop: %s\n", file);
    /* get values */
    name = icon = action = dot = NULL;
    cats = tmp = NULL;
    f = g_key_file_new();
    if (!g_key_file_load_from_file(f, file, 0, NULL))
        goto out;
    if (g_key_file_get_boolean(f, desktop_ent, "NoDisplay", NULL))
    {
        DBG("\tNoDisplay\n");
        goto out;
    }
    if (g_key_file_has_key(f, desktop_ent, "OnlyShowIn", NULL))
    {
        DBG("\tOnlyShowIn\n");
        goto out;
    }
    if (!(action = g_key_file_get_string(f, desktop_ent, "Exec", NULL)))
    {
        DBG("\tNo Exec\n");
        goto out;
    }
    if (!(cats = g_key_file_get_string_list(f,
                desktop_ent, "Categories", NULL, NULL)))
    {
        DBG("\tNo Categories\n");
        goto out;
    }
    if (!(name = g_key_file_get_locale_string(f,
                desktop_ent, "Name", NULL, NULL)))
    {
        DBG("\tNo Name\n");
        goto out;
    }
    icon = g_key_file_get_string(f, desktop_ent, "Icon", NULL);
    if (!icon)
        DBG("\tNo Icon\n");

    /* ignore program arguments */
    while ((dot = strchr(action, '%'))) {
        if (dot[1] != '\0')
            dot[0] = dot[1] = ' ';
    }
    DBG("action: %s\n", action);
    /* if icon is NOT an absolute path but has an extention,
     * e.g. firefox.png, then drop an extenstion to allow to load it
     * as themable icon */
    if (icon && icon[0] != '/' && (dot = strrchr(icon, '.' )) &&
        !(strcasecmp(dot + 1, "png") && strcasecmp(dot + 1, "svg")))
    {
        *dot = '\0';
    }
    DBG("icon: %s\n", icon);
    
    for (mxc = NULL, tmp = cats; *tmp; tmp++) 
        if ((mxc = g_hash_table_lookup(ht, *tmp)))
            break;
    if (!mxc)
    {
        DBG("\tUnknown categories\n");
        goto out;
    }
    
    ixc = xconf_new("item", NULL);
    xconf_append(mxc, ixc);
    if (icon)
    {
        vxc = xconf_new((icon[0] == '/') ? "image" : "icon", icon);
        xconf_append(ixc, vxc);
    }
    vxc = xconf_new("name", name);
    xconf_append(ixc, vxc);
    vxc = xconf_new("action", action);
    xconf_append(ixc, vxc);

out:
    g_free(icon);
    g_free(name);
    g_free(action);
    g_strfreev(cats);
    g_key_file_free(f);
}

static void
do_app_dir_real(GHashTable *ht, const gchar *dir)
{
    GDir *d = NULL;
    gchar *cwd;
    const gchar *name;

    ENTER;
    DBG("%s\n", dir);
    cwd = g_get_current_dir();
    if (g_chdir(dir))
    {
        DBG("can't chdir to %s\n", dir);
        goto out;
    }
    if (!(d = g_dir_open(".", 0, NULL)))
    {
        ERR("can't open dir %s\n", dir);
        goto out;
    }
    
    while ((name = g_dir_read_name(d)))
    {    
        if (g_file_test(name, G_FILE_TEST_IS_DIR))
        {
            do_app_dir_real(ht, name);
            continue;
        }
        if (!g_str_has_suffix(name, ".desktop"))
            continue;
        do_app_file(ht, name);
    }
    
out:
    if (d)
        g_dir_close(d);
    g_chdir(cwd);
    g_free(cwd);
    RET();
}

static void
do_app_dir(GHashTable *ht, const gchar *dir)
{
    gchar *cwd;

    ENTER;    
    cwd = g_get_current_dir();
    DBG("%s\n", dir);
    if (g_hash_table_lookup(ht, dir))
    {
        DBG("already visited\n");
        goto out;
    }
    g_hash_table_insert(ht, (gpointer) dir, ht);
    if (g_chdir(dir))
    {
        ERR("can't chdir to %s\n", dir);
        goto out;
    }
    do_app_dir_real(ht, app_dir_name);

out:
    g_chdir(cwd);
    g_free(cwd);
    RET();
}

static int
xconf_cmp_names(gpointer a, gpointer b)
{
    xconf *aa = a, *bb = b;
    gchar *s1 = NULL, *s2 = NULL;
    int ret;
    
    ENTER;
    XCG(aa, "name", &s1, str);
    XCG(bb, "name", &s2, str);
    ret = g_strcmp0(s1, s2);
    DBG("cmp %s %s - %d\n", s1, s2, ret);
    RET(ret);
}

static gboolean
dir_changed(const gchar *dir, time_t btime)
{
    GDir *d = NULL;
    gchar *cwd;
    const gchar *name;
    gboolean ret = FALSE;
    struct stat buf;
    
    ENTER;
    DBG("%s\n", dir);
    if (g_stat(dir, &buf))
        return FALSE;
    DBG("dir=%s ct=%lu mt=%lu\n", dir, buf.st_ctime, buf.st_mtime);
    if ((ret = buf.st_mtime > btime))
        return TRUE;
    
    cwd = g_get_current_dir();
    if (g_chdir(dir))
    {
        DBG("can't chdir to %s\n", dir);
        goto out;
    }
    if (!(d = g_dir_open(".", 0, NULL)))
    {
        ERR("can't open dir %s\n", dir);
        goto out;
    }
    
    while (!ret && (name = g_dir_read_name(d)))
    {
        if (g_file_test(name, G_FILE_TEST_IS_DIR))
            ret = dir_changed(name, btime);
        else if (!g_str_has_suffix(name, ".desktop"))
            continue;
        else if (g_stat(name, &buf))
            continue;
        DBG("name=%s ct=%lu mt=%lu\n", name, buf.st_ctime, buf.st_mtime);
        ret = buf.st_mtime > btime;
    }
out:
    if (d)
        g_dir_close(d);
    g_chdir(cwd);
    g_free(cwd);
    RET(ret);
}

gboolean
systemmenu_changed(time_t btime)
{
    const gchar * const * dirs;
    gboolean ret = FALSE;
    gchar *cwd = g_get_current_dir();
    
    for (dirs = g_get_system_data_dirs(); *dirs && !ret; dirs++)
    {
        g_chdir(*dirs);
        ret = dir_changed(app_dir_name, btime);
    }

    DBG("btime=%lu\n", btime);
    if (!ret)
    {
        g_chdir(g_get_user_data_dir());
        ret = dir_changed(app_dir_name, btime);
    }
    g_chdir(cwd);
    g_free(cwd);
    return ret;
}

xconf *
xconf_new_from_systemmenu()
{
    xconf *xc, *mxc, *tmp;
    GSList *w;
    GHashTable *ht;
    int i;
    const gchar * const * dirs;

    /* Create category menus */
    ht = g_hash_table_new(g_str_hash, g_str_equal);
    xc = xconf_new("systemmenu", NULL);
    for (i = 0; i < G_N_ELEMENTS(main_cats); i++)
    {
        mxc = xconf_new("menu", NULL);
        xconf_append(xc, mxc);

        tmp = xconf_new("name", _(main_cats[i].local_name));
        xconf_append(mxc, tmp);
        
        tmp = xconf_new("icon", main_cats[i].icon);
        xconf_append(mxc, tmp);

        g_hash_table_insert(ht, main_cats[i].name, mxc);
    }

    /* Read applications and add them to categories */

    for (dirs = g_get_system_data_dirs(); *dirs; dirs++)
        do_app_dir(ht, *dirs);
    do_app_dir(ht, g_get_user_data_dir());
  
    /* Delete empty categories */
retry:
    for (w = xc->sons; w; w = g_slist_next(w))
    {
        tmp = w->data;
        if (!xconf_find(tmp, "item", 0))
        {
            xconf_del(tmp, FALSE);
            goto retry;
        }
    }
    
    /* Sort  */
    xc->sons = g_slist_sort(xc->sons, (GCompareFunc) xconf_cmp_names);
    for (w = xc->sons; w; w = g_slist_next(w))
    {
        tmp = w->data;
        tmp->sons = g_slist_sort(tmp->sons, (GCompareFunc) xconf_cmp_names);
    }
    
    g_hash_table_destroy(ht);
    
    return xc;
}
