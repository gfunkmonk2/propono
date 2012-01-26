/*
 * propono-bacon.c
 * This file is part of propono
 *
 * Copyright (C) 2008,2009 - Jonh Wendell <wendell@bani.com.br>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program. If not, see <http://www.gnu.org/licenses/>.
 */

#include <stdlib.h>
#include <glib.h>
#include <gtk/gtk.h>
#include <string.h>
#include "bacon-message-connection.h"
#include "propono-utils.h"
#include "propono-connection.h"
#include "propono-app.h"
#include "propono-window.h"
#include "propono-commands.h"

static guint32 startup_timestamp = 0;

static GdkDisplay *
display_open_if_needed (const gchar *name)
{
  GSList *displays;
  GSList *l;
  GdkDisplay *display = NULL;

  displays = gdk_display_manager_list_displays (gdk_display_manager_get ());

  for (l = displays; l != NULL; l = l->next)
    {
      if (strcmp (gdk_display_get_name ((GdkDisplay *) l->data), name) == 0)
	{
	  display = l->data;
	  break;
	}
    }

  g_slist_free (displays);

  return display != NULL ? display : gdk_display_open (name);
}

/* serverside */
void
propono_bacon_message_received (const char *message,
				gpointer    data)
{
  gchar **commands;
  gchar **params;
  gint workspace;
  gint viewport_x;
  gint viewport_y;
  gchar *display_name;
  gint screen_number;
  gint i;
  ProponoApp *app;
  ProponoWindow *window;
  GdkDisplay *display;
  GdkScreen *screen;
  gboolean new_window = FALSE;
  GSList *servers = NULL;
  GSList *l, *next;
  ProponoConnection *conn;
  gchar *error;

  g_return_if_fail (message != NULL);

  commands = g_strsplit (message, "\v", -1);

  /* header */
  params = g_strsplit (commands[0], "\t", 6);
  startup_timestamp = atoi (params[0]);
  display_name = params[1];
  screen_number = atoi (params[2]);
  workspace = atoi (params[3]);
  viewport_x = atoi (params[4]);
  viewport_y = atoi (params[5]);

  display = display_open_if_needed (display_name);
  if (display == NULL)
    {
      g_warning ("Could not open display %s\n", display_name);
      g_strfreev (params);
      goto out;
    }

  screen = gdk_display_get_screen (display, screen_number);
  g_strfreev (params);

  /* body */
  for (i = 1; commands[i] != NULL; i++)
    {
      params = g_strsplit (commands[i], "\t", -1);

      if (strcmp (params[0], "NEW-WINDOW") == 0)
	  new_window = TRUE;

      else if (strcmp (params[0], "OPEN-URIS") == 0)
	{
	  gint n_uris, j;
	  gchar **uris;

	  n_uris = atoi (params[1]);
	  uris = g_strsplit (params[2], " ", n_uris);

	  for (j = 0; j < n_uris; j++)
	    {
	      conn = propono_connection_new_from_string (uris[j], &error, TRUE);
	      if (conn)
		servers = g_slist_prepend (servers, conn);
	      else
		/*FIXME: Show a dialog error?*/
		g_free (error);
	    }

	  g_strfreev (uris);
	}

	else
	  {
	    g_warning ("Unexpected bacon command");
	  }

      g_strfreev (params);
    }

  /* execute the commands */
  app = propono_app_get_default ();
  if (new_window)
    window = propono_app_create_window (app, screen);
  else
    /* get a window in the current workspace (if exists) and raise it */
    window = propono_app_get_window_in_viewport (app,
						 screen,
						 workspace,
						 viewport_x,
						 viewport_y);

  for (l = servers; l; l = next)
    {
      ProponoConnection *conn = l->data;
      
      next = l->next;
      propono_cmd_direct_connect (conn, window);
      g_object_unref (conn);
    }
  g_slist_free (servers);

  /* set the proper interaction time on the window.
   * Fall back to roundtripping to the X server when we
   * don't have the timestamp, e.g. when launched from
   * terminal. We also need to make sure that the window
   * has been realized otherwise it will not work. lame.
   */
  if (!GTK_WIDGET_REALIZED (window))
    gtk_widget_realize (GTK_WIDGET (window));

  if (startup_timestamp <= 0)
    startup_timestamp = gdk_x11_get_server_time (GTK_WIDGET (window)->window);

  gdk_x11_window_set_user_time (GTK_WIDGET (window)->window,
				startup_timestamp);
  gtk_window_present (GTK_WINDOW (window));

  out:
  g_strfreev (commands);
}

/* clientside */
static void
propono_bacon_send_message (BaconMessageConnection *connection,
			    GSList *servers,
			    gboolean new_window)
{
  GdkScreen *screen;
  GdkDisplay *display;
  const gchar *display_name;
  gint screen_number;
  gint ws;
  gint viewport_x;
  gint viewport_y;
  GString *command;

  /* the messages have the following format:
   * <---                                  header                                     ---> <----            body             ----->
   * timestamp \t display_name \t screen_number \t workspace \t viewport_x \t viewport_y \v OP1 \t arg \t arg \v OP2 \t arg \t arg|...
   *
   * when the arg is a list of uri, they are separated by a space.
   * So the delimiters are \v for the commands, \t for the tokens in
   * a command and ' ' for the uris: note that such delimiters cannot
   * be part of an uri, this way parsing is easier.
   */

  screen = gdk_screen_get_default ();
  display = gdk_screen_get_display (screen);

  display_name = gdk_display_get_name (display);
  screen_number = gdk_screen_get_number (screen);

  ws = propono_utils_get_current_workspace (screen);
  propono_utils_get_current_viewport (screen, &viewport_x, &viewport_y);

  command = g_string_new (NULL);

  /* header */
  g_string_append_printf (command,
			   "%" G_GUINT32_FORMAT "\t%s\t%d\t%d\t%d\t%d",
			   startup_timestamp,
			   display_name,
			   screen_number,
			   ws,
			   viewport_x,
			   viewport_y);

  /* NEW-WINDOW command */
  if (new_window)
    {
      command = g_string_append_c (command, '\v');
      command = g_string_append (command, "NEW-WINDOW");
    }

  /* OPEN_URIS command */
  if (servers)
    {
      GSList *l;

      command = g_string_append_c (command, '\v');
      command = g_string_append (command, "OPEN-URIS");

      g_string_append_printf (command, "\t%d\t", g_slist_length (servers));

      for (l = servers; l != NULL; l = l->next)
	{
	  ProponoConnection *conn = l->data;
	  gchar             *uri = propono_connection_get_string_rep (conn, TRUE);

	  g_string_append (command, uri);
	  g_free (uri);

	  if (l->next != NULL)
	    command = g_string_append_c (command, ' ');
	}
    }

  bacon_message_connection_send (connection,
				 command->str);

  g_string_free (command, TRUE);
}

void
propono_bacon_start (GSList *servers, gboolean new_window)
{
  BaconMessageConnection *connection;

  connection = bacon_message_connection_new ("propono");
  if (!connection)
    {
      g_warning ("Cannot create the 'propono' connection.");
      return;
    }

  if (!bacon_message_connection_get_is_server (connection)) 
    {
      propono_bacon_send_message (connection, servers, new_window);

      gdk_notify_startup_complete ();

      g_slist_foreach (servers, (GFunc) g_object_unref, NULL);
      g_slist_free (servers);
      bacon_message_connection_free (connection);

      exit (0);
    }
  else 
    {
      bacon_message_connection_set_callback (connection,
					     propono_bacon_message_received,
					     NULL);
    }
}
/* vim: set ts=8: */
