/*
 * propono-vnc-plugin.c
 * This file is part of propono
 *
 * Copyright (C) 2009 Jonh Wendell <wendell@bani.com.br>
 * 
 * propono-vnc-plugin.c is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation, either version 2 of the License, or
 * (at your option) any later version.
 * 
 * propono-vnc-plugin.c is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License along
 * with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <string.h>
#include <glib/gi18n-lib.h>
#include <gmodule.h>
#include <vncdisplay.h>

#include <propono/propono-debug.h>
#include <propono/propono-prefs.h>
#include <propono/propono-cache-prefs.h>

#include "propono-vnc-plugin.h"
#include "propono-vnc-connection.h"
#include "propono-vnc-tab.h"
#include "propono-vnc-listener-dialog.h"
#include "propono-vnc-listener.h"

#define PROPONO_VNC_PLUGIN_GET_PRIVATE(object)(G_TYPE_INSTANCE_GET_PRIVATE ((object), PROPONO_TYPE_VNC_PLUGIN, ProponoVncPluginPrivate))
#define WINDOW_DATA_KEY "ProponoVNCPluginWindowData"

PROPONO_PLUGIN_REGISTER_TYPE(ProponoVncPlugin, propono_vnc_plugin)

typedef struct
{
  GtkActionGroup *ui_action_group;
  guint ui_id;
} WindowData;

typedef struct
{
  ProponoPlugin *plugin;
  ProponoWindow *window;
} ActionData;

static void
free_window_data (WindowData *data)
{
  g_return_if_fail (data != NULL);

  g_object_unref (data->ui_action_group);
  g_slice_free (WindowData, data);
}

static void
free_action_data (ActionData *data)
{
  g_return_if_fail (data != NULL);

  g_slice_free (ActionData, data);
}

static void
listening_cb (GtkAction *action, ActionData *action_data)
{
  propono_vnc_listener_dialog_show (action_data->window, action_data->plugin);
}

static GtkActionEntry action_entries[] =
{
  { "VNCListener",
    NULL,
    /* Translators: "Reverse" here is an adjective, not a verb. */
    N_("_Reverse Connections..."),
    NULL,
    N_("Configure incoming VNC connections"),
    G_CALLBACK (listening_cb)
  }
};

static void
impl_activate (ProponoPlugin *plugin,
               ProponoWindow *window)
{
  GtkActionGroup *action_group;
  GtkUIManager *manager;
  WindowData *data;
  ActionData *action_data;
  gboolean always;

  propono_debug_message (DEBUG_PLUGINS, "ProponoVncPlugin Activate");

  data = g_slice_new (WindowData);
  action_data = g_slice_new (ActionData);
  action_data->window = window;
  action_data->plugin = plugin;

  action_group = propono_window_get_always_sensitive_action (window);
  manager = propono_window_get_ui_manager (window);

  data->ui_action_group = gtk_action_group_new ("ProponoVNCPluginActions");
  gtk_action_group_set_translation_domain (data->ui_action_group, GETTEXT_PACKAGE);
  gtk_action_group_add_actions_full (data->ui_action_group,
				     action_entries,
				     G_N_ELEMENTS (action_entries),
				     action_data,
				     (GDestroyNotify) free_action_data);
  gtk_ui_manager_insert_action_group (manager,
				      data->ui_action_group,
				      -1);

  data->ui_id = gtk_ui_manager_new_merge_id (manager);
  gtk_ui_manager_add_ui (manager,
			 data->ui_id,
			 "/MenuBar/MachineMenu/MachineOps_1",
			 "VNCListener",
			 "VNCListener",
			 GTK_UI_MANAGER_AUTO,
			 TRUE);

  g_object_set_data_full (G_OBJECT (window),
			  WINDOW_DATA_KEY,
			  data,
			  (GDestroyNotify) free_window_data);

  g_object_get (propono_prefs_get_default (),
		"always-enable-listening", &always,
		NULL);
  if (always)
    propono_vnc_listener_start (propono_vnc_listener_get_default ());
}

