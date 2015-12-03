#include <stdlib.h>
#include <strings.h>
#include <stdio.h>
#include <ctype.h>

#include "xconf.h"
#include "panel.h"

//#define DEBUGPRN
#include "dbg.h"


enum { LINE_NONE, LINE_BLOCK_START, LINE_BLOCK_END, LINE_VAR };

#define LINE_LENGTH 256
typedef struct {
    int type;
    gchar str[LINE_LENGTH];
    gchar *t[2];
} line;


/* Creates new xconf node */
xconf *xconf_new(gchar *name, gchar *value)
{
    xconf *x;

    x = g_new0(xconf, 1);
    x->name = g_strdup(name);
    x->value = g_strdup(value);
    return x;
}

/* Append @src's sons to @dst node */
void xconf_append_sons(xconf *dst, xconf *src)
{
    GSList *e;
    xconf *tmp;
    
    if (!dst || !src)
        return;
    for (e = src->sons; e; e = g_slist_next(e))
    {
        tmp = e->data;
        tmp->parent = dst;
    }
    dst->sons = g_slist_concat(dst->sons, src->sons);
    src->sons = NULL;
}

/* Adss new son node */
void xconf_append(xconf *parent, xconf *son)
{
    if (!parent || !son)
        return;
    son->parent = parent;
    /* appending requires traversing all list to the end, which is not
     * efficient, but for v 1.0 it's ok*/
    parent->sons = g_slist_append(parent->sons, son);
}

/* Unlinks node from its parent, if any */
void xconf_unlink(xconf *x)
{
    if (x && x->parent)
    {
        x->parent->sons = g_slist_remove(x->parent->sons, x);
        x->parent = NULL;
    }
}

/* Unlinks node and deletes it and its sub-tree */
void xconf_del(xconf *x, gboolean sons_only)
{
    GSList *s;
    xconf *x2;
    
    if (!x)
        return;
    DBG("%s %s\n", x->name, x->value);
    for (s = x->sons; s; s = g_slist_delete_link(s, s))
    {
        x2 = s->data;
        x2->parent = NULL;
        xconf_del(x2, FALSE);
    }
    x->sons = NULL;
    if (!sons_only)
    {
        g_free(x->name);
        g_free(x->value);
        xconf_unlink(x);
        g_free(x);
    }
}

void xconf_set_value(xconf *x, gchar *value)
{
    xconf_del(x, TRUE);
    g_free(x->value);
    x->value = g_strdup(value);
  
}

void xconf_set_value_ref(xconf *x, gchar *value)
{
    xconf_del(x, TRUE);
    g_free(x->value);
    x->value = value;
  
}

void xconf_set_int(xconf *x, int i)
{
    xconf_del(x, TRUE);
    g_free(x->value);
    x->value = g_strdup_printf("%d", i);
}

xconf *
xconf_get(xconf *xc, gchar *name)
{
    xconf *ret;

    if (!xc)
        return NULL;
    if ((ret = xconf_find(xc, name, 0)))
        return ret;
    ret = xconf_new(name, NULL);
    xconf_append(xc, ret);
    return ret;
}

gchar *xconf_get_value(xconf *x)
{
    return x->value;
}

void xconf_prn(FILE *fp, xconf *x, int n, gboolean sons_only)
{
    int i;
    GSList *s;
    xconf *x2;

    if (!sons_only)
    {
        for (i = 0; i < n; i++)
            fprintf(fp, "    ");
        fprintf(fp, "%s", x->name);
        if (x->value)
            fprintf(fp, " = %s\n", x->value);
        else
            fprintf(fp, " {\n");
        n++;
    }
    for (s = x->sons; s; s = g_slist_next(s))
    {
        x2 = s->data;
        xconf_prn(fp, x2, n, FALSE);
    }
    if (!sons_only && !x->value)
    {
        n--;
        for (i = 0; i < n; i++)
            fprintf(fp, "    ");
        fprintf(fp, "}\n");
    }

}

xconf *xconf_find(xconf *x, gchar *name, int no)
{
    GSList *s;
    xconf *x2;

    if (!x)
        return NULL;
    for (s = x->sons; s; s = g_slist_next(s))
    {
        x2 = s->data;
        if (!strcasecmp(x2->name, name))
        {
            if (!no)
                return x2;
            no--;
        }
    }
    return NULL;
}


void xconf_get_str(xconf *x, gchar **val)
{
    if (x && x->value)
        *val = x->value;
}


