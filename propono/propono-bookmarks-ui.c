/*
 * propono-bookmarks-ui.c
 * This file is part of propono
 *
 * Copyright (C) 2008  Jonh Wendell <wendell@bani.com.br>
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

#include <string.h>
#include <glib/gi18n.h>

#include "propono-bookmarks-ui.h"
#include "propono-utils.h"
#include "propono-bookmarks-tree.h"
#include "propono-plugin.h"
#include "propono-plugins-engine.h"

static void
control_save_button_visibility (GtkEntry *ed, GtkWidget *bt)
{
  gtk_widget_set_sensitive (bt,
			    gtk_entry_get_text_length (ed) > 0);
}

static void
show_dialog_folder (ProponoBookmarks      *book,
		    GtkWindow             *window,
		    ProponoBookmarksEntry *entry,
		    gboolean               is_add)
{
  GtkBuilder  *xml;
  GtkWidget   *dialog, *box, *tree, *name_entry, *save_button;
  const gchar *name;

  xml = propono_utils_get_builder (NULL, NULL);
  dialog     = GTK_WIDGET (gtk_builder_get_object (xml, "bookmarks_add_edit_folder_dialog"));
  name_entry = GTK_WIDGET (gtk_builder_get_object (xml, "edit_bookmark_folder_name_entry"));
  box        = GTK_WIDGET (gtk_builder_get_object (xml, "folder_box1"));
  save_button= GTK_WIDGET (gtk_builder_get_object (xml, "save_button"));

  gtk_window_set_transient_for (GTK_WINDOW (dialog), window);
  gtk_entry_set_text (GTK_ENTRY (name_entry), propono_bookmarks_entry_get_name (entry));
  gtk_editable_set_position (GTK_EDITABLE (name_entry), -1);
  g_signal_connect (name_entry, "changed", G_CALLBACK (control_save_button_visibility), save_button);

  tree = propono_bookmarks_tree_new ();
  propono_bookmarks_tree_select_entry  (PROPONO_BOOKMARKS_TREE (tree),
					propono_bookmarks_entry_get_parent (entry));
  gtk_box_pack_end (GTK_BOX (box), tree, TRUE, TRUE, 0);

  gtk_widget_show_all (dialog);
  while (TRUE)
    {
      ProponoBookmarksEntry *existing_entry;

      if (gtk_dialog_run (GTK_DIALOG (dialog)) != GTK_RESPONSE_OK)
	{
	  if (is_add)
	    g_object_unref (entry);
	  goto finalize;
	}

      name = gtk_entry_get_text (GTK_ENTRY (name_entry));
      if (strlen (name) < 1)
        {
	  propono_utils_show_error (NULL, _("Invalid name for this folder"), GTK_WINDOW (dialog));
	  gtk_widget_grab_focus (name_entry);
	  continue;
	}

      existing_entry = propono_bookmarks_name_exists (book,
						      propono_bookmarks_tree_get_selected_entry (PROPONO_BOOKMARKS_TREE (tree)),
						      name);
      if (existing_entry && existing_entry != entry)
	{
	  gchar *str = g_strdup_printf (_("The name \"%s\" is already used in this folder. Please use a different name."), name);
	  propono_utils_show_error (_("Invalid name for this item"),
				    str,
				    GTK_WINDOW (dialog));
	  g_free (str);
	  gtk_widget_grab_focus (name_entry);
	  continue;
	}

      break;
    }

  propono_bookmarks_entry_set_name (entry, name);
  if (!is_add)
    {
      g_object_ref (entry);
      propono_bookmarks_remove_entry (book, entry);
    }
  propono_bookmarks_add_entry ( book,
				entry,
				propono_bookmarks_tree_get_selected_entry (PROPONO_BOOKMARKS_TREE (tree)));

finalize:
  gtk_widget_destroy (GTK_WIDGET (dialog));
  g_object_unref (G_OBJECT (xml));
}

static void
show_dialog_conn (ProponoBookmarks      *book,
		  GtkWindow             *window,
		  ProponoBookmarksEntry *entry,
		  gboolean               is_add)
{
  gchar             *str, *host, *error_str, *protocol;
  gint               port;
  GtkBuilder        *xml;
  GtkWidget         *dialog, *host_entry, *name_entry, *fs_check;
  GtkWidget         *folder_box, *tree, *save_button, *plugin_box;
  GtkWidget         *plugin_options, *protocol_label;
  ProponoConnection *conn;
  const gchar       *name;
  ProponoPlugin     *plugin;
  gchar             **props;

  xml = propono_utils_get_builder (NULL, NULL);
  dialog         = GTK_WIDGET (gtk_builder_get_object (xml, "bookmarks_add_edit_conn_dialog"));
  name_entry     = GTK_WIDGET (gtk_builder_get_object (xml, "edit_bookmark_name_entry"));
  host_entry     = GTK_WIDGET (gtk_builder_get_object (xml, "edit_bookmark_host_entry"));
  fs_check       = GTK_WIDGET (gtk_builder_get_object (xml, "bookmark_fullscreen_check"));
  folder_box     = GTK_WIDGET (gtk_builder_get_object (xml, "folder_box"));
  plugin_box     = GTK_WIDGET (gtk_builder_get_object (xml, "plugin_options_vbox"));
  save_button    = GTK_WIDGET (gtk_builder_get_object (xml, "save_button"));
  protocol_label = GTK_WIDGET (gtk_builder_get_object (xml, "protocol_label"));

  gtk_window_set_transient_for (GTK_WINDOW (dialog), window);
  conn = propono_bookmarks_entry_get_conn (entry);

  str = propono_connection_get_best_name (conn);
  gtk_entry_set_text (GTK_ENTRY (name_entry), str);
  gtk_editable_set_position (GTK_EDITABLE (name_entry), -1);
  g_signal_connect (name_entry, "changed", G_CALLBACK (control_save_button_visibility), save_button);
  g_free (str);

  str = propono_connection_get_string_rep (conn, FALSE);
  gtk_entry_set_text (GTK_ENTRY (host_entry), str);
  g_free (str);

  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (fs_check),
				propono_connection_get_fullscreen (conn));

  plugin = propono_plugins_engine_get_plugin_by_protocol (propono_plugins_engine_get_default (),
							  propono_connection_get_protocol (conn));
  plugin_options = propono_plugin_get_connect_widget (plugin, conn);
  if (plugin_options)
    gtk_box_pack_start (GTK_BOX (plugin_box), plugin_options, TRUE, TRUE, 0);
  else
    gtk_widget_hide (plugin_box);

  props = propono_plugin_get_public_description (plugin);
  /* Translators: %s is a protocol name, like VNC or SSH */
  str = g_strdup_printf (_("(Protocol: %s)"), props[0]);
  gtk_label_set_label (GTK_LABEL (protocol_label), str);
  g_free (str);
  g_strfreev (props);

  tree = propono_bookmarks_tree_new ();
  propono_bookmarks_tree_select_entry  (PROPONO_BOOKMARKS_TREE (tree),
					propono_bookmarks_entry_get_parent (entry));
  gtk_box_pack_end (GTK_BOX (folder_box), tree, TRUE, TRUE, 0);

  gtk_widget_show_all (dialog);

  while (TRUE)
    {
      ProponoBookmarksEntry *existing_entry;

      if (gtk_dialog_run (GTK_DIALOG (dialog)) != GTK_RESPONSE_OK)
	goto finalize;

      name = gtk_entry_get_text (GTK_ENTRY (name_entry));
      if (strlen (name) < 1)
        {
	  propono_utils_show_error (NULL, _("Invalid name for this item"), GTK_WINDOW (dialog));
	  gtk_widget_grab_focus (name_entry);
	  continue;
	}

      existing_entry = propono_bookmarks_name_exists (book,
						      propono_bookmarks_tree_get_selected_entry (PROPONO_BOOKMARKS_TREE (tree)),
						      name);
      if (existing_entry && existing_entry != entry)
	{
	  str = g_strdup_printf (_("The name \"%s\" is already used in this folder. Please use a different name."), name);
	  propono_utils_show_error (_("Invalid name for this item"),
				    str,
				    GTK_WINDOW (dialog));
	  g_free (str);
	  gtk_widget_grab_focus (name_entry);
	  continue;
	}

      if (!propono_connection_split_string (gtk_entry_get_text (GTK_ENTRY (host_entry)),
					    propono_connection_get_protocol (conn),
					    &protocol,
					    &host,
					    &port,
					    &error_str))
        {
	  propono_utils_show_error (NULL, error_str, GTK_WINDOW (dialog));
	  g_free (error_str);
	  gtk_widget_grab_focus (host_entry);
	  continue;
	}

      break;
    }

  g_object_set (conn,
		"name", name,
		"host", host,
		"port", port,
		"fullscreen", gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (fs_check)),
		NULL);

  if (plugin_options)
    propono_connection_parse_options_widget (conn, plugin_options);

  g_free (protocol);
  g_free (host);

  if (!is_add)
    {
      g_object_ref (entry);
      propono_bookmarks_remove_entry (book, entry);
    }
  propono_bookmarks_add_entry ( book,
				entry,
				propono_bookmarks_tree_get_selected_entry (PROPONO_BOOKMARKS_TREE (tree)));