static void
impl_deactivate  (ProponoPlugin *plugin,
                  ProponoWindow *window)
{
  GtkUIManager *manager;
  WindowData *data;

  propono_debug_message (DEBUG_PLUGINS, "ProponoVncPlugin Deactivate");

  manager = propono_window_get_ui_manager (window);
  data = (WindowData *) g_object_get_data (G_OBJECT (window), WINDOW_DATA_KEY);
  g_return_if_fail (data != NULL);

  gtk_ui_manager_remove_ui (manager, data->ui_id);
  gtk_ui_manager_remove_action_group (manager, data->ui_action_group);

  g_object_set_data (G_OBJECT (window), WINDOW_DATA_KEY, NULL);
}

static void
impl_update_ui (ProponoPlugin *plugin,
                ProponoWindow *window)
{
  propono_debug_message (DEBUG_PLUGINS, "ProponoVncPlugin Update UI");
}

static const GOptionEntry propono_vnc_args[] =
{
  { "vnc-scale", 0, 0, G_OPTION_ARG_NONE, &scaling_command_line,
  /* Translators: this is a command line option (run propono --help) */
  N_("Enable scaled mode"), 0 },
  { NULL }
};

static GSList *
impl_get_context_groups (ProponoPlugin *plugin)
{
  GOptionGroup *group;
  GSList       *groups = NULL;

  propono_debug_message (DEBUG_PLUGINS, "ProponoVncPlugin Get Context Group");

  scaling_command_line = FALSE;
  group = g_option_group_new ("vnc",
			      /* Translators: this is a command line option (run propono --help) */
			      _("VNC Options:"),
			      /* Translators: this is a command line option (run propono --help) */
			      _("Show VNC Options"),
			      NULL,
			      NULL);
  g_option_group_add_entries (group, propono_vnc_args);

  groups = g_slist_append (groups, group);
  groups = g_slist_append (groups, vnc_display_get_option_group ());

  return groups;
}

static const gchar *
impl_get_protocol (ProponoPlugin *plugin)
{
  return "vnc";
}

static gchar **
impl_get_public_description (ProponoPlugin *plugin)
{
  gchar **result = g_new (gchar *, 3);

  result[0] = g_strdup (_("VNC"));
  result[1] = g_strdup (_("Access Unix/Linux, Windows and other machines."));
  result[2] = NULL;

  return result;
}

static const gchar *
impl_get_mdns_service (ProponoPlugin *plugin)
{
  return "_rfb._tcp";
}

static ProponoConnection *
impl_new_connection (ProponoPlugin *plugin)
{
  ProponoConnection *conn;

  conn = propono_vnc_connection_new ();
  propono_vnc_connection_set_scaling (PROPONO_VNC_CONNECTION (conn),
				      scaling_command_line);

  return conn;
}

