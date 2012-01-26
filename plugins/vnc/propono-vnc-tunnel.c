/*
 * propono-vnc-tunnel.c
 * VNC SSH Tunneling for Propono
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

#include <config.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>
#include <stdlib.h>
#include <glib/gi18n.h>

#include <propono/propono-ssh.h>
#include "propono-vnc-tunnel.h"

#define TUNNEL_PORT_OFFSET 5500

static int
find_free_port (void)
{
  int sock, port;
  struct sockaddr_in6 addr;

  memset (&addr, 0, sizeof (addr));
  addr.sin6_family = AF_INET6;
  addr.sin6_addr = in6addr_any;

  sock = socket (AF_INET6, SOCK_STREAM, IPPROTO_TCP);
  if (sock < 0)
    return 0;

  for (port = TUNNEL_PORT_OFFSET + 99; port > TUNNEL_PORT_OFFSET; port--)
    {
      addr.sin6_port = htons (port);
      if (bind (sock, (struct sockaddr *)&addr, sizeof (addr)) == 0)
	{
	  close (sock);
	  return port;
	}
    }

  close (sock);
  return 0;
}

static void
split_gateway (const gchar *gateway, gchar **host, gint *port)
{
  if (g_strrstr (gateway, ":") == NULL)
    {
      *host = g_strdup (gateway);
      *port = 22;
    }
  else
    {
      gchar **server = g_strsplit (gateway, ":", 2);
      *host = g_strdup (server[0]);
      *port = server[1] ? atoi (server[1]) : 22;
      g_strfreev (server);
    }
}


gboolean
propono_vnc_tunnel_create (GtkWindow *parent,
			   gchar **original_host,
			   gchar **original_port,
			   gchar *gateway,
			   GError **error)
{
  int local_port, gateway_port;
  gchar **tunnel_str, **command_str, *gateway_host;

  local_port = find_free_port ();
  if (local_port == 0)
    {
      g_set_error (error,
		   PROPONO_VNC_TUNNEL_ERROR,
		   PROPONO_VNC_TUNNEL_ERROR_NO_FREE_PORT,
		   _("Unable to find a free TCP port"));
      return FALSE;
    }

  tunnel_str = g_new (gchar *, 4);
  tunnel_str[0] = g_strdup ("-f");
  tunnel_str[1] = g_strdup ("-L");
  tunnel_str[2] = g_strdup_printf ("%d:%s:%s",
				   local_port,
				   *original_host,
				   *original_port);
  tunnel_str[3] = NULL;

  command_str = g_new (gchar *, 5);
  command_str[0] = g_strdup ("echo");
  command_str[1] = g_strdup_printf ("%s;", PROPONO_SSH_CHECK);
  command_str[2] = g_strdup ("sleep");
  command_str[3] = g_strdup ("15");
  command_str[4] = NULL;

  split_gateway (gateway, &gateway_host, &gateway_port);

  if (!propono_ssh_connect (parent,
			    gateway_host,
			    gateway_port,
			    NULL,
			    tunnel_str,
			    command_str,
			    NULL,
			    error))
    {
      g_strfreev (tunnel_str);
      g_strfreev (command_str);
      g_free (gateway_host);
      return FALSE;
    }

  g_strfreev (tunnel_str);
  g_strfreev (command_str);
  g_free (gateway_host);
  g_free (*original_host);
  *original_host = g_strdup ("localhost");
  g_free (*original_port);
  *original_port = g_strdup_printf ("%d", local_port);

  return TRUE;
}

GQuark 
propono_vnc_tunnel_error_quark (void)
{
  static GQuark quark = 0;

  if (!quark)
    quark = g_quark_from_string ("propono_vnc_tunnel_error");

  return quark;
}

/* vim: set ts=8: */
