
#include <string.h>
#include <ctype.h>

#define LEN 100
#define PROC_ACPI "/proc/acpi/battery/"


static gboolean
get_token_eq(gchar *buf, gchar *token, gchar *value, gboolean *ret)
{
    int len;
    gchar *var;

    ENTER;
    len = strlen(token);
    if (!(var = strstr(buf, token)))
        RET(FALSE);
    for (var = var + len; isspace(*var); var++) ;
    *ret = !strncmp(var, value, strlen(value));
    RET(TRUE);
}

static gboolean
get_token_int(gchar *buf, gchar *token, gint *value)
{
    int len;
    gchar *var;

    ENTER;
    len = strlen(token);
    if (!(var = strstr(buf, token)))
        RET(FALSE);
    for (var = var + len; isspace(*var); var++) ;
    if (sscanf(var, "%d", value) == 1)
        RET(TRUE);
    RET(FALSE);
}

static gboolean
read_proc(battery_priv *c, GString *path)
{
    int len, lfcap, rcap;
    gchar *buf;
    gboolean ret, exist, charging;

    ENTER;
    len = path->len;

    g_string_append(path, "/info");
    ret = g_file_get_contents(path->str, &buf, 0, NULL);
    DBG("reading %s %s\n", path->str, ret ? "ok" : "fail");
    g_string_truncate(path, len);
    if (!ret)
        RET(FALSE);
    ret = get_token_eq(buf, "present:", "yes", &exist)
        && exist && get_token_int(buf, "last full capacity:", &lfcap);

    g_free(buf);
    if (!ret)
        RET(FALSE);

    g_string_append(path, "/state");
    ret = g_file_get_contents(path->str, &buf, 0, NULL);
    DBG("reading %s %s\n", path->str, ret ? "ok" : "fail");
    g_string_truncate(path, len);
    if (!ret)
        RET(FALSE);
    ret = get_token_eq(buf, "present:", "yes", &exist)
        && exist
        && get_token_int(buf, "remaining capacity:", &rcap)
        && get_token_eq(buf, "charging state:", "charging", &charging);
    g_free(buf);
    if (!ret)
        RET(FALSE);
    DBG("battery=%s\nlast full capacity=%d\nremaining capacity=%d\n"
        "charging=%d\n",
        path->str, lfcap, rcap, charging);

    if (!(lfcap >= rcap && lfcap > 0 && rcap >= 0))
        RET(FALSE);

    c->exist = TRUE;
    c->charging = charging;
    c->level = (int) ((gfloat) rcap * 100 / (gfloat) lfcap);
    RET(TRUE);
}

static gboolean
battery_update_os_proc(battery_priv *c)
{
    GString *path;
    int len;
    GDir *dir;
    gboolean ret = FALSE;
    const gchar *file;

    ENTER;
    c->exist = FALSE;
    path = g_string_sized_new(200);
    g_string_append(path, PROC_ACPI);
    len = path->len;
    if (!(dir = g_dir_open(path->str, 0, NULL))) {
        DBG("can't open dir %s\n", path->str);
        goto out;
    }
    while (!ret && (file = g_dir_read_name(dir))) {
        g_string_append(path, file);
        DBG("testing %s\n", path->str);
        ret = g_file_test(path->str, G_FILE_TEST_IS_DIR);
        if (ret)
            ret = read_proc(c, path);
        g_string_truncate(path, len);
    }
    g_dir_close(dir);

out:
    g_string_free(path, TRUE);
    RET(ret);
}

static gboolean
battery_update_os_sys(battery_priv *c)
{
    ENTER;
    RET(FALSE);
}

static gboolean
battery_update_os(battery_priv *c)
{
    ENTER;
    RET(battery_update_os_proc(c) || battery_update_os_sys(c));
}
