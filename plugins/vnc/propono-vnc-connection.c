/*
 * propono-vnc-connection.c
 * Child class of abstract ProponoConnection, specific to VNC protocol
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
#include <stdlib.h>
#include <propono/propono-utils.h>
#include <propono/propono-cache-prefs.h>

#include "propono-vnc-connection.h"

struct _ProponoVncConnectionPrivate
{
  gchar    *desktop_name;
  gboolean view_only;
  gboolean scaling;
  gboolean keep_ratio;
  gint     shared;
  gint     fd;
  gint     depth_profile;
  gboolean lossy_encoding;
  gchar    *ssh_tunnel_host;
};

enum
{
  PROP_0,
  PROP_DESKTOP_NAME,
  PROP_VIEW_ONLY,
  PROP_SCALING,
  PROP_KEEP_RATIO,
  PROP_SHARED,
  PROP_FD,
  PROP_DEPTH_PROFILE,
  PROP_LOSSY_ENCODING,
  PROP_SSH_TUNNEL_HOST
};

#define PROPONO_VNC_CONNECTION_PRIVATE(o)  (G_TYPE_INSTANCE_GET_PRIVATE ((o), PROPONO_TYPE_VNC_CONNECTION, ProponoVncConnectionPrivate))
G_DEFINE_TYPE (ProponoVncConnection, propono_vnc_connection, PROPONO_TYPE_CONNECTION);

static void
propono_vnc_connection_init (ProponoVncConnection *conn)
{
  conn->priv = G_TYPE_INSTANCE_GET_PRIVATE (conn, PROPONO_TYPE_VNC_CONNECTION, ProponoVncConnectionPrivate);

  conn->priv->desktop_name = NULL;
  conn->priv->view_only = FALSE;
  conn->priv->scaling = FALSE;
  conn->priv->keep_ratio = FALSE;
  conn->priv->shared = -1;
  conn->priv->fd = 0;
  conn->priv->depth_profile = 0;
  conn->priv->lossy_encoding = FALSE;
  conn->priv->ssh_tunnel_host = NULL;
}

static void
propono_vnc_connection_constructed (GObject *object)
{
  propono_connection_set_protocol (PROPONO_CONNECTION (object), "vnc");
}

static void
propono_vnc_connection_finalize (GObject *object)
{
  ProponoVncConnection *conn = PROPONO_VNC_CONNECTION (object);

  g_free (conn->priv->desktop_name);
  g_free (conn->priv->ssh_tunnel_host);

  G_OBJECT_CLASS (propono_vnc_connection_parent_class)->finalize (object);
}

static void
propono_vnc_connection_set_property (GObject *object, guint prop_id, const GValue *value, GParamSpec *pspec)
{
  ProponoVncConnection *conn;

  g_return_if_fail (PROPONO_IS_VNC_CONNECTION (object));

  conn = PROPONO_VNC_CONNECTION (object);

  switch (prop_id)
    {
      case PROP_DESKTOP_NAME:
	propono_vnc_connection_set_desktop_name (conn, g_value_get_string (value));
	break;

      case PROP_VIEW_ONLY:
	propono_vnc_connection_set_view_only (conn, g_value_get_boolean (value));
	break;

      case PROP_SCALING:
	propono_vnc_connection_set_scaling (conn, g_value_get_boolean (value));
	break;

      case PROP_KEEP_RATIO:
	propono_vnc_connection_set_keep_ratio (conn, g_value_get_boolean (value));
	break;

      case PROP_SHARED:
	propono_vnc_connection_set_shared (conn, g_value_get_int (value));
	break;

      case PROP_FD:
	propono_vnc_connection_set_fd (conn, g_value_get_int (value));
	break;

      case PROP_DEPTH_PROFILE:
	propono_vnc_connection_set_depth_profile (conn, g_value_get_int (value));
	break;

      case PROP_LOSSY_ENCODING:
	propono_vnc_connection_set_lossy_encoding (conn, g_value_get_boolean (value));
	break;

      case PROP_SSH_TUNNEL_HOST:
	propono_vnc_connection_set_ssh_tunnel_host (conn, g_value_get_string (value));
	break;

      default:
	G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
	break;
    }
}

static void
propono_vnc_connection_get_property (GObject *object, guint prop_id, GValue *value, GParamSpec *pspec)
{
  ProponoVncConnection *conn;

  g_return_if_fail (PROPONO_IS_VNC_CONNECTION (object));

  conn = PROPONO_VNC_CONNECTION (object);

  switch (prop_id)
    {
      case PROP_DESKTOP_NAME:
	g_value_set_string (value, conn->priv->desktop_name);
	break;

      case PROP_VIEW_ONLY:
	g_value_set_boolean (value, conn->priv->view_only);
	break;

      case PROP_SCALING:
	g_value_set_boolean (value, conn->priv->scaling);
	break;

      case PROP_KEEP_RATIO:
	g_value_set_boolean (value, conn->priv->keep_ratio);
	break;

      case PROP_SHARED:
	g_value_set_int (value, conn->priv->shared);
	break;

      case PROP_FD:
	g_value_set_int (value, conn->priv->fd);
	break;

      case PROP_DEPTH_PROFILE:
	g_value_set_int (value, conn->priv->depth_profile);
	break;

      case PROP_LOSSY_ENCODING:
	g_value_set_boolean (value, conn->priv->lossy_encoding);
	break;

      case PROP_SSH_TUNNEL_HOST:
	g_value_set_string (value, conn->priv->ssh_tunnel_host);
	break;

      default:
	G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
	break;
    }
}

static void
vnc_fill_writer (ProponoConnection *conn, xmlTextWriter *writer)
{
  ProponoVncConnection *vnc_conn = PROPONO_VNC_CONNECTION (conn);
  PROPONO_CONNECTION_CLASS (propono_vnc_connection_parent_class)->impl_fill_writer (conn, writer);

  xmlTextWriterWriteFormatElement (writer, (const xmlChar *)"view_only", "%d", vnc_conn->priv->view_only);
  xmlTextWriterWriteFormatElement (writer, (const xmlChar *)"scaling", "%d", vnc_conn->priv->scaling);
  xmlTextWriterWriteFormatElement (writer, (const xmlChar *)"keep_ratio", "%d", vnc_conn->priv->keep_ratio);
  xmlTextWriterWriteFormatElement (writer, (const xmlChar *)"depth_profile", "%d", vnc_conn->priv->depth_profile);
  xmlTextWriterWriteFormatElement (writer, (const xmlChar *)"lossy_encoding", "%d", vnc_conn->priv->lossy_encoding);

  if (vnc_conn->priv->ssh_tunnel_host && *vnc_conn->priv->ssh_tunnel_host)
    xmlTextWriterWriteFormatElement (writer, (const xmlChar *)"ssh_tunnel_host", "%s", vnc_conn->priv->ssh_tunnel_host);
}

static void
vnc_parse_item (ProponoConnection *conn, xmlNode *root)
{
  xmlNode *curr;
  xmlChar *s_value;
  ProponoVncConnection *vnc_conn = PROPONO_VNC_CONNECTION (conn);

  PROPONO_CONNECTION_CLASS (propono_vnc_connection_parent_class)->impl_parse_item (conn, root);

  for (curr = root->children; curr; curr = curr->next)
    {
      s_value = xmlNodeGetContent (curr);

      if (!xmlStrcmp(curr->name, (const xmlChar *)"view_only"))
	{
	  propono_vnc_connection_set_view_only (vnc_conn, propono_utils_parse_boolean ((const gchar *)s_value));
	}
      else if (!xmlStrcmp(curr->name, (const xmlChar *)"scaling"))
	{
	  if (!scaling_command_line)
	    propono_vnc_connection_set_scaling (vnc_conn, propono_utils_parse_boolean ((const gchar *)s_value));
	}
      else if (!xmlStrcmp(curr->name, (const xmlChar *)"keep_ratio"))
	{
	  propono_vnc_connection_set_keep_ratio (vnc_conn, propono_utils_parse_boolean ((const gchar *)s_value));
	}
      else if (!xmlStrcmp(curr->name, (const xmlChar *)"depth_profile"))
	{
	  propono_vnc_connection_set_depth_profile (vnc_conn, atoi((const char *)s_value));
	}
      else if (!xmlStrcmp(curr->name, (const xmlChar *)"lossy_encoding"))
	{
	  propono_vnc_connection_set_lossy_encoding (vnc_conn, propono_utils_parse_boolean ((const gchar *)s_value));
	}
      else if (!xmlStrcmp(curr->name, (const xmlChar *)"ssh_tunnel_host"))
	{
	  propono_vnc_connection_set_ssh_tunnel_host (vnc_conn, (const gchar *)s_value);
	}

      xmlFree (s_value);
    }
}

static gchar *
vnc_get_best_name (ProponoConnection *conn)
{
  ProponoVncConnection *vnc_conn = PROPONO_VNC_CONNECTION (conn);

  if (propono_connection_get_name (conn))
    return g_strdup (propono_connection_get_name (conn));

  if (vnc_conn->priv->desktop_name)
    return g_strdup (vnc_conn->priv->desktop_name);

  if (propono_connection_get_host (conn))
    return propono_connection_get_string_rep (conn, FALSE);

  return NULL;
}

static void
vnc_fill_conn_from_file (ProponoConnection *conn, GKeyFile *file)
{
  gint shared;
  GError *e = NULL;

  shared = g_key_file_get_integer (file, "options", "shared", &e);
  if (e)
    {
      g_error_free (e);
      return;
    }
  else
    if (shared == 0 || shared == 1)
      propono_vnc_connection_set_shared (PROPONO_VNC_CONNECTION (conn), shared);
    else
      /* Translators: 'shared' here is a VNC protocol specific flag. You can translate it, but I think it's better to let it untranslated */
      g_message (_("Bad value for 'shared' flag: %d. It is supposed to be 0 or 1. Ignoring it."), shared);
}