void xconf_get_int(xconf *x, int *val)
{
    gchar *s;

    if (!x)
        return;
    s = xconf_get_value(x);
    if (!s)
        return;
    *val = strtol(s, NULL, 0);
}

void xconf_get_enum(xconf *x, int *val, xconf_enum *p)
{
    gchar *s;

    if (!x)
        return;
    s = xconf_get_value(x);
    if (!s)
        return;
    while (p && p->str)
    {
        DBG("cmp %s %s\n", p->str, s);
        if (!strcasecmp(p->str, s))
        {
            *val = p->num;
            return;
        }
        p++;
    }
}

void
xconf_set_enum(xconf *x, int val, xconf_enum *p)
{
    if (!x)
        return;

    while (p && p->str)
    {
        if (val == p->num)
        {
            xconf_set_value(x, p->str);
            return;
        }
        p++;
    }
}

static int
read_line(FILE *fp, line *s)
{
    gchar *tmp, *tmp2;

    ENTER;
    s->type = LINE_NONE;
    if (!fp)
        RET(s->type);
    while (fgets(s->str, LINE_LENGTH, fp)) {
        g_strstrip(s->str);

        if (s->str[0] == '#' || s->str[0] == 0) {
            continue;
        }
        DBG( ">> %s\n", s->str);
        if (!g_ascii_strcasecmp(s->str, "}")) {
            s->type = LINE_BLOCK_END;
            break;
        }

        s->t[0] = s->str;
        for (tmp = s->str; isalnum(*tmp); tmp++);
        for (tmp2 = tmp; isspace(*tmp2); tmp2++);
        if (*tmp2 == '=') {
            for (++tmp2; isspace(*tmp2); tmp2++);
            s->t[1] = tmp2;
            *tmp = 0;
            s->type = LINE_VAR;
        } else if  (*tmp2 == '{') {
            *tmp = 0;
            s->type = LINE_BLOCK_START;
        } else {
            ERR( "parser: unknown token: '%c'\n", *tmp2);
        }
        break;
    }
    RET(s->type);

}


static xconf *
read_block(FILE *fp, gchar *name)
{
    line s;
    xconf *x, *xs;

    x = xconf_new(name, NULL);
    while (read_line(fp, &s) != LINE_NONE)
    {
        if (s.type == LINE_BLOCK_START)
        {
            xs = read_block(fp, s.t[0]);
            xconf_append(x, xs);
        }
        else if (s.type == LINE_BLOCK_END)
            break;
        else if (s.type == LINE_VAR)
        {
            xs = xconf_new(s.t[0], s.t[1]);
            xconf_append(x, xs);
        }
        else
        {
            printf("syntax error\n");
            exit(1);
        }
    }
    return x;
}

xconf *xconf_new_from_file(gchar *fname, gchar *name)
{
    FILE *fp = fopen(fname, "r");
    xconf *ret = NULL;
    if (fp)
    {
        ret = read_block(fp, name);
        fclose(fp);
    }
    //xconf_prn(stdout, ret, 0, FALSE);
    return ret;
}

void xconf_save_to_file(gchar *fname, xconf *xc)
{
    FILE *fp = fopen(fname, "w");

    if (fp)
    {
        xconf_prn(fp, xc, 0, TRUE);
        fclose(fp);
    }
}

void
xconf_save_to_profile(xconf *xc)
{
    xconf_save_to_file(panel_get_profile_file(), xc);
}

xconf *xconf_dup(xconf *xc)
{
    xconf *ret, *son;
    GSList *s;

    if (!xc)
        return NULL;
    ret = xconf_new(xc->name, xc->value);
    for (s = xc->sons; s; s = g_slist_next(s))
    {
        son = s->data;
        xconf_append(ret, xconf_dup(son));
    }
    return ret;
}

gboolean
xconf_cmp(xconf *a, xconf *b)
{
    GSList *as, *bs;
    
    if (!(a || b))
        return FALSE;
    if (!(a && b))
        return TRUE;
    
    if (g_ascii_strcasecmp(a->name, b->name))
        return TRUE;
    
    if (g_strcmp0(a->value, b->value))
        return TRUE;
    for (as = a->sons, bs = b->sons; as && bs;
         as = g_slist_next(as), bs = g_slist_next(bs))
    {
        if (xconf_cmp(as->data, bs->data))
            return TRUE;
    }
    return (as != bs);
}
