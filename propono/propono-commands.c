/*
 * propono-commands.c
 * This file is part of propono
 *
 * Copyright (C) 2007,2008,2009 - Jonh Wendell <wendell@bani.com.br>
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

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <glib.h>
#include <glib/gi18n.h>
#include <gtk/gtk.h>
#include <string.h>

#include "propono-commands.h"
#include "propono-utils.h"
#include "propono-connection.h"
#include "propono-notebook.h"
#include "propono-tab.h"
#include "propono-connect.h"
#include "propono-bookmarks.h"
#include "propono-bookmarks-ui.h"
#include "propono-fav.h"
#include "propono-window-private.h"
#include "propono-prefs.h"
#include "propono-cache-prefs.h"
#include "propono-plugin.h"
#include "propono-plugin-info.h"
#include "propono-plugin-info-priv.h"
#include "propono-plugins-engine.h"

void
propono_cmd_direct_connect (ProponoConnection *conn,
			    ProponoWindow     *window)
{
  ProponoTab *tab;

  g_return_if_fail (PROPONO_IS_WINDOW (window));

  tab = propono_window_conn_exists (window, conn);
  if (tab)
    {
      propono_window_set_active_tab (window, tab);
    }
  else
    {
      tab = PROPONO_TAB (propono_tab_new (conn, window));
      propono_notebook_add_tab (PROPONO_NOTEBOOK (window->priv->notebook),
				PROPONO_TAB (tab),
				-1);
    }
}

/* Machine Menu */
void
propono_cmd_machine_connect (GtkAction     *action,
			     ProponoWindow *window)
{
  ProponoTab *tab;
  ProponoConnection *conn;

  g_return_if_fail (PROPONO_IS_WINDOW (window));

  conn = propono_connect (window);
  if (!conn)
    return;

  tab = propono_window_conn_exists (window, conn);
  if (tab)
    {
      propono_window_set_active_tab (window, tab);
    }
  else
    {
      tab = PROPONO_TAB (propono_tab_new (conn, window));
      propono_notebook_add_tab (PROPONO_NOTEBOOK (window->priv->notebook),
				PROPONO_TAB (tab),
				-1);
    }

  g_object_unref (conn);
}

static void
propono_cmd_free_string_list (gpointer str, gpointer user_data)
{
  g_free (str);
}

void
propono_cmd_machine_open (GtkAction     *action,
			  ProponoWindow *window)
{
  GtkWidget         *tab;
  ProponoConnection *conn;
  GtkWidget         *dialog;
  GtkFileFilter     *filter;
  GSList            *files, *l;
  gchar             *uri;
  gchar             *error = NULL;
  GSList            *plugins, *errors = NULL;
  gint              i;

  g_return_if_fail (PROPONO_IS_WINDOW (window));

  dialog = gtk_file_chooser_dialog_new (_("Choose the file"),
					GTK_WINDOW (window),
					GTK_FILE_CHOOSER_ACTION_OPEN,
					GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
					GTK_STOCK_OPEN, GTK_RESPONSE_ACCEPT,
					NULL);

  gtk_file_chooser_set_local_only (GTK_FILE_CHOOSER (dialog), FALSE);
  gtk_file_chooser_set_select_multiple (GTK_FILE_CHOOSER (dialog), TRUE);

  plugins = (GSList *) propono_plugins_engine_get_plugin_list (propono_plugins_engine_get_default ());
  i = 0;
  for (; plugins; plugins = plugins->next)
    {
      ProponoPluginInfo *info = PROPONO_PLUGIN_INFO (plugins->data);

      if (!propono_plugin_info_is_active (info))
	continue;

      filter = propono_plugin_get_file_filter (info->plugin);
      if (filter)
	{
	  gtk_file_chooser_add_filter (GTK_FILE_CHOOSER (dialog), filter);
	  i++;
	}
    }
  if (i == 0)
    {
      propono_utils_show_error (_("There are no supported files"),
				_("None of the active plugins support this action. Activate some plugins and try again."),
				GTK_WINDOW (window));
      goto finalize;
    }

  if (gtk_dialog_run (GTK_DIALOG (dialog)) == GTK_RESPONSE_ACCEPT)
    {
      files = gtk_file_chooser_get_uris (GTK_FILE_CHOOSER (dialog));
      for (l = files; l; l = l->next)
	{
	  uri = (gchar *)l->data;
	  conn = propono_connection_new_from_file (uri, &error, FALSE);

	  if (conn)
	    {
	      tab = propono_tab_new (conn, window);
	      propono_notebook_add_tab (PROPONO_NOTEBOOK (window->priv->notebook),
					PROPONO_TAB (tab),
					-1);
	      g_object_unref (conn);
	    }
	  else
	    {
	      errors = g_slist_append (errors, g_strdup_printf ("<i>%s</i>: %s", uri, error?error:_("Unknown error")));
	      g_free (error);
	    }

	  g_free (uri);
	}
      g_slist_free (files);
    }

  if (errors)
    {
      propono_utils_show_many_errors (ngettext ("The following file could not be opened:",
						 "The following files could not be opened:",
						 g_slist_length (errors)),
				      errors,
				      GTK_WINDOW (window));
      g_slist_foreach (errors, propono_cmd_free_string_list, NULL);
      g_slist_free (errors);
    }

finalize:
  gtk_widget_destroy (dialog);
}

