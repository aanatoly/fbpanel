// Time-stamp: < power_supply.c (2015-12-04 19:01) >

#include <string.h>
#include <glib-2.0/glib.h>
#include <glib-2.0/glib/gprintf.h>

#include "power_supply.h"

#define DEBUG 0

#define STRING_LEN 100

#define SYS_ACPI_PATH "/sys/class/power_supply/"
#define SYS_ACPI_TYPE_FILE "type"
#define SYS_ACPI_TYPE_AC "Mains\n"
#define SYS_ACPI_TYPE_BAT "Battery\n"
#define SYS_ACPI_UEVENT_FILE "uevent"
#define SYS_ACPI_UEVENT_NAME_KEY "POWER_SUPPLY_NAME"
#define SYS_ACPI_UEVENT_AC_ONLINE_KEY "POWER_SUPPLY_ONLINE"
#define SYS_ACPI_UEVENT_AC_ONLINE_VALUE "1"
#define SYS_ACPI_UEVENT_BAT_STATUS_KEY "POWER_SUPPLY_STATUS"
/* Recent kernels. */
#define SYS_ACPI_UEVENT_BAT_CAPACITY_KEY "POWER_SUPPLY_CAPACITY"
/*
 * Older kernlels -- capacity is computed as division of:
 * POWER_SUPPLY_ENERGY_NOW / POWER_SUPPLY_ENERGY_FULL
 * or
 * POWER_SUPPLY_CHARGE_NOW / POWER_SUPPLY_CHARGE_FULL
 */
#define SYS_ACPI_UEVENT_BAT_ENERGY_FULL_KEY "POWER_SUPPLY_ENERGY_FULL"
#define SYS_ACPI_UEVENT_BAT_ENERGY_NOW_KEY "POWER_SUPPLY_ENERGY_NOW"
#define SYS_ACPI_UEVENT_BAT_CHARGE_FULL_KEY "POWER_SUPPLY_CHARGE_FULL"
#define SYS_ACPI_UEVENT_BAT_CHARGE_NOW_KEY "POWER_SUPPLY_CHARGE_NOW"

/* Debug function to print out uevent file key-value pairs. */
static void
uevent_ghfunc(gpointer key, gpointer value, gpointer user_data)
{
    gchar* k = (gchar*) key;
    gchar* v = (gchar*) value;
    g_fprintf(stderr, "'%s' => '%s'\n", k, v);
}

/* Parses uevent file returning the result as GHashTable. */
static GHashTable*
uevent_parse(gchar* filename)
{
    GHashTable* hash = NULL;
    GString* key = g_string_sized_new(STRING_LEN);
    GString* value = g_string_sized_new(STRING_LEN);
    gchar* buf = NULL;
    guint buf_len = 0;
    gchar c;
    guint i;
    gboolean equals_sign_found = FALSE;

    if (g_file_test(filename, G_FILE_TEST_IS_REGULAR)) {
        if (g_file_get_contents(filename, &buf, 0, NULL)) {
            hash = g_hash_table_new_full(&g_str_hash, &g_str_equal, &g_free, &g_free);
            buf_len = strlen(buf);

            for (i = 0; i < buf_len; ++i) {
                c = buf[i];
                if (c == '=' && !equals_sign_found) {
                    equals_sign_found = TRUE;
                } else if (c == '\n' && equals_sign_found) {
                    equals_sign_found = FALSE;
                    g_hash_table_insert(hash, g_strdup(key->str), g_strdup(value->str));
                    g_string_truncate(key, 0);
                    g_string_truncate(value, 0);
                } else {
                    if (equals_sign_found) {
                        g_string_append_c(value, c);
                    } else {
                        g_string_append_c(key, c);
                    }
                }
            }
            if (DEBUG) {
                g_hash_table_foreach(hash, &uevent_ghfunc, NULL);
            }
        }
    }

    g_free(buf);
    g_string_free(key, TRUE);
    g_string_free(value, TRUE);

    return hash;
}

static ac*
ac_new(gchar* path)
{
    ac* tmp = g_new(ac, 1);
    tmp->path = path;
    tmp->name = NULL;
    tmp->online = FALSE;
    return tmp;
}

