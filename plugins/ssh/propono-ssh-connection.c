/*
 * propono-ssh-connection.c
 * Child class of abstract ProponoConnection, specific to SSH protocol
 * This file is part of propono
 *
 * Copyright (C) 2009 - Jonh Wendell <wendell@bani.com.br>
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

#include <glib/gi18n.h>
#include <propono/propono-utils.h>
#include <propono/propono-cache-prefs.h>
#include "propono-ssh-connection.h"

struct _ProponoSshConnectionPrivate
{
  gint dummy;
};

#define PROPONO_SSH_CONNECTION_PRIVATE(o)  (G_TYPE_INSTANCE_GET_PRIVATE ((o), PROPONO_TYPE_SSH_CONNECTION, ProponoSshConnectionPrivate))
G_DEFINE_TYPE (ProponoSshConnection, propono_ssh_connection, PROPONO_TYPE_CONNECTION);

static void
propono_ssh_connection_init (ProponoSshConnection *conn)
{
  conn->priv = G_TYPE_INSTANCE_GET_PRIVATE (conn, PROPONO_TYPE_SSH_CONNECTION, ProponoSshConnectionPrivate);
}

static void
propono_ssh_connection_constructed (GObject *object)
{
  propono_connection_set_protocol (PROPONO_CONNECTION (object), "ssh");
}

static void
ssh_fill_writer (ProponoConnection *conn, xmlTextWriter *writer)
{
  PROPONO_CONNECTION_CLASS (propono_ssh_connection_parent_class)->impl_fill_writer (conn, writer);
}

static void
ssh_parse_item (ProponoConnection *conn, xmlNode *root)
{
  PROPONO_CONNECTION_CLASS (propono_ssh_connection_parent_class)->impl_parse_item (conn, root);
}

static void
ssh_parse_options_widget (ProponoConnection *conn, GtkWidget *widget)
{
  GtkWidget *u_entry;

  u_entry = g_object_get_data (G_OBJECT (widget), "username_entry");
  if (!u_entry)
    {
      g_warning ("Wrong widget passed to ssh_parse_options_widget()");
      return;
    }

  propono_cache_prefs_set_string  ("ssh-connection", "username", gtk_entry_get_text (GTK_ENTRY (u_entry)));

  g_object_set (conn,
		"username", gtk_entry_get_text (GTK_ENTRY (u_entry)),
		NULL);
}

static void
propono_ssh_connection_class_init (ProponoSshConnectionClass *klass)
{
  GObjectClass* object_class = G_OBJECT_CLASS (klass);
  ProponoConnectionClass* parent_class = PROPONO_CONNECTION_CLASS (klass);

  g_type_class_add_private (klass, sizeof (ProponoSshConnectionPrivate));

  object_class->constructed  = propono_ssh_connection_constructed;

  parent_class->impl_fill_writer = ssh_fill_writer;
  parent_class->impl_parse_item  = ssh_parse_item;
  parent_class->impl_parse_options_widget = ssh_parse_options_widget;
}

ProponoConnection *
propono_ssh_connection_new (void)
{
  return PROPONO_CONNECTION (g_object_new (PROPONO_TYPE_SSH_CONNECTION, NULL));
}

/* vim: set ts=8: */
