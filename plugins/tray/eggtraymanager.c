/* eggtraymanager.c
 * Copyright (C) 2002 Anders Carlsson <andersca@gnu.org>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 */

#include <string.h>
#include <gdk/gdkx.h>
#include <gtk/gtkinvisible.h>
#include <gtk/gtksocket.h>
#include <gtk/gtkwindow.h>
#include "eggtraymanager.h"
#include "eggmarshalers.h"

//#define DEBUGPRN
#include "dbg.h"
/* Signals */
enum
{
  TRAY_ICON_ADDED,
  TRAY_ICON_REMOVED,
  MESSAGE_SENT,
  MESSAGE_CANCELLED,
  LOST_SELECTION,
  LAST_SIGNAL
};

typedef struct
{
  long id, len;
  long remaining_len;
  
  long timeout;
  Window window;
  char *str;
} PendingMessage;

static GObjectClass *parent_class = NULL;
static guint manager_signals[LAST_SIGNAL] = { 0 };

#define SYSTEM_TRAY_REQUEST_DOCK    0
#define SYSTEM_TRAY_BEGIN_MESSAGE   1
#define SYSTEM_TRAY_CANCEL_MESSAGE  2

static gboolean egg_tray_manager_check_running_xscreen (Screen *xscreen);

static void egg_tray_manager_init (EggTrayManager *manager);
static void egg_tray_manager_class_init (EggTrayManagerClass *klass);

static void egg_tray_manager_finalize (GObject *object);

static void egg_tray_manager_unmanage (EggTrayManager *manager);

GType
egg_tray_manager_get_type (void)
{
  static GType our_type = 0;

  if (our_type == 0)
    {
      static const GTypeInfo our_info =
      {
	sizeof (EggTrayManagerClass),
	(GBaseInitFunc) NULL,
	(GBaseFinalizeFunc) NULL,
	(GClassInitFunc) egg_tray_manager_class_init,
	NULL, /* class_finalize */
	NULL, /* class_data */
	sizeof (EggTrayManager),
	0,    /* n_preallocs */
	(GInstanceInitFunc) egg_tray_manager_init
      };

      our_type = g_type_register_static (G_TYPE_OBJECT, "EggTrayManager", &our_info, 0);
    }

  return our_type;

}

static void
egg_tray_manager_init (EggTrayManager *manager)
{
  manager->socket_table = g_hash_table_new (NULL, NULL);
}

static void
egg_tray_manager_class_init (EggTrayManagerClass *klass)
{
    GObjectClass *gobject_class;
  
    parent_class = g_type_class_peek_parent (klass);
    gobject_class = (GObjectClass *)klass;

    gobject_class->finalize = egg_tray_manager_finalize;
  
    manager_signals[TRAY_ICON_ADDED] =
        g_signal_new ("tray_icon_added",
              G_OBJECT_CLASS_TYPE (klass),
              G_SIGNAL_RUN_LAST,
              G_STRUCT_OFFSET (EggTrayManagerClass, tray_icon_added),
              NULL, NULL,
              g_cclosure_marshal_VOID__OBJECT,
              G_TYPE_NONE, 1,
              GTK_TYPE_SOCKET);

    manager_signals[TRAY_ICON_REMOVED] =
        g_signal_new ("tray_icon_removed",
              G_OBJECT_CLASS_TYPE (klass),
              G_SIGNAL_RUN_LAST,
              G_STRUCT_OFFSET (EggTrayManagerClass, tray_icon_removed),
              NULL, NULL,
              g_cclosure_marshal_VOID__OBJECT,
              G_TYPE_NONE, 1,
              GTK_TYPE_SOCKET);
    manager_signals[MESSAGE_SENT] =
        g_signal_new ("message_sent",
              G_OBJECT_CLASS_TYPE (klass),
              G_SIGNAL_RUN_LAST,
              G_STRUCT_OFFSET (EggTrayManagerClass, message_sent),
              NULL, NULL,
              _egg_marshal_VOID__OBJECT_STRING_LONG_LONG,
              G_TYPE_NONE, 4,
              GTK_TYPE_SOCKET,
              G_TYPE_STRING,
              G_TYPE_LONG,
              G_TYPE_LONG);
    manager_signals[MESSAGE_CANCELLED] =
        g_signal_new ("message_cancelled",
              G_OBJECT_CLASS_TYPE (klass),
              G_SIGNAL_RUN_LAST,
              G_STRUCT_OFFSET (EggTrayManagerClass, message_cancelled),
              NULL, NULL,
              _egg_marshal_VOID__OBJECT_LONG,
              G_TYPE_NONE, 2,
              GTK_TYPE_SOCKET,
              G_TYPE_LONG);
    manager_signals[LOST_SELECTION] =
        g_signal_new ("lost_selection",
              G_OBJECT_CLASS_TYPE (klass),
              G_SIGNAL_RUN_LAST,
              G_STRUCT_OFFSET (EggTrayManagerClass, lost_selection),
              NULL, NULL,
              g_cclosure_marshal_VOID__VOID,
              G_TYPE_NONE, 0);

}