static void
vnc_parse_options_widget (ProponoConnection *conn, GtkWidget *widget)
{
  GtkWidget *view_only, *scaling, *depth_combo, *lossy, *ssh_host, *ratio;

  view_only = g_object_get_data (G_OBJECT (widget), "view_only");
  scaling = g_object_get_data (G_OBJECT (widget), "scaling");
  ratio = g_object_get_data (G_OBJECT (widget), "ratio");
  depth_combo = g_object_get_data (G_OBJECT (widget), "depth_combo");
  lossy = g_object_get_data (G_OBJECT (widget), "lossy");
  ssh_host = g_object_get_data (G_OBJECT (widget), "ssh_host");
  if (!view_only || !scaling || !depth_combo || !lossy || !ssh_host || !ratio)
    {
      g_warning ("Wrong widget passed to vnc_parse_options_widget()");
      return;
    }

  propono_cache_prefs_set_boolean ("vnc-connection", "view-only", gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (view_only)));
  propono_cache_prefs_set_boolean ("vnc-connection", "scaling", gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (scaling)));
  propono_cache_prefs_set_boolean ("vnc-connection", "keep-ratio", gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (ratio)));
  propono_cache_prefs_set_integer ("vnc-connection", "depth-profile", gtk_combo_box_get_active (GTK_COMBO_BOX (depth_combo)));
  propono_cache_prefs_set_boolean ("vnc-connection", "lossy-encoding", gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (lossy)));
  propono_cache_prefs_set_string  ("vnc-connection", "ssh-tunnel-host", gtk_entry_get_text (GTK_ENTRY (ssh_host)));

  g_object_set (conn,
		"view-only", gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (view_only)),
		"scaling", gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (scaling)),
		"keep-ratio", gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (ratio)),
		"depth-profile", gtk_combo_box_get_active (GTK_COMBO_BOX (depth_combo)),
		"lossy-encoding", gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (lossy)),
		"ssh-tunnel-host", gtk_entry_get_text (GTK_ENTRY (ssh_host)),
		NULL);
}

