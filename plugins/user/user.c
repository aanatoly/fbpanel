

#include "../chart/chart.h"
#include <stdlib.h>
#include <string.h>

//#define DEBUGPRN
#include "dbg.h"

#if defined(__FreeBSD__)
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/sysctl.h>
#include <sys/time.h>
#include <net/if.h>
#include <net/if_mib.h>
#endif


#define CHECK_PERIOD   2 /* second */

struct user_stat {
    gulong tx, rx;
};

typedef struct {
    chart_priv chart;
    struct user_stat user_prev;
    int timer;
    char *iface;
#if defined(__FreeBSD__)
    size_t ifmib_row;
#endif
    gint max_tx;
    gint max_rx;
    gulong max;
    gchar *colors[2];
} user_priv;

static chart_class *k;


static void user_destructor(plugin_instance *p);


#if defined __linux__

#define init_user_stats(x)

static int
user_get_load_real(user_priv *c, struct user_stat *user)
{
    FILE *stat;
    char buf[256], *s = NULL;

    stat = fopen("/proc/net/dev", "r");
    if(!stat)
        return -1;
    if (fgets(buf, 256, stat));
    if (fgets(buf, 256, stat));

    while (!s && !feof(stat) && fgets(buf, 256, stat))
        s = g_strrstr(buf, c->iface);
    fclose(stat);
    if (!s)
        return -1;
    s = g_strrstr(s, ":");
    if (!s)
        return -1;
    s++;
    if (sscanf(s,
            "%lu  %*d     %*d  %*d  %*d  %*d   %*d        %*d       %lu",
            &user->rx, &user->tx)!= 2) {
        DBG("can't read %s statistics\n", c->iface);
        return -1;
    }
    return 0;
}

#elif defined(__FreeBSD__)
static void
init_user_stats(user_priv *c)
{
    int mib[6] = {
        CTL_NET,
        PF_LINK,
        NETLINK_GENERIC,
        IFMIB_SYSTEM,
        IFMIB_IFCOUNT
    };
    u_int count = 0;
    struct ifmibdata ifmd;
    size_t len = sizeof(count);

    c->ifmib_row = 0;
    if (sysctl(mib, 5, (void *)&count, &len, NULL, 0) != 0)
        return;

    mib[3] = IFMIB_IFDATA;
    mib[5] = IFDATA_GENERAL;
    len = sizeof(ifmd);
    for (mib[4] = 1; mib[4] <= count; mib[4]++) {
        if (sysctl(mib, 6, (void *)&ifmd, &len, NULL, 0) != 0)
            continue;
        if (strcmp(ifmd.ifmd_name, c->iface) == 0) {
            c->ifmib_row = mib[4];
            break;
        }
    }
}

static int
user_get_load_real(user_priv *c, struct user_stat *user)
{
    int mib[6] = {
        CTL_NET,
        PF_LINK,
        NETLINK_GENERIC,
        IFMIB_IFDATA,
        c->ifmib_row,
        IFDATA_GENERAL
    };
    struct ifmibdata ifmd;
    size_t len = sizeof(ifmd);

    if (!c->ifmib_row)
        return -1;

    if (sysctl(mib, sizeof(mib)/sizeof(mib[0]), &ifmd, &len, NULL, 0) != 0)
        return -1;

    user->tx = ifmd.ifmd_data.ifi_obytes;
    user->rx = ifmd.ifmd_data.ifi_ibytes;
    return 0;
}

#endif

static int
user_get_load(user_priv *c)
{
    struct user_stat user, user_diff;
    float total[2];
    char buf[256];

    ENTER;
    memset(&user, 0, sizeof(user));
    memset(&user_diff, 0, sizeof(user_diff));
    memset(&total, 0, sizeof(total));

    if (user_get_load_real(c, &user))
        goto end;

    user_diff.tx = ((user.tx - c->user_prev.tx) >> 10) / CHECK_PERIOD;
    user_diff.rx = ((user.rx - c->user_prev.rx) >> 10) / CHECK_PERIOD;

    c->user_prev = user;
    total[0] = (float)(user_diff.tx) / c->max;
    total[1] = (float)(user_diff.rx) / c->max;

end:
    DBG("%f %f %ul %ul\n", total[0], total[1], user_diff.tx, user_diff.rx);
    k->add_tick(&c->chart, total);
    g_snprintf(buf, sizeof(buf), "<b>%s:</b>\nD %lu Kbs, U %lu Kbs",
        c->iface, user_diff.rx, user_diff.tx);
    gtk_widget_set_tooltip_markup(((plugin_instance *)c)->pwid, buf);
    RET(TRUE);
}

static int
user_constructor(plugin_instance *p)
{
    user_priv *c;

    if (!(k = class_get("chart")))
        RET(0);
    if (!PLUGIN_CLASS(k)->constructor(p))
        RET(0);
    c = (user_priv *) p;

    c->iface = "eth0";
    c->max_rx = 120;
    c->max_tx = 12;
    c->colors[0] = "violet";
    c->colors[1] = "blue";
    XCG(p->xc, "interface", &c->iface, str);
    XCG(p->xc, "RxLimit", &c->max_rx, int);
    XCG(p->xc, "TxLimit", &c->max_tx, int);
    XCG(p->xc, "TxColor", &c->colors[0], str);
    XCG(p->xc, "RxColor", &c->colors[1], str);

    init_user_stats(c);

    c->max = c->max_rx + c->max_tx;
    k->set_rows(&c->chart, 2, c->colors);
    gtk_widget_set_tooltip_markup(((plugin_instance *)c)->pwid, "<b>User</b>");
    user_get_load(c);
    c->timer = g_timeout_add(CHECK_PERIOD * 1000,
        (GSourceFunc) user_get_load, (gpointer) c);
    RET(1);
}


static void
user_destructor(plugin_instance *p)
{
    user_priv *c = (user_priv *) p;

    ENTER;
    if (c->timer)
        g_source_remove(c->timer);
    PLUGIN_CLASS(k)->destructor(p);
    class_put("chart");
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
