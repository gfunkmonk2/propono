/*
 * propono-connect.c
 * This file is part of propono
 *
 * Copyright (C) 2007 - Jonh Wendell <wendell@bani.com.br>
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

#ifdef PROPONO_ENABLE_AVAHI
#include <avahi-ui/avahi-ui.h>
#endif

#include "propono-connect.h"
#include "propono-utils.h"
#include "propono-bookmarks.h"
#include "propono-prefs.h"
#include "propono-cache-prefs.h"
#include "propono-plugins-engine.h"
#include "propono-plugin.h"

typedef struct {
  GtkBuilder *xml;
  GtkWidget *dialog;
  GtkWidget *protocol_combo;
  GtkWidget *protocol_description_label;
  GtkListStore *protocol_store;
  GtkWidget *host_entry;
  GtkWidget *find_button;
  GtkWidget *fullscreen_check;
  GtkWidget *plugin_box;
  GtkWidget *connect_button;
} ProponoConnectDialog;

enum {
  COLUMN_TEXT,
  N_COLUMNS
};

enum {
  PROTOCOL_NAME,
  PROTOCOL_DESCRIPTION,
  PROTOCOL_MDNS,
  PROTOCOL_OPTIONS,
  PROTOCOL_PLUGIN,
  N_PROTOCOLS
};

static gchar*
history_filename () {
  return g_build_filename (g_get_user_data_dir (),
			   "propono",
			   "history",
			   NULL);
}

static void
protocol_combo_changed (GtkComboBox *combo, ProponoConnectDialog *dialog)
{
  GtkTreeIter tree_iter;
  gchar       *description, *service;
  GtkWidget   *options;
  GList       *children, *l;

  if (!gtk_combo_box_get_active_iter (combo, &tree_iter))
    {
      /* Translators: Please consider the word "from" instead of "in". I'll fix this phrase for 2.31 (or 3.0) */
      g_warning (_("Could not get the active protocol in the protocol list."));
      return;
    }

  gtk_tree_model_get (GTK_TREE_MODEL (dialog->protocol_store), &tree_iter,
		      PROTOCOL_DESCRIPTION, &description,
		      PROTOCOL_MDNS, &service,
		      PROTOCOL_OPTIONS, &options,
		      -1);

  gtk_label_set_label (GTK_LABEL (dialog->protocol_description_label),
		       description);

#ifdef PROPONO_ENABLE_AVAHI
  if (service)
    gtk_widget_show (dialog->find_button);
  else
    gtk_widget_hide (dialog->find_button);
#endif

  children = gtk_container_get_children (GTK_CONTAINER (dialog->plugin_box));
  for (l = children; l; l = l->next)
    gtk_container_remove (GTK_CONTAINER (dialog->plugin_box), GTK_WIDGET (l->data));
  g_list_free (children);

  if (options)
    {
      gtk_box_pack_start (GTK_BOX (dialog->plugin_box), options, TRUE, TRUE, 0);
      gtk_widget_show_all (dialog->plugin_box);
    }
  else
    gtk_widget_hide (dialog->plugin_box);

  g_free (description);
  g_free (service);
}