static void
ac_free(gpointer p)
{
    ac* tmp = (ac*) p;
    g_free(tmp->path);
    g_free(tmp->name);
    g_free(tmp);
    if (DEBUG) {
        g_fprintf(stderr, "ac_free %p\n", p);
    }
}

static void
ac_print(gpointer p, gpointer user_data)
{
    ac* tmp = (ac*) p;
    g_fprintf(stderr, "AC\n  path: %s\n  name: %s\n online: %d\n", tmp->path, tmp->name, tmp->online);
}

/* Parses information about AC power supply based on ac->path. */
static ac*
ac_parse(ac* ac)
{
    GHashTable* hash;
    gchar* tmp_value;

    if (ac->path != NULL) {
        hash = uevent_parse(ac->path);
        if (hash != NULL) {
            // ac name
            tmp_value = (gchar*) g_hash_table_lookup(hash, SYS_ACPI_UEVENT_NAME_KEY);
            if (tmp_value != NULL) {
                ac->name = g_strdup(tmp_value);
            }

            // ac online
            tmp_value = (gchar*) g_hash_table_lookup(hash, SYS_ACPI_UEVENT_AC_ONLINE_KEY);
            if (tmp_value != NULL) {
                ac->online = strcmp(SYS_ACPI_UEVENT_AC_ONLINE_VALUE, tmp_value) == 0;
            }

            g_hash_table_destroy(hash);
        }
    }

    return ac;
}

static bat*
bat_new(gchar* path)
{
    bat* tmp = g_new(bat, 1);
    tmp->path = path;
    tmp->name = NULL;
    tmp->status = NULL;
    tmp->capacity = -1;
    return tmp;
}

static void
bat_free(gpointer p)
{
    bat* tmp = (bat*) p;
    g_free(tmp->path);
    g_free(tmp->name);
    g_free(tmp->status);
    g_free(tmp);
    if (DEBUG) {
        g_fprintf(stderr, "bat_free %p\n", p);
    }
}

static void
bat_print(gpointer p, gpointer user_data)
{
    bat* tmp = (bat*) p;
    g_fprintf(stderr, "BATTERY\n  path: %s\n  name: %s\n  status: %s\n  capacity: %f\n", tmp->path, tmp->name, tmp->status, tmp->capacity);
}

/* Parses information about BATTERY power supply based on bat->path. */
static bat*
bat_parse(bat* bat)
{
    GHashTable* hash;
    gchar* tmp_value;

    if (bat->path != NULL) {
        hash = uevent_parse(bat->path);
        if (hash != NULL) {
            // battery name
            tmp_value = (gchar*) g_hash_table_lookup(hash, SYS_ACPI_UEVENT_NAME_KEY);
            if (tmp_value != NULL) {
                bat->name = g_strdup(tmp_value);
            }

            // battery status
            tmp_value = (gchar*) g_hash_table_lookup(hash, SYS_ACPI_UEVENT_BAT_STATUS_KEY);
            if (tmp_value != NULL) {
                bat->status = g_strdup(tmp_value);
            }

            // battery remaining capacity
            tmp_value = (gchar*) g_hash_table_lookup(hash, SYS_ACPI_UEVENT_BAT_CAPACITY_KEY);
            if (tmp_value != NULL) {
                bat->capacity = g_ascii_strtod(tmp_value, NULL);
            } else { // for older kernels
                tmp_value = (gchar*) g_hash_table_lookup(hash, SYS_ACPI_UEVENT_BAT_ENERGY_NOW_KEY);
                gdouble tmp = -1;
                if (tmp_value != NULL) { // ac off
                    tmp = g_ascii_strtod(tmp_value, NULL);
                    tmp_value = (gchar*) g_hash_table_lookup(hash, SYS_ACPI_UEVENT_BAT_ENERGY_FULL_KEY);
                    if (tmp_value != NULL && tmp > 0) {
                        tmp = tmp / g_ascii_strtod(tmp_value, NULL) * 100;
                        bat->capacity = tmp;
                    }
                } else {
                    tmp_value = (gchar*) g_hash_table_lookup(hash, SYS_ACPI_UEVENT_BAT_CHARGE_NOW_KEY);
                    if (tmp_value != NULL) { // ac on
                        tmp = g_ascii_strtod(tmp_value, NULL);
                        tmp_value = (gchar*) g_hash_table_lookup(hash, SYS_ACPI_UEVENT_BAT_CHARGE_FULL_KEY);
                        if (tmp_value != NULL && tmp > 0) {
                            tmp = tmp / g_ascii_strtod(tmp_value, NULL) * 100;
                            bat->capacity = tmp;
                        }
                    }
                }
            }

            g_hash_table_destroy(hash);
        }
    }

    return bat;
}