static void
egg_tray_manager_finalize (GObject *object)
{
  EggTrayManager *manager;
  
  manager = EGG_TRAY_MANAGER (object);

  egg_tray_manager_unmanage (manager);
  
  G_OBJECT_CLASS (parent_class)->finalize (object);
}

EggTrayManager *
egg_tray_manager_new (void)
{
  EggTrayManager *manager;

  manager = g_object_new (EGG_TYPE_TRAY_MANAGER, NULL);

  return manager;
}

static gboolean
egg_tray_manager_plug_removed (GtkSocket       *socket,
    EggTrayManager  *manager)
{
    Window *window;

    ENTER;
    window = g_object_get_data (G_OBJECT (socket), "egg-tray-child-window");

    g_hash_table_remove (manager->socket_table, GINT_TO_POINTER (*window));
    g_object_set_data (G_OBJECT (socket), "egg-tray-child-window",
        NULL);
  
    g_signal_emit (manager, manager_signals[TRAY_ICON_REMOVED], 0, socket);

    /* This destroys the socket. */
    RET(FALSE);
}

static gboolean
egg_tray_manager_socket_exposed (GtkWidget      *widget,
      GdkEventExpose *event,
      gpointer        user_data)
{
    ENTER;
    gdk_window_clear_area (widget->window,
          event->area.x, event->area.y,
          event->area.width, event->area.height);
    RET(FALSE);
}



static void
egg_tray_manager_make_socket_transparent (GtkWidget *widget,
      gpointer   user_data)
{
    ENTER;
    if (GTK_WIDGET_NO_WINDOW (widget))
        RET();
    gdk_window_set_back_pixmap (widget->window, NULL, TRUE);
    RET();
}



static void
egg_tray_manager_socket_style_set (GtkWidget *widget,
      GtkStyle  *previous_style,
      gpointer   user_data)
{
    ENTER;
    if (widget->window == NULL)
        RET();
    egg_tray_manager_make_socket_transparent(widget, user_data);
    RET();
}

static void
egg_tray_manager_handle_dock_request(EggTrayManager *manager,
    XClientMessageEvent  *xevent)
{
    GtkWidget *socket;
    Window *window;

    ENTER;
    socket = gtk_socket_new ();
    gtk_widget_set_app_paintable (socket, TRUE);
    gtk_widget_set_double_buffered (socket, FALSE);
    gtk_widget_add_events (socket, GDK_EXPOSURE_MASK);
    
    g_signal_connect (socket, "realize",
          G_CALLBACK (egg_tray_manager_make_socket_transparent), NULL);
    g_signal_connect (socket, "expose_event",
          G_CALLBACK (egg_tray_manager_socket_exposed), NULL);
    g_signal_connect_after (socket, "style_set",
          G_CALLBACK (egg_tray_manager_socket_style_set), NULL);
    gtk_widget_show (socket);


    /* We need to set the child window here
     * so that the client can call _get functions
     * in the signal handler
     */
    window = g_new (Window, 1);
    *window = xevent->data.l[2];
    DBG("plug window %lx\n", *window);
    g_object_set_data_full (G_OBJECT (socket), "egg-tray-child-window",
        window, g_free);
    g_signal_emit(manager, manager_signals[TRAY_ICON_ADDED], 0,
        socket);
    /* Add the socket only if it's been attached */
    if (GTK_IS_WINDOW(gtk_widget_get_toplevel(GTK_WIDGET(socket)))) {
        GtkRequisition req;
        XWindowAttributes wa;
        
        DBG("socket has window. going on\n");
        gtk_socket_add_id(GTK_SOCKET (socket), xevent->data.l[2]);
        g_signal_connect(socket, "plug_removed",
              G_CALLBACK(egg_tray_manager_plug_removed), manager);

        gdk_error_trap_push();
        XGetWindowAttributes(GDK_DISPLAY(), *window, &wa);
        if (gdk_error_trap_pop()) {
            ERR("can't embed window %lx\n", xevent->data.l[2]);
            goto error;
        }
        g_hash_table_insert(manager->socket_table,
            GINT_TO_POINTER(xevent->data.l[2]), socket);
        req.width = req.height = 1;
        gtk_widget_size_request(socket, &req);
        RET();
    }
error:    
    DBG("socket has NO window. destroy it\n");
    g_signal_emit(manager, manager_signals[TRAY_ICON_REMOVED], 0,
        socket);
    gtk_widget_destroy(socket);
    RET();
}

