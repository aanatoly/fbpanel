#include "run.h"
#include "dbg.h"

void
run_app(gchar *cmd)
{
    GError *error = NULL;

    ENTER;
    if (!cmd)
        RET();

    if (!g_spawn_command_line_async(cmd, &error))
    {
        GtkWidget *dialog = gtk_message_dialog_new(NULL, 0,
            GTK_MESSAGE_ERROR,
            GTK_BUTTONS_CLOSE,
            "%s", error->message);
        gtk_dialog_run(GTK_DIALOG(dialog));
        gtk_widget_destroy(dialog);
        g_error_free(error);
    }
    RET();
}


GPid
run_app_argv(gchar **argv)
{
    GError *error = NULL;
    GSpawnFlags flags = G_SPAWN_DO_NOT_REAP_CHILD | G_SPAWN_SEARCH_PATH;
    GPid pid;

    ENTER;
    flags |= G_SPAWN_STDOUT_TO_DEV_NULL;
    if (!g_spawn_async(NULL, argv, NULL, flags, NULL, NULL, &pid, &error)) {
        GtkWidget *dialog = gtk_message_dialog_new(NULL, 0,
            GTK_MESSAGE_ERROR,
            GTK_BUTTONS_CLOSE,
            "%s", error->message);
        gtk_dialog_run(GTK_DIALOG(dialog));
        gtk_widget_destroy(dialog);
        g_error_free(error);
    }

    RET(pid);
}