static void
setup_protocol (ProponoConnectDialog *dialog)
{
  GHashTable      *plugins;
  GHashTableIter  hash_iter;
  gpointer        key, value;
  GtkTreeIter     tree_iter;
  GtkCellRenderer *rend;
  gchar           *last_protocol;
  gint            selected, i;

  dialog->protocol_store = gtk_list_store_new (N_PROTOCOLS, G_TYPE_STRING, G_TYPE_STRING, G_TYPE_STRING, G_TYPE_OBJECT, G_TYPE_OBJECT);
  plugins = propono_plugin_engine_get_plugins_by_protocol (propono_plugins_engine_get_default ());
  last_protocol = propono_cache_prefs_get_string ("connection", "last-protocol", NULL);

  g_hash_table_iter_init (&hash_iter, plugins);
  selected = 0;
  i = 0;
  while (g_hash_table_iter_next (&hash_iter, &key, &value)) 
    {
      gchar         **description;
      ProponoPlugin *plugin = PROPONO_PLUGIN (value);
      GtkWidget     *widget;

      description = propono_plugin_get_public_description (plugin);
      if (!description || !description[0])
	continue;

      widget = propono_plugin_get_connect_widget (plugin, NULL);

      gtk_list_store_append (dialog->protocol_store, &tree_iter);
      gtk_list_store_set (dialog->protocol_store, &tree_iter,
			  PROTOCOL_NAME, description[0],
			  PROTOCOL_DESCRIPTION, description[1],
			  PROTOCOL_MDNS, propono_plugin_get_mdns_service (plugin),
			  PROTOCOL_OPTIONS, widget,
			  PROTOCOL_PLUGIN, plugin,
			  -1);

      if (last_protocol && g_str_equal (last_protocol, description[0]))
        selected = i;

      g_strfreev (description);
      if (widget)
	g_object_unref (widget);
      i++;
    }

  gtk_combo_box_set_model (GTK_COMBO_BOX (dialog->protocol_combo),
			   GTK_TREE_MODEL (dialog->protocol_store));
  rend = gtk_cell_renderer_text_new ();
  gtk_cell_layout_pack_start (GTK_CELL_LAYOUT (dialog->protocol_combo), rend, TRUE);
  gtk_cell_layout_add_attribute (GTK_CELL_LAYOUT (dialog->protocol_combo), rend, "text", 0);

  g_signal_connect (dialog->protocol_combo,
		    "changed",
		    G_CALLBACK (protocol_combo_changed),
		    dialog);
  gtk_combo_box_set_active (GTK_COMBO_BOX (dialog->protocol_combo), selected);
  g_free (last_protocol);
}

static GPtrArray *
saved_history (void)
{
  gchar *filename, *file_contents = NULL;
  gchar **history_from_file = NULL, **list;
  gboolean success;
  gint len;
  GPtrArray *array;

  array = g_ptr_array_new_with_free_func (g_free);

  filename = history_filename ();
  success = g_file_get_contents (filename,
				 &file_contents,
				 NULL,
				 NULL);

  if (success)
    {
      history_from_file = g_strsplit (file_contents, "\n", 0);
      len = g_strv_length (history_from_file);
      if (len > 0 && strlen (history_from_file[len-1]) == 0)
	{
	  g_free (history_from_file[len-1]);
	  history_from_file[len-1] = NULL;
	}

      list = history_from_file;

      while (*list != NULL)
	g_ptr_array_add (array, *list++);
    }

  g_free (filename);
  g_free (file_contents);
  g_free (history_from_file);
  return array;
}

static void
control_connect_button (GtkEditable *entry, ProponoConnectDialog *dialog)
{
  gtk_widget_set_sensitive (dialog->connect_button,
			    gtk_entry_get_text_length (GTK_ENTRY (entry)) > 0);
}

static void
setup_combo (ProponoConnectDialog *dialog)
{
  GtkListStore *store;
  GtkTreeIter   iter;
  GtkEntryCompletion *completion;
  GPtrArray    *history;
  gint          i, size;
  GtkEntry     *entry;

  entry = GTK_ENTRY (gtk_bin_get_child (GTK_BIN (dialog->host_entry)));
  store = gtk_list_store_new (N_COLUMNS, G_TYPE_STRING);

  history = saved_history ();

  g_object_get (propono_prefs_get_default (), "history-size", &size, NULL);
  if (size <= 0)
    size = G_MAXINT;

  for (i=history->len-1; i>=0 && i>=(gint)history->len-size; i--)
   {
      GtkTreeIter iter;
      gtk_list_store_append (store, &iter);
      gtk_list_store_set (store, &iter, COLUMN_TEXT, g_ptr_array_index (history, i), -1);
    }
  g_ptr_array_free (history, TRUE);

  gtk_combo_box_set_model (GTK_COMBO_BOX (dialog->host_entry),
			   GTK_TREE_MODEL (store));
  gtk_combo_box_entry_set_text_column (GTK_COMBO_BOX_ENTRY (dialog->host_entry),
				       0);

  completion = gtk_entry_completion_new ();
  gtk_entry_completion_set_model (completion, GTK_TREE_MODEL (store));
  gtk_entry_completion_set_text_column (completion, 0);
  gtk_entry_completion_set_inline_completion (completion, TRUE);
  gtk_entry_set_completion (entry, completion);
  g_object_unref (completion);

  gtk_entry_set_activates_default (entry, TRUE);
  g_signal_connect (entry, "changed", G_CALLBACK (control_connect_button), dialog);
}