static void
pending_message_free (PendingMessage *message)
{
  g_free (message->str);
  g_free (message);
}

static void
egg_tray_manager_handle_message_data (EggTrayManager       *manager,
				       XClientMessageEvent  *xevent)
{
  GList *p;
  int len;
  
  /* Try to see if we can find the
   * pending message in the list
   */
  for (p = manager->messages; p; p = p->next)
    {
      PendingMessage *msg = p->data;

      if (xevent->window == msg->window)
	{
	  /* Append the message */
	  len = MIN (msg->remaining_len, 20);

	  memcpy ((msg->str + msg->len - msg->remaining_len),
		  &xevent->data, len);
	  msg->remaining_len -= len;

	  if (msg->remaining_len == 0)
	    {
	      GtkSocket *socket;

	      socket = g_hash_table_lookup (manager->socket_table, GINT_TO_POINTER (msg->window));

	      if (socket)
		{
		  g_signal_emit (manager, manager_signals[MESSAGE_SENT], 0,
				 socket, msg->str, msg->id, msg->timeout);
		}
	      manager->messages = g_list_remove_link (manager->messages,
						      p);
	      
	      pending_message_free (msg);
	    }

	  return;
	}
    }
}

static void
egg_tray_manager_handle_begin_message (EggTrayManager       *manager,
				       XClientMessageEvent  *xevent)
{
  GList *p;
  PendingMessage *msg;

  /* Check if the same message is
   * already in the queue and remove it if so
   */
  for (p = manager->messages; p; p = p->next)
    {
      PendingMessage *msg = p->data;

      if (xevent->window == msg->window &&
	  xevent->data.l[4] == msg->id)
	{
	  /* Hmm, we found it, now remove it */
	  pending_message_free (msg);
	  manager->messages = g_list_remove_link (manager->messages, p);
	  break;
	}
    }

  /* Now add the new message to the queue */
  msg = g_new0 (PendingMessage, 1);
  msg->window = xevent->window;
  msg->timeout = xevent->data.l[2];
  msg->len = xevent->data.l[3];
  msg->id = xevent->data.l[4];
  msg->remaining_len = msg->len;
  msg->str = g_malloc (msg->len + 1);
  msg->str[msg->len] = '\0';
  manager->messages = g_list_prepend (manager->messages, msg);
}

static void
egg_tray_manager_handle_cancel_message (EggTrayManager       *manager,
					XClientMessageEvent  *xevent)
{
  GtkSocket *socket;
  
  socket = g_hash_table_lookup (manager->socket_table, GINT_TO_POINTER (xevent->window));
  
  if (socket)
    {
      g_signal_emit (manager, manager_signals[MESSAGE_CANCELLED], 0,
		     socket, xevent->data.l[2]);
    }
}