static ProponoConnection *
impl_new_connection_from_file (ProponoPlugin *plugin,
			       const gchar   *data,
			       gboolean       use_bookmarks,
			       gchar        **error_msg)
{
  GKeyFile          *file;
  GError            *error;
  gboolean           loaded;
  gchar             *host, *actual_host, *protocol;
  gint               port;
  ProponoConnection *conn;

  *error_msg = NULL;
  conn = NULL;
  host = NULL;
  protocol = NULL;
  error = NULL;

  file = g_key_file_new ();
  loaded = g_key_file_load_from_data (file,
				      data,
				      -1,
				      0,
				      &error);
  if (!loaded)
    {
      if (error)
	{
	  *error_msg = g_strdup (error->message);
	  g_error_free (error);
	}
      else
	*error_msg = g_strdup (_("Could not parse the file."));

      goto the_end;
    }

  if (!g_key_file_has_group (file, "connection"))
    {
      /* Translators: Do not translate "connection". It's the name of a group in the .vnc (.ini like) file. */
      *error_msg = g_strdup (_("The file is not a VNC one: Missing the group \"connection\"."));
      goto the_end;
    }

  if (!g_key_file_has_key (file, "connection", "host", NULL))
    {
      /* Translators: Do not translate "host". It's the name of a key in the .vnc (.ini like) file. */
      *error_msg = g_strdup (_("The file is not a VNC one: Missing the key \"host\"."));
      goto the_end;
    }

  host = g_key_file_get_string (file, "connection", "host", NULL);
  port = g_key_file_get_integer (file, "connection", "port", NULL);
  if (!port)
    {
      if (!propono_connection_split_string (host, "vnc", &protocol, &actual_host, &port, error_msg))
	goto the_end;

      g_free (host);
      host = actual_host;
    }

  if (use_bookmarks)
    conn = propono_bookmarks_exists (propono_bookmarks_get_default (), "vnc", host, port);
  if (!conn)
    {
      gchar *s_value;
      gint shared;

      conn = propono_vnc_connection_new ();
      propono_connection_set_host (conn, host);
      propono_connection_set_port (conn, port);

      s_value = g_key_file_get_string  (file, "connection", "username", NULL);
      propono_connection_set_username (conn, s_value);
      g_free (s_value);

      s_value = g_key_file_get_string  (file, "connection", "password", NULL);
      propono_connection_set_password (conn, s_value);
      g_free (s_value);

      shared = g_key_file_get_integer (file, "options", "shared", NULL);
      if (shared == 0 || shared == 1)
	propono_vnc_connection_set_shared (PROPONO_VNC_CONNECTION (conn), shared);
      else
        /* Translators: 'shared' here is a VNC protocol specific flag. You can translate it, but I think it's better to let it untranslated */
	g_message (_("Bad value for 'shared' flag: %d. It is supposed to be 0 or 1. Ignoring it."), shared);
    }

the_end:

  g_free (host);
  g_free (protocol);
  g_key_file_free (file);
  return conn;

}

static GtkWidget *
impl_new_tab (ProponoPlugin *plugin,
	      ProponoConnection *conn,
	      ProponoWindow     *window)
{
  return propono_vnc_tab_new (conn, window);
}

static void
ssh_tunnel_check_toggled_cb (GtkToggleButton *button, GObject *box)
{
  gboolean active = gtk_toggle_button_get_active (button);
  GtkWidget *ssh_host_entry = g_object_get_data (G_OBJECT (box), "ssh_host");

  gtk_widget_set_sensitive (ssh_host_entry, active);

  if (active)
    gtk_widget_grab_focus (ssh_host_entry);
  else
    gtk_entry_set_text (GTK_ENTRY (ssh_host_entry), "");
}

static void
scaling_check_toggled_cb (GtkToggleButton *button, GObject *box)
{
  gboolean active = gtk_toggle_button_get_active (button);
  GtkWidget *ratio = g_object_get_data (G_OBJECT (box), "ratio");

  gtk_widget_set_sensitive (ratio, active);
}