static void
save_history (GtkWidget *combo) {
  gchar *host;
  GPtrArray *history;
  guint i;
  gchar *filename, *path;
  GString *content;
  GError *error = NULL;

  host = gtk_combo_box_get_active_text (GTK_COMBO_BOX (combo));

  history = saved_history ();
  for (i=0; i<history->len; i++)
    if (!g_strcmp0 (g_ptr_array_index (history, i), host))
      {
	g_ptr_array_remove_index (history, i);
	break;
      }

  g_ptr_array_add (history, host);
  content = g_string_new (NULL);

  for (i=0; i<history->len; i++)
    g_string_append_printf (content, "%s\n", (char *) g_ptr_array_index (history, i));

  filename = history_filename ();
  path = g_path_get_dirname (filename);
  g_mkdir_with_parents (path, 0755);

  g_file_set_contents (filename,
		       content->str,
		       -1,
		       &error);

  g_free (filename);
  g_free (path);
  g_ptr_array_free (history, TRUE);
  g_string_free (content, TRUE);

  if (error) {
    g_warning (_("Error while saving history file: %s"), error->message);
    g_error_free (error);
  }
}

#ifdef PROPONO_ENABLE_AVAHI
static void
propono_connect_find_button_cb (GtkButton            *button,
				ProponoConnectDialog *dialog)
{
  GtkWidget     *d;
  GtkTreeIter   tree_iter;
  gchar         *service;
  GtkWidget     *options = NULL;
  ProponoPlugin *plugin = NULL;

  if (!gtk_combo_box_get_active_iter (GTK_COMBO_BOX (dialog->protocol_combo),
				      &tree_iter))
    {
      g_warning (_("Could not get the active protocol in the protocol list."));
      return;
    }

  gtk_tree_model_get (GTK_TREE_MODEL (dialog->protocol_store), &tree_iter,
		      PROTOCOL_MDNS, &service,
		      PROTOCOL_PLUGIN, &plugin,
		      PROTOCOL_OPTIONS, &options,
		      -1);
  if (!service)
    {
      if (plugin)
	g_object_unref (plugin);
      return;
    }

  d = aui_service_dialog_new (_("Choose a Remote Desktop"),
				GTK_WINDOW(dialog->dialog),
				GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
				GTK_STOCK_OK, GTK_RESPONSE_ACCEPT,
				NULL);
  gtk_window_set_transient_for (GTK_WINDOW(d), GTK_WINDOW(dialog->dialog));
  aui_service_dialog_set_resolve_service (AUI_SERVICE_DIALOG(d), TRUE);
  aui_service_dialog_set_resolve_host_name (AUI_SERVICE_DIALOG(d), TRUE);
  aui_service_dialog_set_browse_service_types (AUI_SERVICE_DIALOG(d),
					       service,
					       NULL);

  if (gtk_dialog_run(GTK_DIALOG(d)) == GTK_RESPONSE_ACCEPT)
    {
      gchar *tmp;
      char a[AVAHI_ADDRESS_STR_MAX];

      avahi_address_snprint (a,
			     sizeof(a),
			     aui_service_dialog_get_address (AUI_SERVICE_DIALOG (d)));
      tmp = g_strdup_printf ("%s::%d",
			     a,
			     aui_service_dialog_get_port (AUI_SERVICE_DIALOG (d)));

      gtk_entry_set_text (GTK_ENTRY (gtk_bin_get_child (GTK_BIN (dialog->host_entry))),
			  tmp);

      g_free (tmp);
      if (plugin && options)
	propono_plugin_parse_mdns_dialog (plugin, options, d);
    }

  g_free (service);
  gtk_widget_destroy (d);
  if (plugin)
    g_object_unref (plugin);
}
#endif