static void
propono_vnc_connection_class_init (ProponoVncConnectionClass *klass)
{
  GObjectClass* object_class = G_OBJECT_CLASS (klass);
  ProponoConnectionClass* parent_class = PROPONO_CONNECTION_CLASS (klass);

  g_type_class_add_private (klass, sizeof (ProponoVncConnectionPrivate));

  object_class->finalize = propono_vnc_connection_finalize;
  object_class->set_property = propono_vnc_connection_set_property;
  object_class->get_property = propono_vnc_connection_get_property;
  object_class->constructed  = propono_vnc_connection_constructed;

  parent_class->impl_fill_writer = vnc_fill_writer;
  parent_class->impl_parse_item  = vnc_parse_item;
  parent_class->impl_get_best_name = vnc_get_best_name;
  parent_class->impl_fill_conn_from_file = vnc_fill_conn_from_file;
  parent_class->impl_parse_options_widget = vnc_parse_options_widget;

  g_object_class_install_property (object_class,
                                   PROP_DESKTOP_NAME,
                                   g_param_spec_string ("desktop-name",
                                                        "desktop-name",
	                                                "name of this connection as reported by the server",
                                                        NULL,
	                                                G_PARAM_READWRITE |
                                                        G_PARAM_CONSTRUCT |
                                                        G_PARAM_STATIC_NICK |
                                                        G_PARAM_STATIC_NAME |
                                                        G_PARAM_STATIC_BLURB));

  g_object_class_install_property (object_class,
                                   PROP_VIEW_ONLY,
                                   g_param_spec_boolean ("view-only",
                                                        "View-only connection",
	                                                "Whether this connection is a view-only one",
                                                        FALSE,
	                                                G_PARAM_READWRITE |
                                                        G_PARAM_CONSTRUCT |
                                                        G_PARAM_STATIC_NICK |
                                                        G_PARAM_STATIC_NAME |
                                                        G_PARAM_STATIC_BLURB));
  g_object_class_install_property (object_class,
                                   PROP_SCALING,
                                   g_param_spec_boolean ("scaling",
                                                        "Use scaling",
	                                                "Whether to use scaling on this connection",
                                                        FALSE,
	                                                G_PARAM_READWRITE |
                                                        G_PARAM_CONSTRUCT |
                                                        G_PARAM_STATIC_NICK |
                                                        G_PARAM_STATIC_NAME |
                                                        G_PARAM_STATIC_BLURB));

  g_object_class_install_property (object_class,
                                   PROP_KEEP_RATIO,
                                   g_param_spec_boolean ("keep-ratio",
                                                        "Keep Ratio",
	                                                "Whether to keep the aspect ratio when using scaling",
                                                        FALSE,
	                                                G_PARAM_READWRITE |
                                                        G_PARAM_CONSTRUCT |
                                                        G_PARAM_STATIC_NICK |
                                                        G_PARAM_STATIC_NAME |
                                                        G_PARAM_STATIC_BLURB));

  g_object_class_install_property (object_class,
                                   PROP_SHARED,
                                   g_param_spec_int ("shared",
                                                     "shared flag",
	                                              "if the server should allow more than one client connected",
                                                      -1,
                                                      1,
                                                      -1,
	                                              G_PARAM_READWRITE |
                                                      G_PARAM_CONSTRUCT |
                                                      G_PARAM_STATIC_NICK |
                                                      G_PARAM_STATIC_NAME |
                                                      G_PARAM_STATIC_BLURB));

  g_object_class_install_property (object_class,
                                   PROP_FD,
                                   g_param_spec_int ("fd",
                                                     "file descriptor",
	                                              "the file descriptor for this connection",
                                                      0,
                                                      G_MAXINT,
                                                      0,
	                                              G_PARAM_READWRITE |
                                                      G_PARAM_CONSTRUCT |
                                                      G_PARAM_STATIC_NICK |
                                                      G_PARAM_STATIC_NAME |
                                                      G_PARAM_STATIC_BLURB));

  g_object_class_install_property (object_class,
                                   PROP_DEPTH_PROFILE,
                                   g_param_spec_int ("depth-profile",
                                                     "Depth Profile",
	                                              "The profile of depth color to be used in gtk-vnc widget",
                                                      0,
                                                      5,
                                                      0,
	                                              G_PARAM_READWRITE |
                                                      G_PARAM_CONSTRUCT |
                                                      G_PARAM_STATIC_NICK |
                                                      G_PARAM_STATIC_NAME |
                                                      G_PARAM_STATIC_BLURB));

  g_object_class_install_property (object_class,
                                   PROP_LOSSY_ENCODING,
                                   g_param_spec_boolean ("lossy-encoding",
                                                        "Lossy encoding",
	                                                "Whether to use a lossy encoding",
                                                        FALSE,
	                                                G_PARAM_READWRITE |
                                                        G_PARAM_CONSTRUCT |
                                                        G_PARAM_STATIC_NICK |
                                                        G_PARAM_STATIC_NAME |
                                                        G_PARAM_STATIC_BLURB));

  g_object_class_install_property (object_class,
                                   PROP_SSH_TUNNEL_HOST,
                                   g_param_spec_string ("ssh-tunnel-host",
                                                        "SSH Tunnel Host",
	                                                "hostname used to create the SSH tunnel",
                                                        NULL,
	                                                G_PARAM_READWRITE |
                                                        G_PARAM_CONSTRUCT |
                                                        G_PARAM_STATIC_NICK |
                                                        G_PARAM_STATIC_NAME |
                                                        G_PARAM_STATIC_BLURB));

}

