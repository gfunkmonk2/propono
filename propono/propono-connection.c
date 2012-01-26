/*
 * propono-connection.c
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

#include <stdlib.h>
#include <glib/gi18n.h>
#include <gio/gio.h>
#include <gtk/gtk.h>

#include "propono-connection.h"
#include "propono-enums.h"
#include "propono-bookmarks.h"
#include "propono-plugin.h"
#include "propono-plugins-engine.h"

struct _ProponoConnectionPrivate
{
  gchar *protocol;
  gchar *host;
  gint   port;
  gchar *username;
  gchar *password;
  gchar *name;
  gboolean fullscreen;
};

enum
{
  PROP_0,
  PROP_PROTOCOL,
  PROP_HOST,
  PROP_PORT,
  PROP_USERNAME,
  PROP_PASSWORD,
  PROP_NAME,
  PROP_BEST_NAME,
  PROP_FULLSCREEN
};

#define PROPONO_CONNECTION_PRIVATE(o)  (G_TYPE_INSTANCE_GET_PRIVATE ((o), PROPONO_TYPE_CONNECTION, ProponoConnectionPrivate))
G_DEFINE_ABSTRACT_TYPE (ProponoConnection, propono_connection, G_TYPE_OBJECT);

static void
propono_connection_init (ProponoConnection *conn)
{
  conn->priv = G_TYPE_INSTANCE_GET_PRIVATE (conn, PROPONO_TYPE_CONNECTION, ProponoConnectionPrivate);

  conn->priv->protocol = NULL;
  conn->priv->host = NULL;
  conn->priv->port = 0;
  conn->priv->password = NULL;
  conn->priv->username = NULL;
  conn->priv->name = NULL;
  conn->priv->fullscreen = FALSE;
}

static void
propono_connection_finalize (GObject *object)
{
  ProponoConnection *conn = PROPONO_CONNECTION (object);

  g_free (conn->priv->protocol);
  g_free (conn->priv->host);
  g_free (conn->priv->username);
  g_free (conn->priv->password);
  g_free (conn->priv->name);

  G_OBJECT_CLASS (propono_connection_parent_class)->finalize (object);
}

static void
propono_connection_set_property (GObject *object, guint prop_id, const GValue *value, GParamSpec *pspec)
{
  ProponoConnection *conn;

  g_return_if_fail (PROPONO_IS_CONNECTION (object));

  conn = PROPONO_CONNECTION (object);

  switch (prop_id)
    {
      case PROP_PROTOCOL:
	propono_connection_set_protocol (conn, g_value_get_string (value));
	break;

      case PROP_HOST:
	propono_connection_set_host (conn, g_value_get_string (value));
	break;

      case PROP_PORT:
	propono_connection_set_port (conn, g_value_get_int (value));
	break;

      case PROP_USERNAME:
	propono_connection_set_username (conn, g_value_get_string (value));
	break;

      case PROP_PASSWORD:
	propono_connection_set_password (conn, g_value_get_string (value));
	break;

      case PROP_FULLSCREEN:
	propono_connection_set_fullscreen (conn, g_value_get_boolean (value));
	break;

      case PROP_NAME:
	propono_connection_set_name (conn, g_value_get_string (value));
	break;

      default:
	G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
	break;
    }
}

static void
propono_connection_get_property (GObject *object, guint prop_id, GValue *value, GParamSpec *pspec)
{
  ProponoConnection *conn;

  g_return_if_fail (PROPONO_IS_CONNECTION (object));

  conn = PROPONO_CONNECTION (object);


  switch (prop_id)
    {
      case PROP_PROTOCOL:
	g_value_set_string (value, conn->priv->protocol);
	break;

      case PROP_HOST:
	g_value_set_string (value, conn->priv->host);
	break;

      case PROP_PORT:
	g_value_set_int (value, conn->priv->port);
	break;

      case PROP_USERNAME:
	g_value_set_string (value, conn->priv->username);
	break;

      case PROP_PASSWORD:
	g_value_set_string (value, conn->priv->password);
	break;

      case PROP_FULLSCREEN:
	g_value_set_boolean (value, conn->priv->fullscreen);
	break;

      case PROP_NAME:
	g_value_set_string (value, conn->priv->name);
	break;

      case PROP_BEST_NAME:
	g_value_set_string (value, propono_connection_get_best_name (conn));
	break;

      default:
	G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
	break;
    }
}

static void
default_fill_writer (ProponoConnection *conn, xmlTextWriter *writer)
{
  if (conn->priv->protocol)
    xmlTextWriterWriteElement (writer, "protocol", conn->priv->protocol);
  xmlTextWriterWriteElement (writer, "name", conn->priv->name);
  xmlTextWriterWriteElement (writer, "host", conn->priv->host);
  xmlTextWriterWriteElement (writer, "username", conn->priv->username ? conn->priv->username : "");
  xmlTextWriterWriteFormatElement (writer, "port", "%d", conn->priv->port);
  xmlTextWriterWriteFormatElement (writer, "fullscreen", "%d", conn->priv->fullscreen);
}

static void
default_parse_item (ProponoConnection *conn, xmlNode *root)
{
  xmlNode *curr;
  xmlChar *s_value;

  for (curr = root->children; curr; curr = curr->next)
    {
      s_value = xmlNodeGetContent (curr);

      if (!xmlStrcmp(curr->name, (const xmlChar *)"host"))
	propono_connection_set_host (conn, s_value);
      else if (!xmlStrcmp(curr->name, (const xmlChar *)"name"))
	propono_connection_set_name (conn, s_value);
      else if (!xmlStrcmp(curr->name, (const xmlChar *)"username"))
	propono_connection_set_username (conn, s_value);
      else if (!xmlStrcmp(curr->name, (const xmlChar *)"port"))
	propono_connection_set_port (conn, atoi (s_value));
      else if (!xmlStrcmp(curr->name, (const xmlChar *)"fullscreen"))
	propono_connection_set_fullscreen (conn, propono_utils_parse_boolean (s_value));

      xmlFree (s_value);
    }
}

static gchar *
default_get_best_name (ProponoConnection *conn)
{
  if (conn->priv->name)
    return g_strdup (conn->priv->name);

  if (conn->priv->host)
    return propono_connection_get_string_rep (conn, FALSE);

  return NULL;
}

static void
default_parse_options_widget (ProponoConnection *conn, GtkWidget *widget)
{
}

static void
propono_connection_class_init (ProponoConnectionClass *klass)
{
  GObjectClass* object_class = G_OBJECT_CLASS (klass);

  g_type_class_add_private (klass, sizeof (ProponoConnectionPrivate));

  object_class->finalize = propono_connection_finalize;
  object_class->set_property = propono_connection_set_property;
  object_class->get_property = propono_connection_get_property;

  klass->impl_fill_writer = default_fill_writer;
  klass->impl_parse_item = default_parse_item;
  klass->impl_get_best_name = default_get_best_name;
  klass->impl_fill_conn_from_file = NULL;
  klass->impl_parse_options_widget = default_parse_options_widget;

  g_object_class_install_property (object_class,
                                   PROP_PROTOCOL,
                                   g_param_spec_string ("protocol",
                                                        "protocol",
	                                                "connection protocol",
                                                        NULL,
	                                                G_PARAM_READWRITE |
                                                        G_PARAM_CONSTRUCT |
                                                        G_PARAM_STATIC_NICK |
                                                        G_PARAM_STATIC_NAME |
                                                        G_PARAM_STATIC_BLURB));

  g_object_class_install_property (object_class,
                                   PROP_HOST,
                                   g_param_spec_string ("host",
                                                        "hostname",
	                                                "hostname or ip address of this connection",
                                                        NULL,
	                                                G_PARAM_READWRITE |
                                                        G_PARAM_CONSTRUCT |
                                                        G_PARAM_STATIC_NICK |
                                                        G_PARAM_STATIC_NAME |
                                                        G_PARAM_STATIC_BLURB));

  g_object_class_install_property (object_class,
                                   PROP_PORT,
                                   g_param_spec_int ("port",
                                                     "port",
	                                              "tcp/ip port of this connection",
                                                      0,
                                                      G_MAXINT,
                                                      0,
	                                              G_PARAM_READWRITE |
                                                      G_PARAM_CONSTRUCT |
                                                      G_PARAM_STATIC_NICK |
                                                      G_PARAM_STATIC_NAME |
                                                      G_PARAM_STATIC_BLURB));

  g_object_class_install_property (object_class,
                                   PROP_USERNAME,
                                   g_param_spec_string ("username",
                                                        "username",
	                                                "username (if any) necessary for complete this connection",
                                                        NULL,
	                                                G_PARAM_READWRITE |
                                                        G_PARAM_CONSTRUCT |
                                                        G_PARAM_STATIC_NICK |
                                                        G_PARAM_STATIC_NAME |
                                                        G_PARAM_STATIC_BLURB));

  g_object_class_install_property (object_class,
                                   PROP_PASSWORD,
                                   g_param_spec_string ("password",
                                                        "password",
	                                                "password (if any) necessary for complete this connection",
                                                        NULL,
	                                                G_PARAM_READWRITE |
                                                        G_PARAM_CONSTRUCT |
                                                        G_PARAM_STATIC_NICK |
                                                        G_PARAM_STATIC_NAME |
                                                        G_PARAM_STATIC_BLURB));

  g_object_class_install_property (object_class,
                                   PROP_NAME,
                                   g_param_spec_string ("name",
                                                        "connection name",
	                                                "friendly name for this connection",
                                                        NULL,
	                                                G_PARAM_READWRITE |
                                                        G_PARAM_CONSTRUCT |
                                                        G_PARAM_STATIC_NICK |
                                                        G_PARAM_STATIC_NAME |
                                                        G_PARAM_STATIC_BLURB));

  g_object_class_install_property (object_class,
                                   PROP_BEST_NAME,
                                   g_param_spec_string ("best-name",
                                                        "best-name",
	                                                "preferred name for this connection",
                                                        NULL,
	                                                G_PARAM_READABLE |
                                                        G_PARAM_STATIC_NICK |
                                                        G_PARAM_STATIC_NAME |
                                                        G_PARAM_STATIC_BLURB));

  g_object_class_install_property (object_class,
                                   PROP_FULLSCREEN,
                                   g_param_spec_boolean ("fullscreen",
                                                        "Full screen connection",
	                                                "Whether this connection is a view-only one",
                                                        FALSE,
	                                                G_PARAM_READWRITE |
                                                        G_PARAM_CONSTRUCT |
                                                        G_PARAM_STATIC_NICK |
                                                        G_PARAM_STATIC_NAME |
                                                        G_PARAM_STATIC_BLURB));

}

void
propono_connection_set_protocol (ProponoConnection *conn,
			         const gchar       *protocol)
{
  g_return_if_fail (PROPONO_IS_CONNECTION (conn));

  g_free (conn->priv->protocol);
  conn->priv->protocol = g_strdup (protocol);
}
const gchar *
propono_connection_get_protocol (ProponoConnection *conn)
{
  g_return_val_if_fail (PROPONO_IS_CONNECTION (conn), NULL);

  return conn->priv->protocol;
}

void
propono_connection_set_host (ProponoConnection *conn,
			     const gchar *host)
{
  g_return_if_fail (PROPONO_IS_CONNECTION (conn));

  g_free (conn->priv->host);
  if (host)
    conn->priv->host = g_strdup (g_strstrip ((gchar *)host));
  else
    conn->priv->host = NULL;
}
const gchar *
propono_connection_get_host (ProponoConnection *conn)
{
  g_return_val_if_fail (PROPONO_IS_CONNECTION (conn), NULL);

  return conn->priv->host;
}

void
propono_connection_set_port (ProponoConnection *conn,
			     gint port)
{
  g_return_if_fail (PROPONO_IS_CONNECTION (conn));

  conn->priv->port = port;
}
gint
propono_connection_get_port (ProponoConnection *conn)
{
  g_return_val_if_fail (PROPONO_IS_CONNECTION (conn), 0);

  return conn->priv->port;
}

void
propono_connection_set_username (ProponoConnection *conn,
				 const gchar *username)
{
  g_return_if_fail (PROPONO_IS_CONNECTION (conn));

  g_free (conn->priv->username);
  conn->priv->username = g_strdup (username);
}
const gchar *
propono_connection_get_username (ProponoConnection *conn)
{
  g_return_val_if_fail (PROPONO_IS_CONNECTION (conn), NULL);

  return conn->priv->username;
}

void
propono_connection_set_password (ProponoConnection *conn,
			         const gchar *password)
{
  g_return_if_fail (PROPONO_IS_CONNECTION (conn));

  g_free (conn->priv->password);
  conn->priv->password = g_strdup (password);
}
const gchar *
propono_connection_get_password (ProponoConnection *conn)
{
  g_return_val_if_fail (PROPONO_IS_CONNECTION (conn), NULL);

  return conn->priv->password;
}

void
propono_connection_set_fullscreen (ProponoConnection *conn,
				  gboolean value)
{
  g_return_if_fail (PROPONO_IS_CONNECTION (conn));

  conn->priv->fullscreen = value;
}
gboolean
propono_connection_get_fullscreen (ProponoConnection *conn)
{
  g_return_val_if_fail (PROPONO_IS_CONNECTION (conn), FALSE);

  return conn->priv->fullscreen;
}

void
propono_connection_set_name (ProponoConnection *conn,
			     const gchar *name)
{
  g_return_if_fail (PROPONO_IS_CONNECTION (conn));

  g_free (conn->priv->name);
  conn->priv->name = g_strdup (name);
}
const gchar *
propono_connection_get_name (ProponoConnection *conn)
{
  g_return_val_if_fail (PROPONO_IS_CONNECTION (conn), NULL);

  return conn->priv->name;
}

/**
 * propono_connection_split_string:
 * @uri: The URI to be splitted.
 * @known_protocol: The protocol, if it's known. NULL otherwise.
 * @protocol: Will hold the protocol of this URI
 * @host: Will hold the host of this URI
 * @port: Will hold the port of this URI
 * @error_msg: Will hold an error message in case of fail
 *
 * Splits a URI into its several parts.
 *
 * Returns: %TRUE if the URI is splitted successfuly. FALSE otherwise.
 */
