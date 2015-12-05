#ifndef _RUN_H_
#define _RUN_H_

#include <gtk/gtk.h>

void run_app(gchar *cmd);
GPid run_app_argv(gchar **argv);

#endif