ProponoConnection *
propono_vnc_connection_new (void)
{
  return PROPONO_CONNECTION (g_object_new (PROPONO_TYPE_VNC_CONNECTION, NULL));
}

void
propono_vnc_connection_set_desktop_name (ProponoVncConnection *conn,
					 const gchar *desktop_name)
{
  g_return_if_fail (PROPONO_IS_VNC_CONNECTION (conn));

  g_free (conn->priv->desktop_name);
  conn->priv->desktop_name = g_strdup (desktop_name);
}
const gchar *
propono_vnc_connection_get_desktop_name (ProponoVncConnection *conn)
{
  g_return_val_if_fail (PROPONO_IS_VNC_CONNECTION (conn), NULL);

  return conn->priv->desktop_name;
}

void
propono_vnc_connection_set_shared (ProponoVncConnection *conn,
				   gint value)
{
  g_return_if_fail (PROPONO_IS_VNC_CONNECTION (conn));
  g_return_if_fail (value >=-1 && value <=1);

  conn->priv->shared = value;
}
gint
propono_vnc_connection_get_shared (ProponoVncConnection *conn)
{
  g_return_val_if_fail (PROPONO_IS_VNC_CONNECTION (conn), -1);

  return conn->priv->shared;
}

void
propono_vnc_connection_set_view_only (ProponoVncConnection *conn,
				      gboolean value)
{
  g_return_if_fail (PROPONO_IS_VNC_CONNECTION (conn));

  conn->priv->view_only = value;
}
gboolean
propono_vnc_connection_get_view_only (ProponoVncConnection *conn)
{
  g_return_val_if_fail (PROPONO_IS_VNC_CONNECTION (conn), FALSE);

  return conn->priv->view_only;
}

