// Time-stamp: < main.c (2015-12-04 19:02) >
// run with: make -k -f Makefile-test valgrind

#include <glib-2.0/glib.h>
#include <glib-2.0/glib/gprintf.h>

#include "power_supply.h"

int main(int argc, char** args)
{
    power_supply* ps = power_supply_new();
    power_supply_parse(ps);
    gboolean ac_online = power_supply_is_ac_online(ps);
    gdouble bat_capacity = power_supply_get_bat_capacity(ps);

    power_supply_free(ps);

    g_fprintf(stdout, "ac_online: %d\nbat_capacity: %f\n", ac_online, bat_capacity);

    return 0;
}