static GdkFilterReturn
egg_tray_manager_handle_event (EggTrayManager       *manager,
			       XClientMessageEvent  *xevent)
{
  switch (xevent->data.l[1])
    {
    case SYSTEM_TRAY_REQUEST_DOCK:
      egg_tray_manager_handle_dock_request (manager, xevent);
      return GDK_FILTER_REMOVE;

    case SYSTEM_TRAY_BEGIN_MESSAGE:
      egg_tray_manager_handle_begin_message (manager, xevent);
      return GDK_FILTER_REMOVE;

    case SYSTEM_TRAY_CANCEL_MESSAGE:
      egg_tray_manager_handle_cancel_message (manager, xevent);
      return GDK_FILTER_REMOVE;
    default:
      break;
    }

  return GDK_FILTER_CONTINUE;
}

static GdkFilterReturn
egg_tray_manager_window_filter (GdkXEvent *xev, GdkEvent *event, gpointer data)
{
  XEvent *xevent = (GdkXEvent *)xev;
  EggTrayManager *manager = data;

  if (xevent->type == ClientMessage)
    {
      if (xevent->xclient.message_type == manager->opcode_atom)
	{
	  return egg_tray_manager_handle_event (manager, (XClientMessageEvent *)xevent);
	}
      else if (xevent->xclient.message_type == manager->message_data_atom)
	{
	  egg_tray_manager_handle_message_data (manager, (XClientMessageEvent *)xevent);
	  return GDK_FILTER_REMOVE;
	}
    }
  else if (xevent->type == SelectionClear)
    {
      g_signal_emit (manager, manager_signals[LOST_SELECTION], 0);
      egg_tray_manager_unmanage (manager);
    }
  
  return GDK_FILTER_CONTINUE;
}

static void
egg_tray_manager_unmanage (EggTrayManager *manager)
{
  Display *display;
  guint32 timestamp;
  GtkWidget *invisible;

  if (manager->invisible == NULL)
    return;

  invisible = manager->invisible;
  g_assert (GTK_IS_INVISIBLE (invisible));
  g_assert (GTK_WIDGET_REALIZED (invisible));
  g_assert (GDK_IS_WINDOW (invisible->window));
  
  display = GDK_WINDOW_XDISPLAY (invisible->window);
  
  if (XGetSelectionOwner (display, manager->selection_atom) ==
      GDK_WINDOW_XWINDOW (invisible->window))
    {
      timestamp = gdk_x11_get_server_time (invisible->window);      
      XSetSelectionOwner (display, manager->selection_atom, None, timestamp);
    }

  gdk_window_remove_filter (invisible->window, egg_tray_manager_window_filter, manager);  

  manager->invisible = NULL; /* prior to destroy for reentrancy paranoia */
  gtk_widget_destroy (invisible);
  g_object_unref (G_OBJECT (invisible));
}

static gboolean
egg_tray_manager_manage_xscreen (EggTrayManager *manager, Screen *xscreen)
{
  GtkWidget *invisible;
  char *selection_atom_name;
  guint32 timestamp;
  GdkScreen *screen;
  
  g_return_val_if_fail (EGG_IS_TRAY_MANAGER (manager), FALSE);
  g_return_val_if_fail (manager->screen == NULL, FALSE);

  /* If there's already a manager running on the screen
   * we can't create another one.
   */
#if 0
  if (egg_tray_manager_check_running_xscreen (xscreen))
    return FALSE;
#endif
  screen = gdk_display_get_screen (gdk_x11_lookup_xdisplay (DisplayOfScreen (xscreen)),
				   XScreenNumberOfScreen (xscreen));
  
  invisible = gtk_invisible_new_for_screen (screen);
  gtk_widget_realize (invisible);
  
  gtk_widget_add_events (invisible, GDK_PROPERTY_CHANGE_MASK | GDK_STRUCTURE_MASK);

  selection_atom_name = g_strdup_printf ("_NET_SYSTEM_TRAY_S%d",
					 XScreenNumberOfScreen (xscreen));
  manager->selection_atom = XInternAtom (DisplayOfScreen (xscreen), selection_atom_name, False);

  g_free (selection_atom_name);
  
  timestamp = gdk_x11_get_server_time (invisible->window);
  XSetSelectionOwner (DisplayOfScreen (xscreen), manager->selection_atom,
		      GDK_WINDOW_XWINDOW (invisible->window), timestamp);

  /* Check if we were could set the selection owner successfully */
  if (XGetSelectionOwner (DisplayOfScreen (xscreen), manager->selection_atom) ==
      GDK_WINDOW_XWINDOW (invisible->window))
    {
      XClientMessageEvent xev;

      xev.type = ClientMessage;
      xev.window = RootWindowOfScreen (xscreen);
      xev.message_type = XInternAtom (DisplayOfScreen (xscreen), "MANAGER", False);

      xev.format = 32;
      xev.data.l[0] = timestamp;
      xev.data.l[1] = manager->selection_atom;
      xev.data.l[2] = GDK_WINDOW_XWINDOW (invisible->window);
      xev.data.l[3] = 0;	/* manager specific data */
      xev.data.l[4] = 0;	/* manager specific data */

      XSendEvent (DisplayOfScreen (xscreen),
		  RootWindowOfScreen (xscreen),
		  False, StructureNotifyMask, (XEvent *)&xev);

      manager->invisible = invisible;
      g_object_ref (G_OBJECT (manager->invisible));
      
      manager->opcode_atom = XInternAtom (DisplayOfScreen (xscreen),
					  "_NET_SYSTEM_TRAY_OPCODE",
					  False);

      manager->message_data_atom = XInternAtom (DisplayOfScreen (xscreen),
						"_NET_SYSTEM_TRAY_MESSAGE_DATA",
						False);

      /* Add a window filter */
      gdk_window_add_filter (invisible->window, egg_tray_manager_window_filter, manager);
      return TRUE;
    }
  else
    {
      gtk_widget_destroy (invisible);
 
      return FALSE;
    }
}

