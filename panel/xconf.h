#ifndef _XCONF_H_
#define _XCONF_H_

#include <glib.h>
#include <stdio.h>

typedef struct _xconf
{
    gchar *name;
    gchar *value;
    GSList *sons;
    struct _xconf *parent;
} xconf;

typedef struct {
    gchar *str;
    gchar *desc;
    int num;
} xconf_enum;

xconf *xconf_new(gchar *name, gchar *value);
void xconf_append(xconf *parent, xconf *son);
void xconf_append_sons(xconf *parent, xconf *son);
void xconf_unlink(xconf *x);
void xconf_del(xconf *x, gboolean sons_only);
void xconf_set_value(xconf *x, gchar *value);
void xconf_set_value_ref(xconf *x, gchar *value);
gchar *xconf_get_value(xconf *x);
void xconf_prn(FILE *fp, xconf *x, int n, gboolean sons_only);
xconf *xconf_find(xconf *x, gchar *name, int no);
xconf *xconf_dup(xconf *xc);
gboolean xconf_cmp(xconf *a, xconf *b);
xconf *xconf_new_from_file(gchar *fname, gchar *name);
void xconf_save_to_file(gchar *fname, xconf *xc);
void xconf_save_to_profile(xconf *xc);

xconf *xconf_get(xconf *x, gchar *name);
void xconf_get_int(xconf *x, int *val);
void xconf_get_enum(xconf *x, int *val, xconf_enum *e);
void xconf_get_str(xconf *x, gchar **val);

void xconf_set_int(xconf *x, int val);
void xconf_set_enum(xconf *x, int val, xconf_enum *e);

#define XCG(xc, name, var, type, extra...)                      \
    xconf_get_ ## type(xconf_find(xc, name, 0), var, ## extra)


#endif