finalize:
  gtk_widget_destroy (GTK_WIDGET (dialog));
  g_object_unref (G_OBJECT (xml));
}

void
propono_bookmarks_add (ProponoBookmarks  *book,
                       ProponoConnection *conn,
		       GtkWindow         *window)
{
  ProponoBookmarksEntry *entry;

  g_return_if_fail (PROPONO_IS_BOOKMARKS (book));
  g_return_if_fail (PROPONO_IS_CONNECTION (conn));

  entry = propono_bookmarks_entry_new_conn (conn);
  show_dialog_conn (book, window, entry, TRUE);
}

void
propono_bookmarks_edit (ProponoBookmarks      *book,
                        ProponoBookmarksEntry *entry,
		        GtkWindow             *window)
{
  g_return_if_fail (PROPONO_IS_BOOKMARKS (book));
  g_return_if_fail (PROPONO_IS_BOOKMARKS_ENTRY (entry));

  g_object_ref (entry);

  switch (propono_bookmarks_entry_get_node (entry))
    {
      case PROPONO_BOOKMARKS_ENTRY_NODE_FOLDER:
	show_dialog_folder (book, window, entry, FALSE);
	break;

      case PROPONO_BOOKMARKS_ENTRY_NODE_CONN:
        show_dialog_conn (book, window, entry, FALSE);
        break;

      default:
	g_assert_not_reached ();
    }

  g_object_unref (entry);
}