gboolean
propono_connection_split_string (const gchar *uri,
				 const gchar *known_protocol,
				 gchar       **protocol,
				 gchar       **host,
				 gint         *port,
				 gchar       **error_msg)
{
  gchar **server, **url;
  gint    lport;
  gchar  *lhost;
  gchar   ipv6_host[255] = {0,};
  ProponoPlugin *plugin;

  *error_msg = NULL;
  *host = NULL;
  *port = 0;

  url = g_strsplit (uri, "://", 2);
  if (g_strv_length (url) == 2)
    {
      if (known_protocol)
	*protocol = g_strdup (known_protocol);
      else
	*protocol = g_strdup (url[0]);
      lhost = url[1];
    }
  else
    {
      if (known_protocol)
	*protocol = g_strdup (known_protocol);
      else
	*protocol = g_strdup ("vnc");
      lhost = (gchar *) uri;
    }

  plugin = propono_plugins_engine_get_plugin_by_protocol (propono_plugins_engine_get_default (),
							  *protocol);
  if (!plugin)
    {
      *error_msg = g_strdup_printf (_("The protocol %s is not supported."), *protocol);
      g_free (*protocol);
      *protocol = NULL;
      g_strfreev (url);
      return FALSE;
    }

  if (lhost[0] == '[')
    {
      int i;
      for (i = 1; lhost[i] && lhost[i] != ']'; i++)
	{
	  ipv6_host[i-1] = lhost[i];
	  lhost[i-1] = '_';
	}
      ipv6_host[i-1] = '\0';
      lhost[i] = '_';
    }

  if (g_strrstr (lhost, "::") != NULL)
    {
      server = g_strsplit (lhost, "::", 2);
      lport = server[1] ? atoi (server[1]) : propono_plugin_get_default_port (plugin);
    }
  else
    {
      server = g_strsplit (lhost, ":", 2);
      lport = server[1] ? atoi (server[1]) : propono_plugin_get_default_port (plugin);

      if ((g_str_equal (*protocol, "vnc")) && (lport < 1024))
        lport += 5900;
    }

  lhost = ipv6_host[0] ? ipv6_host : (server[0][0] ? server[0] : "localhost");

  *host = g_strdup (lhost);
  *port = lport;

  g_strfreev (server);
  g_strfreev (url);

  return TRUE;
}