static GtkWidget *
impl_get_connect_widget (ProponoPlugin *plugin, ProponoConnection *conn)
{
  GtkWidget *box, *check, *label, *combo, *box2, *ssh_host_entry;
  GtkTable  *table;
  gchar     *str, *ssh_host;
  gboolean has_conn = PROPONO_IS_VNC_CONNECTION (conn), active;

  box = gtk_vbox_new (FALSE, 0);

  str = g_strdup_printf ("<b>%s</b>", _("VNC Options"));
  label = gtk_label_new (str);
  g_free (str);
  gtk_label_set_use_markup (GTK_LABEL (label), TRUE);
  gtk_misc_set_alignment (GTK_MISC (label), 0, 0.5);
  gtk_misc_set_padding (GTK_MISC (label), 0, 6);
  gtk_box_pack_start (GTK_BOX (box), label, FALSE, FALSE, 0);

  table = GTK_TABLE (gtk_table_new (6, 2, FALSE));
  label = gtk_label_new ("  ");
  gtk_table_attach (table, label, 0, 1, 0, 1, GTK_SHRINK, GTK_SHRINK, 0, 0);

  /* View only check button */
  check = gtk_check_button_new_with_mnemonic (_("_View only"));
  g_object_set_data (G_OBJECT (box), "view_only", check);
  gtk_table_attach_defaults (table, check, 1, 2, 0, 1);
  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (check),
				has_conn ? propono_vnc_connection_get_view_only (PROPONO_VNC_CONNECTION (conn))
				: propono_cache_prefs_get_boolean ("vnc-connection", "view-only", FALSE));

  /* Scaling check button */
  check = gtk_check_button_new_with_mnemonic (_("_Scaling"));
  g_object_set_data (G_OBJECT (box), "scaling", check);
  gtk_table_attach_defaults (table, check, 1, 2, 1, 2);
  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (check),
				has_conn ? propono_vnc_connection_get_scaling (PROPONO_VNC_CONNECTION (conn))
				: propono_cache_prefs_get_boolean ("vnc-connection", "scaling", FALSE));
  active = gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (check));
  g_signal_connect (check,
		    "toggled",
		    G_CALLBACK (scaling_check_toggled_cb),
		    box);

  /* Keep ratio check button */
  box2 = gtk_hbox_new (FALSE, 4);
  label = gtk_label_new ("   ");
  gtk_box_pack_start (GTK_BOX (box2), GTK_WIDGET (label), FALSE, FALSE, 0);
  check = gtk_check_button_new_with_mnemonic (_("_Keep aspect ratio"));
  gtk_box_pack_start (GTK_BOX (box2), check, TRUE, TRUE, 0);
  g_object_set_data (G_OBJECT (box), "ratio", check);
  gtk_table_attach_defaults (table, box2, 1, 2, 2, 3);
  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (check),
				has_conn ? propono_vnc_connection_get_keep_ratio (PROPONO_VNC_CONNECTION (conn))
				: propono_cache_prefs_get_boolean ("vnc-connection", "keep-ratio", TRUE));
  gtk_widget_set_sensitive (check, active);

  /* JPEG Compression check button */
  check = gtk_check_button_new_with_mnemonic (_("_Use JPEG Compression"));
  gtk_widget_set_tooltip_text (check, _("This might not work on all VNC servers"));
  g_object_set_data (G_OBJECT (box), "lossy", check);
  gtk_table_attach_defaults (table, check, 1, 2, 3, 4);
  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (check),
				has_conn ? propono_vnc_connection_get_lossy_encoding (PROPONO_VNC_CONNECTION (conn))
				: propono_cache_prefs_get_boolean ("vnc-connection", "lossy-encoding", FALSE));

  /* Depth color combo box */
  box2 = gtk_hbox_new (FALSE, 4);
  label = gtk_label_new_with_mnemonic (_("_Depth Color:"));
  gtk_misc_set_alignment (GTK_MISC (label), 0, 0.5);
  gtk_box_pack_start (GTK_BOX (box2), GTK_WIDGET (label), FALSE, FALSE, 0);

  combo = gtk_combo_box_new_text ();
  gtk_combo_box_append_text (GTK_COMBO_BOX (combo), _("Use Server Settings"));
  gtk_combo_box_append_text (GTK_COMBO_BOX (combo), _("True Color (24 bits)"));
  gtk_combo_box_append_text (GTK_COMBO_BOX (combo), _("High Color (16 bits)"));
  gtk_combo_box_append_text (GTK_COMBO_BOX (combo), _("Low Color (8 bits)"));
  gtk_combo_box_append_text (GTK_COMBO_BOX (combo), _("Ultra Low Color (3 bits)"));
  gtk_combo_box_set_active (GTK_COMBO_BOX (combo),
			    has_conn ? propono_vnc_connection_get_depth_profile (PROPONO_VNC_CONNECTION (conn))
			    : propono_cache_prefs_get_integer ("vnc-connection", "depth-profile", 0));
  g_object_set_data (G_OBJECT (box), "depth_combo", combo);
  gtk_label_set_mnemonic_widget (GTK_LABEL (label), combo);
  gtk_box_pack_start (GTK_BOX (box2), GTK_WIDGET (combo), FALSE, FALSE, 0);
  gtk_table_attach_defaults (table, box2, 1, 2, 4, 5);

  /* SSH Tunneling */
  box2 = gtk_hbox_new (FALSE, 4);

  /* Translators: the whole sentence will be: Use host <hostname> as a SSH tunnel*/
  check = gtk_check_button_new_with_mnemonic (_("Use h_ost"));
  g_object_set_data (G_OBJECT (box), "use_ssh", check);
  gtk_box_pack_start (GTK_BOX (box2), check, FALSE, FALSE, 0);

  ssh_host_entry = gtk_entry_new ();
  gtk_widget_set_sensitive (ssh_host_entry, FALSE);
  g_object_set_data (G_OBJECT (box), "ssh_host", ssh_host_entry);
  /* Translators: This is the tooltip of the SSH tunneling entry */
  str = g_strdup_printf ("%s\n%s\n%s",
			 _("hostname or user@hostname"),
			 _("Supply an alternative port using colon"),
			 _("For instance: joe@domain.com:5022"));
  gtk_widget_set_tooltip_text (ssh_host_entry, str);
  g_free (str);
  gtk_box_pack_start (GTK_BOX (box2), ssh_host_entry, FALSE, FALSE, 0);

  /* Translators: the whole sentence will be: Use host <hostname> as a SSH tunnel*/
  label = gtk_label_new_with_mnemonic (_("as a SSH tunnel"));
  gtk_misc_set_alignment (GTK_MISC (label), 0, 0.5);
  gtk_box_pack_start (GTK_BOX (box2), label, FALSE, FALSE, 0);

  g_signal_connect (check,
		    "toggled",
		    G_CALLBACK (ssh_tunnel_check_toggled_cb),
		    box);

  ssh_host = has_conn ? g_strdup (propono_vnc_connection_get_ssh_tunnel_host (PROPONO_VNC_CONNECTION (conn)))
                      : propono_cache_prefs_get_string  ("vnc-connection", "ssh-tunnel-host", NULL);
  if (ssh_host)
    gtk_entry_set_text (GTK_ENTRY (ssh_host_entry), ssh_host);
  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (check), ssh_host && *ssh_host);
  g_free (ssh_host);

  gtk_table_attach_defaults (table, box2, 1, 2, 5, 6);

  gtk_box_pack_start (GTK_BOX (box), GTK_WIDGET (table), FALSE, FALSE, 0);
  return box;
}