void
propono_vnc_connection_set_scaling (ProponoVncConnection *conn,
				    gboolean value)
{
  g_return_if_fail (PROPONO_IS_VNC_CONNECTION (conn));

  conn->priv->scaling = value;
}
gboolean
propono_vnc_connection_get_scaling (ProponoVncConnection *conn)
{
  g_return_val_if_fail (PROPONO_IS_VNC_CONNECTION (conn), FALSE);

  return conn->priv->scaling;
}

void
propono_vnc_connection_set_keep_ratio (ProponoVncConnection *conn,
				       gboolean value)
{
  g_return_if_fail (PROPONO_IS_VNC_CONNECTION (conn));

  conn->priv->keep_ratio = value;
}
gboolean
propono_vnc_connection_get_keep_ratio (ProponoVncConnection *conn)
{
  g_return_val_if_fail (PROPONO_IS_VNC_CONNECTION (conn), FALSE);

  return conn->priv->keep_ratio;
}

void
propono_vnc_connection_set_fd (ProponoVncConnection *conn,
			       gint                 value)
{
  g_return_if_fail (PROPONO_IS_VNC_CONNECTION (conn));
  g_return_if_fail (value >= 0);

  conn->priv->fd = value;
}
gint
propono_vnc_connection_get_fd (ProponoVncConnection *conn)
{
  g_return_val_if_fail (PROPONO_IS_VNC_CONNECTION (conn), 0);

  return conn->priv->fd;
}

void
propono_vnc_connection_set_depth_profile (ProponoVncConnection *conn,
					  gint                 value)
{
  g_return_if_fail (PROPONO_IS_VNC_CONNECTION (conn));
  g_return_if_fail (value >= 0);

  conn->priv->depth_profile = value;
}
gint
propono_vnc_connection_get_depth_profile (ProponoVncConnection *conn)
{
  g_return_val_if_fail (PROPONO_IS_VNC_CONNECTION (conn), 0);

  return conn->priv->depth_profile;
}

void
propono_vnc_connection_set_lossy_encoding (ProponoVncConnection *conn,
					   gboolean value)
{
  g_return_if_fail (PROPONO_IS_VNC_CONNECTION (conn));

  conn->priv->lossy_encoding = value;
}
gboolean
propono_vnc_connection_get_lossy_encoding (ProponoVncConnection *conn)
{
  g_return_val_if_fail (PROPONO_IS_VNC_CONNECTION (conn), FALSE);

  return conn->priv->lossy_encoding;
}

void
propono_vnc_connection_set_ssh_tunnel_host (ProponoVncConnection *conn,
					    const gchar *host)
{
  g_return_if_fail (PROPONO_IS_VNC_CONNECTION (conn));

  g_free (conn->priv->ssh_tunnel_host);
  conn->priv->ssh_tunnel_host = g_strdup (host);
}
const gchar *
propono_vnc_connection_get_ssh_tunnel_host (ProponoVncConnection *conn)
{
  g_return_val_if_fail (PROPONO_IS_VNC_CONNECTION (conn), NULL);

  return conn->priv->ssh_tunnel_host;
}

/* vim: set ts=8: */
