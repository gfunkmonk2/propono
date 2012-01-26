/*
 * propono-vnc-listener.c
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

#include <stdlib.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <arpa/inet.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>

#include <glib/gi18n.h>
#include <propono/propono-commands.h>
#include <propono/propono-app.h>
#include "propono-vnc-listener.h"
#include "propono-vnc-connection.h"

struct _ProponoVncListenerPrivate
{
  int server_sock;
  GIOChannel *io;
  gboolean listening;
  guint io_uid;
  gint port;
};

enum
{
  PROP_0,
  PROP_LISTENING,
  PROP_PORT
};

static ProponoVncListener *listener_singleton = NULL;

#define PROPONO_VNC_LISTENER_PRIVATE(o)  (G_TYPE_INSTANCE_GET_PRIVATE ((o), PROPONO_TYPE_VNC_LISTENER, ProponoVncListenerPrivate))
G_DEFINE_TYPE (ProponoVncListener, propono_vnc_listener, G_TYPE_OBJECT);

static void
propono_vnc_listener_init (ProponoVncListener *listener)
{
  listener->priv = G_TYPE_INSTANCE_GET_PRIVATE (listener, PROPONO_TYPE_VNC_LISTENER, ProponoVncListenerPrivate);

  listener->priv->io = NULL;
  listener->priv->server_sock = 0;
  listener->priv->listening = FALSE;
  listener->priv->io_uid = 0;
  listener->priv->port = 0;
}

static void
propono_vnc_listener_dispose (GObject *object)
{
  ProponoVncListener *listener = PROPONO_VNC_LISTENER (object);

  propono_vnc_listener_stop (listener);

  G_OBJECT_CLASS (propono_vnc_listener_parent_class)->dispose (object);
}

static void
propono_vnc_listener_get_property (GObject *object, guint prop_id, GValue *value, GParamSpec *pspec)
{
  ProponoVncListener *listener;

  g_return_if_fail (PROPONO_IS_VNC_LISTENER (object));

  listener = PROPONO_VNC_LISTENER (object);

  switch (prop_id)
    {
      case PROP_LISTENING:
	g_value_set_boolean (value, listener->priv->listening);
	break;

      case PROP_PORT:
	g_value_set_int (value, propono_vnc_listener_get_port (listener));
	break;

      default:
	G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
	break;
    }
}

static void
propono_vnc_listener_class_init (ProponoVncListenerClass *klass)
{
  GObjectClass* object_class = G_OBJECT_CLASS (klass);

  g_type_class_add_private (klass, sizeof (ProponoVncListenerPrivate));

  object_class->dispose = propono_vnc_listener_dispose;
  object_class->get_property = propono_vnc_listener_get_property;

  g_object_class_install_property (object_class,
                                   PROP_LISTENING,
                                   g_param_spec_boolean ("listening",
                                                        "Listening",
	                                                "If we are listening for incoming (reverse) VNC connections",
                                                        FALSE,
	                                                G_PARAM_READABLE |
                                                        G_PARAM_STATIC_NICK |
                                                        G_PARAM_STATIC_NAME |
                                                        G_PARAM_STATIC_BLURB));
  g_object_class_install_property (object_class,
                                   PROP_PORT,
                                   g_param_spec_int ("port",
                                                     "Port",
	                                             "TCP port in which we are listening for reverse connections",
                                                     5500,
                                                     5600,
                                                     5500,
	                                             G_PARAM_READABLE |
                                                     G_PARAM_STATIC_NICK |
                                                     G_PARAM_STATIC_NAME |
                                                     G_PARAM_STATIC_BLURB));

}

ProponoVncListener *
propono_vnc_listener_get_default (void)
{
  if (G_UNLIKELY (!listener_singleton))
    listener_singleton = PROPONO_VNC_LISTENER (g_object_new (PROPONO_TYPE_VNC_LISTENER, NULL));

  return listener_singleton;
}

static gboolean
incoming (GIOChannel *source, GIOCondition condition, ProponoVncListener *listener)
{
  ProponoConnection *conn;
  ProponoWindow *window;
  int cl_sock;
  struct sockaddr_in6 client_addr;
  char client_name[INET6_ADDRSTRLEN];
  socklen_t client_addr_len = sizeof (client_addr);

  cl_sock = accept (listener->priv->server_sock, (struct sockaddr *) &client_addr, &client_addr_len);
  if (cl_sock < 0)
    g_error ("accept() failed");

  window = propono_app_get_active_window (propono_app_get_default ());
  if (!window)
    {
      g_warning (_("Incoming VNC connection arrived but there is no active window"));
      return TRUE;
    }

  conn = propono_vnc_connection_new ();
  propono_vnc_connection_set_fd (PROPONO_VNC_CONNECTION (conn), cl_sock);

  if (inet_ntop (AF_INET6, &client_addr.sin6_addr.s6_addr, client_name, sizeof (client_name)) != NULL)
    propono_connection_set_host (conn, client_name);
  propono_connection_set_port (conn, ntohs (client_addr.sin6_port));

  propono_cmd_direct_connect (conn, window);

  return TRUE;
}

void
propono_vnc_listener_start (ProponoVncListener *listener)
{
  struct sockaddr_in6 server_addr;
  int port;

  g_return_if_fail (PROPONO_IS_VNC_LISTENER (listener));

  if (listener->priv->listening)
    return;

  listener->priv->server_sock = socket (AF_INET6, SOCK_STREAM, IPPROTO_TCP);
  if (listener->priv->server_sock < 0)
    g_error ("socket() failed");

  memset (&server_addr, 0, sizeof (server_addr));
  server_addr.sin6_family = AF_INET6;
  server_addr.sin6_addr = in6addr_any;

  for (port=5500; port<=5600; port++)
    {
      server_addr.sin6_port = htons (port);

      if (bind (listener->priv->server_sock, (struct sockaddr *) &server_addr, sizeof (server_addr)) == 0)
	break;
    }
  if (port>5600)
    g_error ("bind() failed");

  if (listen (listener->priv->server_sock, 5) < 0)
    g_error ("listen() failed");

  listener->priv->io = g_io_channel_unix_new (listener->priv->server_sock);
  listener->priv->io_uid = g_io_add_watch (listener->priv->io, G_IO_IN, (GIOFunc)incoming, listener);

  listener->priv->port = port;
  listener->priv->listening = TRUE;
  g_object_notify (G_OBJECT (listener), "listening");
}

void
propono_vnc_listener_stop (ProponoVncListener *listener)
{
  g_return_if_fail (PROPONO_IS_VNC_LISTENER (listener));

  if (!listener->priv->listening)
    return;

  if (listener->priv->io)
    {
      g_source_remove (listener->priv->io_uid);
      g_io_channel_unref (listener->priv->io);
      listener->priv->io = NULL;
    }

  if (listener->priv->server_sock > 0)
    {
      close (listener->priv->server_sock);
      listener->priv->server_sock = 0;
    }

  listener->priv->listening = FALSE;
  g_object_notify (G_OBJECT (listener), "listening");
}

gboolean
propono_vnc_listener_is_listening (ProponoVncListener *listener)
{
  g_return_val_if_fail (PROPONO_IS_VNC_LISTENER (listener), FALSE);

  return listener->priv->listening;
}

gint
propono_vnc_listener_get_port (ProponoVncListener *listener)
{
  g_return_val_if_fail (PROPONO_IS_VNC_LISTENER (listener), 0);

  return listener->priv->listening ? listener->priv->port : 0;
}

/* vim: set ts=8: */