ProponoConnection *
propono_connection_new_from_string (const gchar *uri, gchar **error_msg, gboolean use_bookmarks)
{
  ProponoConnection *conn = NULL;
  gint    port;
  gchar  *host, *protocol;
  ProponoPlugin *plugin;

  if (!propono_connection_split_string (uri, NULL, &protocol, &host, &port, error_msg))
    return NULL;

  if (use_bookmarks)
    conn = propono_bookmarks_exists (propono_bookmarks_get_default (),
				     protocol,
				     host,
				     port);
  if (!conn)
    {
      plugin = propono_plugins_engine_get_plugin_by_protocol (propono_plugins_engine_get_default (),
							      protocol);
      if (!plugin)
	goto finalize;

      conn = propono_plugin_new_connection (plugin);
      propono_connection_set_host (conn, host);
      propono_connection_set_port (conn, port);
    }

finalize:
  g_free (host);
  g_free (protocol);
  return conn;
}

ProponoConnection *
propono_connection_new_from_file (const gchar *uri, gchar **error_msg, gboolean use_bookmarks)
{
  ProponoConnection *conn;
  gchar             *data;
  GFile             *file_a;
  GError            *error;
  GHashTable        *plugins;
  GHashTableIter     iter;
  gpointer           plugin;

  *error_msg = NULL;
  data = NULL;
  conn = NULL;
  error = NULL;

  file_a = g_file_new_for_commandline_arg (uri);
  if (!g_file_load_contents (file_a,
			     NULL,
			     &data,
			     NULL,
			     NULL,
			     &error))
    {
      if (error)
	{
	  *error_msg = g_strdup (error->message);
	  g_error_free (error);
	}
      else
	*error_msg = g_strdup (_("Could not open the file."));

      goto the_end;
    }

  plugins = propono_plugin_engine_get_plugins_by_protocol (propono_plugins_engine_get_default ());
  g_hash_table_iter_init (&iter, plugins);
  while (g_hash_table_iter_next (&iter, NULL, &plugin))
    {
      conn = propono_plugin_new_connection_from_file (PROPONO_PLUGIN (plugin),
						      data,
						      use_bookmarks,
						      error_msg);
      g_free (*error_msg);
      *error_msg = NULL;
      if (conn)
	break;
    }

the_end:
  g_free (data);
  g_object_unref (file_a);

  if (!conn && !*error_msg)
    *error_msg = g_strdup (_("The file was not recognized by any of the plugins."));

  return conn;
}