static gint
impl_get_default_port (ProponoPlugin *plugin)
{
  return 5900;
}

static GtkFileFilter *
impl_get_file_filter (ProponoPlugin *plugin)
{
  GtkFileFilter *filter;

  filter = gtk_file_filter_new ();
  /* Translators: this is a pattern to open *.vnc files in a open dialog. */
  gtk_file_filter_set_name (filter, _("VNC Files"));
  gtk_file_filter_add_pattern (filter, "*.vnc");

  return filter;
}

static void
propono_vnc_plugin_init (ProponoVncPlugin *plugin)
{
  propono_debug_message (DEBUG_PLUGINS, "ProponoVncPlugin initializing");
}

static void
propono_vnc_plugin_finalize (GObject *object)
{
  propono_debug_message (DEBUG_PLUGINS, "ProponoVncPlugin finalizing");

  G_OBJECT_CLASS (propono_vnc_plugin_parent_class)->finalize (object);
}

static void
propono_vnc_plugin_class_init (ProponoVncPluginClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);
  ProponoPluginClass *plugin_class = PROPONO_PLUGIN_CLASS (klass);

  object_class->finalize   = propono_vnc_plugin_finalize;

  plugin_class->activate   = impl_activate;
  plugin_class->deactivate = impl_deactivate;
  plugin_class->update_ui  = impl_update_ui;
  plugin_class->get_context_groups = impl_get_context_groups;
  plugin_class->get_protocol  = impl_get_protocol;
  plugin_class->get_public_description  = impl_get_public_description;
  plugin_class->new_connection = impl_new_connection;
  plugin_class->new_connection_from_file = impl_new_connection_from_file;
  plugin_class->get_mdns_service  = impl_get_mdns_service;
  plugin_class->new_tab = impl_new_tab;
  plugin_class->get_connect_widget = impl_get_connect_widget;
  plugin_class->get_default_port = impl_get_default_port;
  plugin_class->get_file_filter = impl_get_file_filter;
}
/* vim: set ts=8: */