void
propono_bookmarks_del (ProponoBookmarks      *book,
                       ProponoBookmarksEntry *entry,
		       GtkWindow             *window)
{
  GtkWidget *dialog;
  gchar     *name, *title, *msg1, *msg2;
  GError    *error = NULL;
  GSList    *parent;

  g_return_if_fail (PROPONO_IS_BOOKMARKS (book));
  g_return_if_fail (PROPONO_IS_BOOKMARKS_ENTRY (entry));

  name = g_strdup ("<i>%s</i>");
  /* Translators: %s is a bookmark entry name*/
  msg1 = g_strdup_printf (_("Are you sure you want to remove %s from bookmarks?"), name);
  g_free (name);

  if (propono_bookmarks_entry_get_node (entry) == PROPONO_BOOKMARKS_ENTRY_NODE_FOLDER)
    {
      name = g_strdup (propono_bookmarks_entry_get_name (entry));
      title = g_strdup (_("Remove Folder?"));
      msg2 = g_strdup_printf ("%s\n\n%s", msg1, _("Notice that all its subfolders and items will be removed as well."));
    }
  else
    {
      name = propono_connection_get_best_name (propono_bookmarks_entry_get_conn (entry));
      title = g_strdup (_("Remove Item?"));
      msg2 = g_strdup (msg1);
    }

  dialog = gtk_message_dialog_new (window,
				   GTK_DIALOG_MODAL | GTK_DIALOG_DESTROY_WITH_PARENT,
				   GTK_MESSAGE_QUESTION,
				   GTK_BUTTONS_OK_CANCEL,
				   "%s",
				   title);

  gtk_message_dialog_format_secondary_markup (GTK_MESSAGE_DIALOG (dialog),
					      msg2,
					      name);
 
  if (gtk_dialog_run (GTK_DIALOG (dialog)) == GTK_RESPONSE_OK)
    if (!propono_bookmarks_remove_entry (book, entry))
      g_warning (_("Error removing bookmark: Entry not found"));

  gtk_widget_destroy (dialog);
  g_free (name);
  g_free (title);
  g_free (msg1);
  g_free (msg2);
}

void
propono_bookmarks_new_folder (ProponoBookmarks *book,
			      GtkWindow        *window)
{
  ProponoBookmarksEntry *entry;

  g_return_if_fail (PROPONO_IS_BOOKMARKS (book));

  entry = propono_bookmarks_entry_new_folder (_("New Folder"));
  show_dialog_folder (book, window, entry, TRUE);
}

/* vim: set ts=8: */