gchar*
propono_connection_get_string_rep (ProponoConnection *conn,
				   gboolean has_protocol)
{
  GString *uri;
  gchar *result;
  gboolean is_ipv6;
  ProponoPlugin *plugin;

  g_return_val_if_fail (PROPONO_IS_CONNECTION (conn), NULL);

  is_ipv6 = g_strstr_len (conn->priv->host, -1, ":") != NULL;

  if (has_protocol)
    {
      uri = g_string_new (conn->priv->protocol);
      g_string_append (uri, "://");
    }
  else
    uri = g_string_new (NULL);

  if (is_ipv6)
    g_string_append_c (uri, '[');
  g_string_append (uri, conn->priv->host);
  if (is_ipv6)
    g_string_append_c (uri, ']');

  plugin = g_hash_table_lookup (propono_plugin_engine_get_plugins_by_protocol (propono_plugins_engine_get_default ()),
				conn->priv->protocol);
  if (plugin)
    if (propono_plugin_get_default_port (plugin) != conn->priv->port)
      g_string_append_printf (uri, "::%d", conn->priv->port);

  result = uri->str;
  g_string_free (uri, FALSE);

  return result;
}

void
propono_connection_fill_writer (ProponoConnection *conn,
				xmlTextWriter     *writer)
{
  PROPONO_CONNECTION_GET_CLASS (conn)->impl_fill_writer (conn, writer);
}

void
propono_connection_parse_item (ProponoConnection *conn,
			       xmlNode           *root)
{
  PROPONO_CONNECTION_GET_CLASS (conn)->impl_parse_item (conn, root);
}

gchar*
propono_connection_get_best_name (ProponoConnection *conn)
{
  return PROPONO_CONNECTION_GET_CLASS (conn)->impl_get_best_name (conn);
}

void
propono_connection_parse_options_widget (ProponoConnection *conn,
					 GtkWidget         *widget)
{
  g_return_if_fail (PROPONO_IS_CONNECTION (conn));
  g_return_if_fail (GTK_IS_WIDGET (widget));

  PROPONO_CONNECTION_GET_CLASS (conn)->impl_parse_options_widget (conn, widget);
}
/* vim: set ts=8: */