gboolean
egg_tray_manager_manage_screen (EggTrayManager *manager,
				GdkScreen      *screen)
{
  g_return_val_if_fail (GDK_IS_SCREEN (screen), FALSE);
  g_return_val_if_fail (manager->screen == NULL, FALSE);

  return egg_tray_manager_manage_xscreen (manager, 
					  GDK_SCREEN_XSCREEN (screen));
}

static gboolean
egg_tray_manager_check_running_xscreen (Screen *xscreen)
{
  Atom selection_atom;
  char *selection_atom_name;

  selection_atom_name = g_strdup_printf ("_NET_SYSTEM_TRAY_S%d",
					 XScreenNumberOfScreen (xscreen));
  selection_atom = XInternAtom (DisplayOfScreen (xscreen), selection_atom_name, False);
  g_free (selection_atom_name);

  if (XGetSelectionOwner (DisplayOfScreen (xscreen), selection_atom))
    return TRUE;
  else
    return FALSE;
}

gboolean
egg_tray_manager_check_running (GdkScreen *screen)
{
  g_return_val_if_fail (GDK_IS_SCREEN (screen), FALSE);

  return egg_tray_manager_check_running_xscreen (GDK_SCREEN_XSCREEN (screen));
}

char *
egg_tray_manager_get_child_title (EggTrayManager *manager,
				  EggTrayManagerChild *child)
{
  Window *child_window;
  Atom utf8_string, atom, type;
  int result;
  gchar *val, *retval;
  int format;
  gulong nitems;
  gulong bytes_after;
  guchar *tmp = NULL;
  
  g_return_val_if_fail (EGG_IS_TRAY_MANAGER (manager), NULL);
  g_return_val_if_fail (GTK_IS_SOCKET (child), NULL);
  
  child_window = g_object_get_data (G_OBJECT (child),
        "egg-tray-child-window");
  
  utf8_string = XInternAtom (GDK_DISPLAY (), "UTF8_STRING", False);
  atom = XInternAtom (GDK_DISPLAY (), "_NET_WM_NAME", False);
  
  gdk_error_trap_push();

  result = XGetWindowProperty (GDK_DISPLAY (), *child_window, atom, 0,
        G_MAXLONG, False, utf8_string, &type, &format, &nitems,
        &bytes_after, &tmp);
  val = (gchar *) tmp;
  if (gdk_error_trap_pop() || result != Success || type != utf8_string)
    return NULL;

  if (format != 8 || nitems == 0) {
      if (val)
          XFree (val);
      return NULL;
  }

  if (!g_utf8_validate (val, nitems, NULL))
    {
      XFree (val);
      return NULL;
    }

  retval = g_strndup (val, nitems);

  XFree (val);

  return retval;

}
