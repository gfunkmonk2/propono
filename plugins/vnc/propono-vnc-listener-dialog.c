/*
 * propono-vnc-listener-dialog.c
 * This file is part of propono
 *
 * Copyright (C) 2009 Jonh Wendell <wendell@bani.com.br>
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

#include <config.h>

#include <net/if.h>
#include <arpa/inet.h>
#include <netinet/in.h>

#ifdef HAVE_IFADDRS_H
#include <ifaddrs.h>
#else
#include "if/ifaddrs.h"
#endif

#include <string.h>
#include <glib/gi18n.h>

#include <propono/propono-utils.h>
#include <propono/propono-prefs.h>
#include "propono-vnc-listener-dialog.h"
#include "propono-vnc-listener.h"

typedef struct
{
  GtkBuilder *xml;
  GtkWidget *dialog;
  GtkWidget *enable_reverse_check;
  GtkWidget *always_enabled_check;
  GtkWidget *port_label;
  GtkWidget *connectivity_exp;
  GtkTextBuffer *ip_buffer;
} VncListenDialog;

static void
setup_ip_buffer (VncListenDialog *dialog)
{
  char            buf[INET6_ADDRSTRLEN], *dup;
  struct ifaddrs  *myaddrs, *ifa;
  void            *sin;
  GArray          *ipv4, *ipv6;
  GString         *str;
  int             i;

  ipv4 = g_array_new (FALSE, TRUE, sizeof (char *));
  ipv6 = g_array_new (FALSE, TRUE, sizeof (char *));
  str = g_string_new (NULL);

  getifaddrs (&myaddrs);
  for (ifa = myaddrs; ifa != NULL; ifa = ifa->ifa_next)
    {
      if (ifa->ifa_addr == NULL || ifa->ifa_name == NULL || (ifa->ifa_flags & IFF_UP) == 0 || strncmp (ifa->ifa_name, "lo", 2) == 0)
	continue;

      switch (ifa->ifa_addr->sa_family)
	{
	  case AF_INET:
	    sin = &((struct sockaddr_in *)ifa->ifa_addr)->sin_addr;
	    inet_ntop (AF_INET, sin, buf, INET6_ADDRSTRLEN);
	    dup = g_strdup (buf);
	    g_array_append_val (ipv4, dup);
	    break;

	  case AF_INET6:
	    sin = &((struct sockaddr_in6 *)ifa->ifa_addr)->sin6_addr;
	    inet_ntop (AF_INET6, sin, buf, INET6_ADDRSTRLEN);
	    dup = g_strdup (buf);
	    g_array_append_val (ipv6, dup);
	    break;
	  default: continue;
	}
    }

  if (ipv4->len > 0)
    {
      if (ipv6->len > 0)
        g_string_append (str, _("IPv4:"));

      for (i = 0; i < ipv4->len; i++)
	{
	  dup = g_array_index (ipv4, char *, i);
	  g_string_append_printf (str, "\n%s", dup);
	  g_free (dup);
	}
    }
  if (ipv6->len > 0)
    {
      if (ipv4->len > 0)
        g_string_append (str, _("\n\nIPv6:"));

      for (i = 0; i < ipv6->len; i++)
	{
	  dup = g_array_index (ipv6, char *, i);
	  g_string_append_printf (str, "\n%s", g_array_index (ipv6, char *, i));
	  g_free (dup);
	}
    }

  gtk_text_buffer_set_text (dialog->ip_buffer, str->str, -1);

  freeifaddrs (myaddrs);
  g_array_free (ipv4, TRUE);
  g_array_free (ipv6, TRUE);
  g_string_free (str, TRUE);
}

static void
dialog_destroy (GtkObject *obj,
		VncListenDialog *dialog)
{
  g_object_unref (dialog->xml);
  g_slice_free (VncListenDialog, dialog);
}

static void
dialog_response_handler (GtkDialog       *widget,
			 gint            res_id,
			 VncListenDialog *dialog)
{
  switch (res_id)
    {
      case GTK_RESPONSE_HELP:
	// TODO: display the help
	break;

      default:
	gtk_widget_destroy (dialog->dialog);
	break;
    }
}

static void
update_ui_sensitivity (VncListenDialog *dialog)
{
  gboolean listening;
  gchar *str;
  ProponoVncListener *listener = propono_vnc_listener_get_default ();

  listening = propono_vnc_listener_is_listening (listener);

  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (dialog->enable_reverse_check),
				listening);
  gtk_widget_set_sensitive (dialog->always_enabled_check, listening);
  gtk_widget_set_sensitive (dialog->connectivity_exp, listening);

  if (listening)
    {
      str = g_strdup_printf (_("On the port %d"), propono_vnc_listener_get_port (listener));
      gtk_label_set_label (GTK_LABEL (dialog->port_label), str);
      g_free (str);
    }
  else
    {
      gtk_expander_set_expanded (GTK_EXPANDER (dialog->connectivity_exp), FALSE);
    }
}

static void
enable_reverse_toggled_cb (GtkToggleButton *button, VncListenDialog *dialog)
{
  ProponoVncListener *listener = propono_vnc_listener_get_default ();

  if (gtk_toggle_button_get_active (button))
    propono_vnc_listener_start (listener);
  else
    {
      propono_vnc_listener_stop (listener);
      gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (dialog->always_enabled_check),
				    FALSE);
    }

  update_ui_sensitivity (dialog);
}

static void
always_enabled_toggled_cb (GtkToggleButton *button, VncListenDialog *dialog)
{
  g_object_set (propono_prefs_get_default (),
		"always-enable-listening", gtk_toggle_button_get_active (button),
		NULL);
}

void
propono_vnc_listener_dialog_show (ProponoWindow *parent, ProponoPlugin *plugin)
{
  VncListenDialog *dialog;
  GtkBuilder *xml;
  gboolean always;

  xml = propono_utils_get_builder (plugin, "vnc.ui");
  if (!xml)
    return;

  dialog = g_slice_new (VncListenDialog);
  dialog->xml = xml;

  dialog->dialog = GTK_WIDGET (gtk_builder_get_object (xml, "listener_dialog"));
  g_assert (dialog->dialog != NULL);

  dialog->ip_buffer = GTK_TEXT_BUFFER (gtk_builder_get_object (xml, "ip_textbuffer"));
  g_assert (dialog->ip_buffer != NULL);
  setup_ip_buffer (dialog);

  dialog->enable_reverse_check = GTK_WIDGET (gtk_builder_get_object (xml, "enable_reverse_check"));
  g_assert (dialog->enable_reverse_check != NULL);
  g_signal_connect (dialog->enable_reverse_check,
		    "toggled",
		    G_CALLBACK (enable_reverse_toggled_cb),
		    dialog);

  dialog->always_enabled_check = GTK_WIDGET (gtk_builder_get_object (xml, "always_enabled_check"));
  g_assert (dialog->always_enabled_check != NULL);
  g_object_get (propono_prefs_get_default (),
		"always-enable-listening", &always,
		NULL);
  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (dialog->always_enabled_check),
				always);
  g_signal_connect (dialog->always_enabled_check,
		    "toggled",
		    G_CALLBACK (always_enabled_toggled_cb),
		    dialog);

  dialog->connectivity_exp = GTK_WIDGET (gtk_builder_get_object (xml, "connectivity_exp"));
  g_assert (dialog->connectivity_exp != NULL);

  dialog->port_label = GTK_WIDGET (gtk_builder_get_object (xml, "port_label"));
  g_assert (dialog->port_label != NULL);

  update_ui_sensitivity (dialog);

  g_signal_connect (dialog->dialog,
		    "destroy",
		    G_CALLBACK (dialog_destroy),
		    dialog);

  g_signal_connect (dialog->dialog,
		    "response",
		    G_CALLBACK (dialog_response_handler),
		    dialog);

  if (GTK_IS_WINDOW (parent))
    gtk_window_set_transient_for (GTK_WINDOW (dialog->dialog), GTK_WINDOW (parent));

  gtk_widget_show_all (dialog->dialog);
}

/* vim: set ts=8: */
