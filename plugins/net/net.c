
/*
 * A little bug fixed by Mykola <mykola@2ka.mipt.ru>:)
 * FreeBSD support is added by Eygene Ryabinkin <rea-fbsd@codelabs.ru>
 */

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

struct net_stat {
    gulong tx, rx;
};

typedef struct {
    chart_priv chart;
    struct net_stat net_prev;
    int timer;
    char *iface;
#if defined(__FreeBSD__)
    size_t ifmib_row;
#endif
    gint max_tx;
    gint max_rx;
    gulong max;
    gchar *colors[2];
} net_priv;

static chart_class *k;


static void net_destructor(plugin_instance *p);


#if defined __linux__

#define init_net_stats(x)

static int
net_get_load_real(net_priv *c, struct net_stat *net)
{
    FILE *stat;
    char buf[256], *s = NULL;

    stat = fopen("/proc/net/dev", "r");
    if(!stat)
        return -1;
    fgets(buf, 256, stat);
    fgets(buf, 256, stat);

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
            &net->rx, &net->tx)!= 2) {
        DBG("can't read %s statistics\n", c->iface);
        return -1;
    }
    return 0;
}

#elif defined(__FreeBSD__)
static void
init_net_stats(net_priv *c)
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
net_get_load_real(net_priv *c, struct net_stat *net)
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

    net->tx = ifmd.ifmd_data.ifi_obytes;
    net->rx = ifmd.ifmd_data.ifi_ibytes;
    return 0;
}

#endif

static int
net_get_load(net_priv *c)
{
    struct net_stat net, net_diff;
    float total[2];
    char buf[256];

    ENTER;
    memset(&net, 0, sizeof(net));
    memset(&net_diff, 0, sizeof(net_diff));
    memset(&total, 0, sizeof(total));

    if (net_get_load_real(c, &net))
        goto end;

    net_diff.tx = ((net.tx - c->net_prev.tx) >> 10) / CHECK_PERIOD;
    net_diff.rx = ((net.rx - c->net_prev.rx) >> 10) / CHECK_PERIOD;

    c->net_prev = net;
    total[0] = (float)(net_diff.tx) / c->max;
    total[1] = (float)(net_diff.rx) / c->max;

end:
    DBG("%f %f %ul %ul\n", total[0], total[1], net_diff.tx, net_diff.rx);
    k->add_tick(&c->chart, total);
    g_snprintf(buf, sizeof(buf), "<b>%s:</b>\nD %lu Kbs, U %lu Kbs",
        c->iface, net_diff.rx, net_diff.tx);
    gtk_widget_set_tooltip_markup(((plugin_instance *)c)->pwid, buf);
    RET(TRUE);
}

static int
net_constructor(plugin_instance *p)
{
    net_priv *c;

    if (!(k = class_get("chart")))
        RET(0);
    if (!PLUGIN_CLASS(k)->constructor(p))
        RET(0);
    c = (net_priv *) p;

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

    init_net_stats(c);

    c->max = c->max_rx + c->max_tx;
    k->set_rows(&c->chart, 2, c->colors);
    gtk_widget_set_tooltip_markup(((plugin_instance *)c)->pwid, "<b>Net</b>");
    net_get_load(c);
    c->timer = g_timeout_add(CHECK_PERIOD * 1000,
        (GSourceFunc) net_get_load, (gpointer) c);
    RET(1);
}


static void
net_destructor(plugin_instance *p)
{
    net_priv *c = (net_priv *) p;

    ENTER;
    if (c->timer)
        g_source_remove(c->timer);
    PLUGIN_CLASS(k)->destructor(p);
    class_put("chart");
    RET();
}


static plugin_class class = {
    .count       = 0,
    .type        = "net",
    .name        = "Net usage",
    .version     = "1.0",
    .description = "Display net usage",
    .priv_size   = sizeof(net_priv),

    .constructor = net_constructor,
    .destructor  = net_destructor,
};
static plugin_class *class_ptr = (plugin_class *) &class;