void
propono_cmd_machine_close (GtkAction     *action,
			   ProponoWindow *window)
{
  propono_window_close_active_tab (window);
}

void
propono_cmd_machine_take_screenshot (GtkAction     *action,
				     ProponoWindow *window)
{
  propono_tab_take_screenshot (propono_window_get_active_tab (window));
}

void
propono_cmd_machine_close_all (GtkAction     *action,
			       ProponoWindow *window)
{
  propono_window_close_all_tabs (window);
}

void
propono_cmd_machine_quit (GtkAction     *action,
			  ProponoWindow *window)
{
  gtk_widget_destroy (GTK_WIDGET (window));
}

/* Edit Menu */
void
propono_cmd_edit_preferences (GtkAction     *action,
			      ProponoWindow *window)
{
  propono_prefs_dialog_show (window);
}

void
propono_cmd_edit_plugins (GtkAction     *action,
                          ProponoWindow *window)
{
  g_return_if_fail (PROPONO_IS_WINDOW (window));

  propono_plugin_dialog_show (GTK_WINDOW (window));
}

/* View Menu */
void
propono_cmd_view_show_toolbar	(GtkAction     *action,
				 ProponoWindow *window)
{
  g_return_if_fail (PROPONO_IS_WINDOW (window));

  propono_utils_toggle_widget_visible (window->priv->toolbar);

  propono_cache_prefs_set_boolean ("window",
				   "toolbar-visible",
				   GTK_WIDGET_VISIBLE (window->priv->toolbar));
}

void
propono_cmd_view_show_statusbar	(GtkAction     *action,
				 ProponoWindow *window)
{
  g_return_if_fail (PROPONO_IS_WINDOW (window));

  propono_utils_toggle_widget_visible (window->priv->statusbar);

  propono_cache_prefs_set_boolean ("window",
				   "statusbar-visible",
				   GTK_WIDGET_VISIBLE (window->priv->statusbar));
}

void
propono_cmd_view_show_fav_panel	(GtkAction     *action,
				 ProponoWindow *window)
{
  g_return_if_fail (PROPONO_IS_WINDOW (window));

  propono_utils_toggle_widget_visible (window->priv->fav_panel);

  propono_cache_prefs_set_boolean ("window",
				   "side-panel-visible",
				   GTK_WIDGET_VISIBLE (window->priv->fav_panel));
}

void
propono_cmd_view_fullscreen (GtkAction     *action,
			     ProponoWindow *window)
{
  g_return_if_fail (PROPONO_IS_WINDOW (window));

  propono_window_toggle_fullscreen (window);
}

/* Bookmarks Menu */
void
propono_cmd_open_bookmark (ProponoWindow     *window,
			   ProponoConnection *conn)
{
  ProponoTab *tab;
  ProponoConnection *new_conn;

  g_return_if_fail (PROPONO_IS_WINDOW (window));
  g_return_if_fail (PROPONO_IS_CONNECTION (conn));

  tab = propono_window_conn_exists (window, conn);
  if (tab)
    {
      propono_window_set_active_tab (window, tab);
    }
  else
    {
      tab = PROPONO_TAB (propono_tab_new (conn, window));
      propono_notebook_add_tab (PROPONO_NOTEBOOK (window->priv->notebook),
				PROPONO_TAB (tab),
				-1);
    }
}

void
propono_cmd_bookmarks_add (GtkAction     *action,
			   ProponoWindow *window)
{
  ProponoTab        *tab;
  ProponoConnection *conn;

  g_return_if_fail (PROPONO_IS_WINDOW (window));

  tab = propono_window_get_active_tab (window);
  g_return_if_fail (PROPONO_IS_TAB (tab));

  conn = propono_tab_get_conn (tab);
  g_return_if_fail (PROPONO_IS_CONNECTION (conn));

  propono_bookmarks_add (propono_bookmarks_get_default (),
                         conn,
                         GTK_WINDOW (window));

  if (propono_window_get_active_tab (window) == tab)
    {
      gchar *name = propono_connection_get_best_name (conn);
      propono_tab_set_title (tab, name);
      g_free (name);
    }
}

/* Help Menu */

void
propono_cmd_help_contents (GtkAction     *action,
			   ProponoWindow *window)
{
  propono_utils_help_contents (GTK_WINDOW (window), NULL);
}

void
propono_cmd_help_about (GtkAction     *action,
			ProponoWindow *window)
{
  propono_utils_help_about (GTK_WINDOW (window));
}

/* vim: set ts=8: */
