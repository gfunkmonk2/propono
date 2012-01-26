/*
 * propono-ssh-tab.c
 * SSH Implementation for ProponoSshTab widget
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
#include <vte/vte.h>
#include <gdk/gdkkeysyms.h>

#include <propono/propono-utils.h>
#include <propono/propono-prefs.h>

#include "propono-ssh-tab.h"
#include "propono-ssh-connection.h"

#define PROPONO_SSH_TAB_GET_PRIVATE(object)(G_TYPE_INSTANCE_GET_PRIVATE ((object), PROPONO_TYPE_SSH_TAB, ProponoSshTabPrivate))

struct _ProponoSshTabPrivate
{
  GtkWidget  *vte;
};

G_DEFINE_TYPE (ProponoSshTab, propono_ssh_tab, PROPONO_TYPE_TAB)

static gchar *
ssh_tab_get_tooltip (ProponoTab *tab)
{
  ProponoConnection *conn = propono_tab_get_conn (tab);

  return  g_markup_printf_escaped (
				  "<b>%s</b> %s\n"
				  "<b>%s</b> %d",
				  _("Host:"), propono_connection_get_host (conn),
				  _("Port:"), propono_connection_get_port (conn));
}

static GdkPixbuf *
ssh_tab_get_screenshot (ProponoTab *tab)
{
  return NULL;
}

static gboolean
emit_delayed_signal (GObject *object)
{
  ProponoSshTab *ssh_tab = PROPONO_SSH_TAB (object);

  g_signal_emit_by_name (object, "tab-initialized");
  gtk_widget_grab_focus (ssh_tab->priv->vte);
  return FALSE;
}

static void
propono_ssh_tab_constructed (GObject *object)
{
  gchar **arg;
  const gchar *username;
  gint i;
  ProponoSshTab *ssh_tab = PROPONO_SSH_TAB (object);
  ProponoTab    *tab = PROPONO_TAB (object);
  ProponoConnection *conn = propono_tab_get_conn (tab);

  if (G_OBJECT_CLASS (propono_ssh_tab_parent_class)->constructed)
    G_OBJECT_CLASS (propono_ssh_tab_parent_class)->constructed (object);

  username = propono_connection_get_username (conn);
  i = 0;

  arg = g_new (gchar *, 7);
  arg[i++] = g_strdup ("ssh");

  if (username && *username)
    {
      arg[i++] = g_strdup ("-l");
      arg[i++] = g_strdup (username);
    }

  arg[i++] = g_strdup ("-p");
  arg[i++] = g_strdup_printf ("%d", propono_connection_get_port (conn));

  arg[i++] = g_strdup (propono_connection_get_host (conn));
  arg[i++] = NULL;

  vte_terminal_fork_command (VTE_TERMINAL (ssh_tab->priv->vte),
			     "ssh",
			     arg,
			     NULL,
			     NULL,
			     FALSE,
			     FALSE,
			     FALSE);
  g_strfreev (arg);
  gtk_widget_show_all (GTK_WIDGET (ssh_tab));

  propono_tab_add_recent_used (tab);
  propono_tab_set_state (tab, PROPONO_TAB_STATE_CONNECTED);
  g_idle_add ((GSourceFunc)emit_delayed_signal, object);
}

static void 
propono_ssh_tab_class_init (ProponoSshTabClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);
  ProponoTabClass* tab_class = PROPONO_TAB_CLASS (klass);

  object_class->constructed = propono_ssh_tab_constructed;

  tab_class->impl_get_tooltip = ssh_tab_get_tooltip;
  tab_class->impl_get_screenshot = ssh_tab_get_screenshot;

  g_type_class_add_private (object_class, sizeof (ProponoSshTabPrivate));
}

static void
ssh_disconnected_cb (VteTerminal *ssh, ProponoSshTab *tab)
{
  g_signal_emit_by_name (G_OBJECT (tab), "tab-disconnected");
}

static void
propono_ssh_tab_init (ProponoSshTab *ssh_tab)
{
  ssh_tab->priv = PROPONO_SSH_TAB_GET_PRIVATE (ssh_tab);

  /* Create the ssh widget */
  ssh_tab->priv->vte = vte_terminal_new ();
  propono_tab_add_view (PROPONO_TAB (ssh_tab), ssh_tab->priv->vte);

  g_signal_connect (ssh_tab->priv->vte,
		    "child-exited",
		    G_CALLBACK (ssh_disconnected_cb),
		    ssh_tab);
}

GtkWidget *
propono_ssh_tab_new (ProponoConnection *conn,
		     ProponoWindow     *window)
{
  return GTK_WIDGET (g_object_new (PROPONO_TYPE_SSH_TAB,
				   "conn", conn,
				   "window", window,
				   NULL));
}

/* vim: set ts=8: */
