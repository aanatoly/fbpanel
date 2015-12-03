

#include "gconf.h"
#include "panel.h"

//#define DEBUGPRN
#include "dbg.h"

enum
{
    TYPE_COL,
    NAME_COL,
    N_COLUMNS
};

GtkTreeStore *store;
GtkWidget *tree;
GtkWidget *bbox;

static void
mk_model(xconf *xc)
{
    GtkTreeIter iter;
    xconf *pxc;
    int i;
    gchar *type;
    
    store = gtk_tree_store_new(N_COLUMNS, G_TYPE_STRING, G_TYPE_STRING);
    for (i = 0; (pxc = xconf_find(xc, "plugin", i)); i++)
    {
        XCG(pxc, "type", &type, str);
        gtk_tree_store_append(store, &iter, NULL); 
        gtk_tree_store_set (store, &iter,
            TYPE_COL, type,
            NAME_COL, "Martin Heidegger",
            -1);
    }
}

static void
tree_selection_changed_cb(GtkTreeSelection *selection, gpointer data)
{
    GtkTreeIter iter;
    GtkTreeModel *model;
    gchar *type;
    gboolean sel;

    sel = gtk_tree_selection_get_selected(selection, &model, &iter);
    if (sel)
    {
        gtk_tree_model_get(model, &iter, TYPE_COL, &type, -1);
        g_print("%s\n", type);
        g_free(type);
    }
    gtk_widget_set_sensitive(bbox, sel);
}

static GtkWidget *
mk_view()
{
    GtkCellRenderer *renderer;
    GtkTreeViewColumn *column;
    GtkTreeSelection *select;
    
    tree = gtk_tree_view_new_with_model(GTK_TREE_MODEL(store));
    renderer = gtk_cell_renderer_text_new();
    column = gtk_tree_view_column_new_with_attributes("Type",
        renderer, "text", TYPE_COL, NULL);
    gtk_tree_view_append_column(GTK_TREE_VIEW(tree), column);

    select = gtk_tree_view_get_selection(GTK_TREE_VIEW(tree));
    gtk_tree_selection_set_mode(select, GTK_SELECTION_SINGLE);
    g_signal_connect(G_OBJECT(select), "changed",
        G_CALLBACK(tree_selection_changed_cb), NULL);
    return tree;
}

GtkWidget *
mk_buttons()
{
    GtkWidget *bm, *b, *w;

    bm = gtk_hbox_new(FALSE, 3);
    
    w = gtk_button_new_from_stock(GTK_STOCK_ADD);
    gtk_box_pack_start(GTK_BOX(bm), w, FALSE, TRUE, 0);

    b = gtk_hbox_new(FALSE, 3);
    gtk_box_pack_start(GTK_BOX(bm), b, FALSE, TRUE, 0);
    bbox = b;
    gtk_widget_set_sensitive(bbox, FALSE);
    
    w = gtk_button_new_from_stock(GTK_STOCK_EDIT);
    gtk_box_pack_start(GTK_BOX(b), w, FALSE, TRUE, 0);
    w = gtk_button_new_from_stock(GTK_STOCK_DELETE);
    gtk_box_pack_start(GTK_BOX(b), w, FALSE, TRUE, 0);
    w = gtk_button_new_from_stock(GTK_STOCK_GO_DOWN);
    gtk_box_pack_start(GTK_BOX(b), w, FALSE, TRUE, 0);
    w = gtk_button_new_from_stock(GTK_STOCK_GO_UP);
    gtk_box_pack_start(GTK_BOX(b), w, FALSE, TRUE, 0);

    return bm;
}

GtkWidget *
mk_tab_plugins(xconf *xc)
{
    GtkWidget *page, *w;
    
    ENTER;
    page = gtk_vbox_new(FALSE, 1);
    gtk_container_set_border_width(GTK_CONTAINER(page), 10);

    mk_model(xc);
    
    w = mk_view();
    gtk_box_pack_start(GTK_BOX(page), w, TRUE, TRUE, 0);
    w = mk_buttons();
    gtk_box_pack_start(GTK_BOX(page), w, FALSE, TRUE, 0);
    
    gtk_widget_show_all(page);    
    RET(page);
}