extern power_supply*
power_supply_new() {
    power_supply* tmp = g_new(power_supply, 1);
    tmp->ac_list = g_sequence_new(&ac_free);
    tmp->bat_list = g_sequence_new(&bat_free);
    return tmp;
}

extern void
power_supply_free(gpointer p) {
    power_supply* tmp = (power_supply*) p;
    g_sequence_free(tmp->ac_list);
    g_sequence_free(tmp->bat_list);
    g_free(tmp);
    if (DEBUG) {
        g_fprintf(stderr, "power_supply_free %p\n", p);
    }
}

/* Parses power supplies on the current system. */
extern power_supply*
power_supply_parse(power_supply* ps) {
    GDir* dir = NULL;
    const gchar* tmp;
    GString* filename = g_string_sized_new(STRING_LEN);
    guint len = 0;
    gchar* contents;

    if (g_file_test(SYS_ACPI_PATH, G_FILE_TEST_IS_DIR)) {
        dir = g_dir_open(SYS_ACPI_PATH, 0, NULL);
        if (dir != NULL) {
            while ((tmp = g_dir_read_name(dir)) != NULL) {
                g_string_append(filename, SYS_ACPI_PATH);
                g_string_append(filename, tmp);
                g_string_append_c(filename, G_DIR_SEPARATOR);
                len = filename->len;
                g_string_append(filename, SYS_ACPI_TYPE_FILE);
                if (g_file_test(filename->str, G_FILE_TEST_IS_REGULAR)) {
                    g_file_get_contents(filename->str, &contents, 0, NULL);
                    g_string_truncate(filename, len);
                    g_string_append(filename, SYS_ACPI_UEVENT_FILE);
                    if (strcmp(SYS_ACPI_TYPE_AC, contents) == 0) {
                        ac* tmp = ac_new(g_strdup(filename->str));
                        ac_parse(tmp);
                        g_sequence_append(ps->ac_list, tmp);
                    } else if (strcmp(SYS_ACPI_TYPE_BAT, contents) == 0) {
                        bat* tmp = bat_new(g_strdup(filename->str));
                        bat_parse(tmp);
                        g_sequence_append(ps->bat_list, tmp);
                    } else {
                        g_fprintf(stderr, "unsupported power supply type %s", contents);
                    }
                    g_free(contents);
                }
                g_string_truncate(filename, 0);
            }
            g_dir_close(dir);
        }
    }

    g_string_free(filename, TRUE);

    if (DEBUG) {
        g_sequence_foreach(ps->ac_list, &ac_print, NULL);
        g_sequence_foreach(ps->bat_list, &bat_print, NULL);
    }

    return ps;
}

extern gboolean
power_supply_is_ac_online(power_supply* ps)
{
    gboolean ac_online = FALSE;
    GSequenceIter* it;
    ac* ac_power;
    if (ps->ac_list != NULL) {
        it = g_sequence_get_begin_iter(ps->ac_list);
        while (!g_sequence_iter_is_end(it)) {
            ac_power = (ac*) g_sequence_get(it);
            if (ac_power->online) {
                ac_online = TRUE;
                break;
            }
            it = g_sequence_iter_next(it);
        }
    }
    return ac_online;
}

extern gdouble
power_supply_get_bat_capacity(power_supply* ps)
{
    gdouble total_bat_capacity = 0;
    guint bat_count = 0;
    GSequenceIter* it;
    bat* battery;
    if (ps->bat_list != NULL) {
        it = g_sequence_get_begin_iter(ps->bat_list);
        while (!g_sequence_iter_is_end(it)) {
            battery = (bat*) g_sequence_get(it);
            if (battery->capacity > 0) {
                total_bat_capacity = total_bat_capacity + battery->capacity;
            }
            bat_count++;
            it = g_sequence_iter_next(it);
        }
    }
    return total_bat_capacity / bat_count;
}
