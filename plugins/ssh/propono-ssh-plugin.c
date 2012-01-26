/*
 * propono-ssh-plugin.c
 * This file is part of propono
 *
 * Copyright (C) 2009 Jonh Wendell <wendell@bani.com.br>
 * 
 * propono-ssh-plugin.c is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation, either version 2 of the License, or
 * (at your option) any later version.
 * 
 * propono-ssh-plugin.c is distributed in the hope that it will be useful, but
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

#include <glib/gi18n-lib.h>
#include <gmodule.h>

#include <propono/propono-debug.h>
#include <propono/propono-cache-prefs.h>

#include "propono-ssh-plugin.h"
#include "propono-ssh-connection.h"
#include "propono-ssh-tab.h"

#ifdef PROPONO_ENABLE_AVAHI
#include <avahi-ui/avahi-ui.h>
#include <avahi-common/malloc.h>
#endif

#define PROPONO_SSH_PLUGIN_GET_PRIVATE(object)(G_TYPE_INSTANCE_GET_PRIVATE ((object), PROPONO_TYPE_SSH_PLUGIN, ProponoSshPluginPrivate))

PROPONO_PLUGIN_REGISTER_TYPE(ProponoSshPlugin, propono_ssh_plugin)

static void
impl_activate (ProponoPlugin *plugin,
               ProponoWindow *window)
{
  propono_debug_message (DEBUG_PLUGINS, "ProponoSshPlugin Activate");
}

static void
impl_deactivate  (ProponoPlugin *plugin,
                  ProponoWindow *window)
{
  propono_debug_message (DEBUG_PLUGINS, "ProponoSshPlugin Deactivate");
}

static void
impl_update_ui (ProponoPlugin *plugin,
                ProponoWindow *window)
{
  propono_debug_message (DEBUG_PLUGINS, "ProponoSshPlugin Update UI");
}

static const gchar *
impl_get_protocol (ProponoPlugin *plugin)
{
  return "ssh";
}

static gchar **
impl_get_public_description (ProponoPlugin *plugin)
{
  gchar **result = g_new (gchar *, 3);

  result[0] = g_strdup (_("SSH"));
  /* Translators: This is a description of the SSH protocol. It appears at Connect dialog. */
  result[1] = g_strdup (_("Access Unix/Linux terminals"));
  result[2] = NULL;

  return result;
}

static const gchar *
impl_get_mdns_service (ProponoPlugin *plugin)
{
  return "_ssh._tcp";
}

static ProponoConnection *
impl_new_connection (ProponoPlugin *plugin)
{
  return propono_ssh_connection_new ();
}

static GtkWidget *
impl_new_tab (ProponoPlugin     *plugin,
	      ProponoConnection *conn,
	      ProponoWindow     *window)
{
  return propono_ssh_tab_new (conn, window);
}

static gint
impl_get_default_port (ProponoPlugin *plugin)
{
  return 22;
}

static void
propono_ssh_plugin_init (ProponoSshPlugin *plugin)
{
  propono_debug_message (DEBUG_PLUGINS, "ProponoSshPlugin initializing");
}

static void
propono_ssh_plugin_finalize (GObject *object)
{
  propono_debug_message (DEBUG_PLUGINS, "ProponoSshPlugin finalizing");

  G_OBJECT_CLASS (propono_ssh_plugin_parent_class)->finalize (object);
}

static GtkWidget *
impl_get_connect_widget (ProponoPlugin *plugin, ProponoConnection *conn)
{
  GtkWidget *box, *label, *u_box, *u_entry;
  gchar     *str;

  box = gtk_vbox_new (FALSE, 0);

  str = g_strdup_printf ("<b>%s</b>", _("SSH Options"));
  label = gtk_label_new (str);
  g_free (str);
  gtk_label_set_use_markup (GTK_LABEL (label), TRUE);
  gtk_misc_set_alignment (GTK_MISC (label), 0, 0.5);
  gtk_misc_set_padding (GTK_MISC (label), 0, 6);
  gtk_box_pack_start (GTK_BOX (box), label, FALSE, FALSE, 0);

  u_box = gtk_hbox_new (FALSE, 4);
  label = gtk_label_new ("  ");
  gtk_box_pack_start (GTK_BOX (u_box), label, FALSE, FALSE, 0);

  label = gtk_label_new_with_mnemonic (_("_Username:"));
  gtk_box_pack_start (GTK_BOX (u_box), label, FALSE, FALSE, 0);

  u_entry = gtk_entry_new ();
  /* Translators: This is the tooltip for the username field in a SSH connection */
  gtk_widget_set_tooltip_text (u_entry, _("Optional. If blank, your username will be used. Also, it can be supplied in the Machine field above, in the form username@hostname."));
  g_object_set_data (G_OBJECT (box), "username_entry", u_entry);
  gtk_box_pack_start (GTK_BOX (u_box), u_entry, TRUE, TRUE, 5);
  gtk_label_set_mnemonic_widget (GTK_LABEL (label), u_entry);
  str = g_strdup (PROPONO_IS_CONNECTION (conn) ?
		  propono_connection_get_username (conn) :
		  propono_cache_prefs_get_string  ("ssh-connection", "username", ""));
  gtk_entry_set_text (GTK_ENTRY (u_entry), str);
  gtk_entry_set_activates_default (GTK_ENTRY (u_entry), TRUE);
  g_free (str);

  gtk_box_pack_start (GTK_BOX (box), u_box, TRUE, TRUE, 0);
  return box;
}

static void
ssh_parse_mdns_dialog (ProponoPlugin *plugin,
		       GtkWidget *connect_widget,
		       GtkWidget *dialog)
{
#ifdef PROPONO_ENABLE_AVAHI
  const AvahiStringList *txt;
  gchar *u = NULL;

  for (txt = aui_service_dialog_get_txt_data (AUI_SERVICE_DIALOG (dialog)); txt; txt = txt->next)
    {
      char *key, *value;

      if (avahi_string_list_get_pair ((AvahiStringList*) txt, &key, &value, NULL) < 0)
	break;

      if (strcmp(key, "u") == 0)
	u = g_strdup(value);

      avahi_free (key);
      avahi_free (value);
    }

  if (u)
    {
      GtkEntry *u_entry = g_object_get_data (G_OBJECT (connect_widget), "username_entry");

      if (u_entry)
        gtk_entry_set_text (u_entry, u);
      else
	g_warning ("Wrong widget passed to ssh_parse_mdns_dialog()");

      g_free (u);
    }
#endif
}

static void
propono_ssh_plugin_class_init (ProponoSshPluginClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);
  ProponoPluginClass *plugin_class = PROPONO_PLUGIN_CLASS (klass);

  object_class->finalize   = propono_ssh_plugin_finalize;

  plugin_class->activate   = impl_activate;
  plugin_class->deactivate = impl_deactivate;
  plugin_class->update_ui  = impl_update_ui;
  plugin_class->get_protocol  = impl_get_protocol;
  plugin_class->get_public_description  = impl_get_public_description;
  plugin_class->new_connection = impl_new_connection;
  plugin_class->get_mdns_service  = impl_get_mdns_service;
  plugin_class->new_tab = impl_new_tab;
  plugin_class->get_default_port = impl_get_default_port;
  plugin_class->get_connect_widget = impl_get_connect_widget;
  plugin_class->parse_mdns_dialog = ssh_parse_mdns_dialog;
}
/* vim: set ts=8: */
