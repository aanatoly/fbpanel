// Time-stamp: < power_supply.h (2015-12-04 19:01) >

#ifndef POWER_SUPPLY_H
#define POWER_SUPPLY_H

#include <glib-2.0/glib.h>

/* Struct representing AC power supply. */
typedef struct {
    /* Path to uevent file. */
    gchar* path;
    gchar* name;
    gboolean online;
} ac;

/* Struct representing BATTERY power supply. */
typedef struct {
    /* Path to uevent file. */
    gchar* path;
    gchar* name;
    gchar* status;
    /* In percent 0.0--100.0. */
    gdouble capacity;
} bat;

/* Struct representing all the power supplies on current system. */
typedef struct {
    /* List of ac structs. */
    GSequence* ac_list;
    /* List of bat structs. */
    GSequence* bat_list;
} power_supply;

/* Allocate memory for struct power_supply. */
power_supply* power_supply_new();

/* Free memory allocated by power_supply_new(). */
void power_supply_free(gpointer p);

/* Parses power supplies on the current system. */
power_supply* power_supply_parse();

/*
 * Return TRUE if AC power is on (ie. at least one AC adapter is on).
 */
gboolean power_supply_is_ac_online(power_supply* ps);

/*
 * Return total BATTERY capacity (as percentage of capacity) on the
 * system as an average of all batteries capacity div by number of
 * batteries.
 */
gdouble power_supply_get_bat_capacity(power_supply* ps);

#endif /* POWER_SUPPLY_H */