ProponoConnection *propono_connect (ProponoWindow *window)
{
  ProponoConnection    *conn = NULL;
  gint                  result;
  ProponoConnectDialog  dialog;

  dialog.xml = propono_utils_get_builder (NULL, NULL);
  if (!dialog.xml)
    return NULL;

  dialog.dialog = GTK_WIDGET (gtk_builder_get_object (dialog.xml, "connect_dialog"));
  gtk_window_set_transient_for (GTK_WINDOW (dialog.dialog), GTK_WINDOW (window));

  dialog.protocol_combo = GTK_WIDGET (gtk_builder_get_object (dialog.xml, "protocol_combo"));
  dialog.protocol_description_label = GTK_WIDGET (gtk_builder_get_object (dialog.xml, "protocol_description_label"));
  dialog.host_entry  = GTK_WIDGET (gtk_builder_get_object (dialog.xml, "host_entry"));
  dialog.find_button = GTK_WIDGET (gtk_builder_get_object (dialog.xml, "find_button"));
  dialog.fullscreen_check = GTK_WIDGET (gtk_builder_get_object (dialog.xml, "fullscreen_check"));
  dialog.plugin_box = GTK_WIDGET (gtk_builder_get_object (dialog.xml, "plugin_options_connect_vbox"));
  dialog.connect_button = GTK_WIDGET (gtk_builder_get_object (dialog.xml, "connect_button"));

  setup_protocol (&dialog);
  setup_combo (&dialog);
  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (dialog.fullscreen_check),
				propono_cache_prefs_get_boolean ("connection", "fullscreen", FALSE));

#ifdef PROPONO_ENABLE_AVAHI
  g_signal_connect (dialog.find_button,
		    "clicked",
		    G_CALLBACK (propono_connect_find_button_cb),
		    &dialog);
#else
  gtk_widget_hide (dialog.find_button);
  gtk_widget_set_no_show_all (dialog.find_button, TRUE);
#endif

  gtk_widget_show_all (dialog.dialog);
  result = gtk_dialog_run (GTK_DIALOG (dialog.dialog));

  if (result == GTK_RESPONSE_OK)
    {
      gchar         *host = NULL, *error_msg = NULL, *protocol = NULL, *actual_host;
      gint          port;
      ProponoPlugin *plugin;
      GtkWidget     *options;
      GtkTreeIter   iter;

      host = gtk_combo_box_get_active_text (GTK_COMBO_BOX (dialog.host_entry));
      gtk_widget_hide (GTK_WIDGET (dialog.dialog));

      if (!host || !g_strcmp0 (host, ""))
	goto fail;

      save_history (dialog.host_entry);

      if (!gtk_combo_box_get_active_iter (GTK_COMBO_BOX (dialog.protocol_combo), &iter))
	{
	  g_warning (_("Could not get the active protocol in the protocol list."));
	  goto fail;
	}

      gtk_tree_model_get (GTK_TREE_MODEL (dialog.protocol_store), &iter,
			  PROTOCOL_NAME, &protocol,
			  PROTOCOL_OPTIONS, &options,
			  PROTOCOL_PLUGIN, &plugin,
		      -1);

      propono_cache_prefs_set_string ("connection", "last-protocol", protocol);
      g_free (protocol);
      propono_cache_prefs_set_boolean ("connection", "fullscreen", gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (dialog.fullscreen_check)));

      conn = propono_plugin_new_connection (plugin);
      if (propono_connection_split_string (host,
					   propono_connection_get_protocol (conn),
					   &protocol,
					   &actual_host,
					   &port,
					   &error_msg))
	{
	  g_object_set (conn,
			"host", actual_host,
			"port", port,
			"fullscreen", gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (dialog.fullscreen_check)),
			NULL);

	  if (options)
	    propono_connection_parse_options_widget (conn, options);

	  g_free (protocol);
	  g_free (actual_host);
	}
      else
	{
	  propono_utils_show_error (NULL, error_msg ? error_msg : _("Unknown error"),
				    GTK_WINDOW (window));
	}

      g_object_unref (plugin);
      if (options)
	g_object_unref (options);

fail:
      g_free (host);
      g_free (error_msg);
    }

  gtk_widget_destroy (dialog.dialog);
  g_object_unref (dialog.xml);
  return conn;
}
/* vim: set ts=8: */
