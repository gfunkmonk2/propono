/*
 * propono-vnc-connection.h
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

#ifndef __PROPONO_VNC_CONNECTION_H__
#define __PROPONO_VNC_CONNECTION_H__

#include <propono/propono-connection.h>

G_BEGIN_DECLS

gboolean scaling_command_line;

#define PROPONO_TYPE_VNC_CONNECTION             (propono_vnc_connection_get_type ())
#define PROPONO_VNC_CONNECTION(obj)             (G_TYPE_CHECK_INSTANCE_CAST ((obj), PROPONO_TYPE_VNC_CONNECTION, ProponoVncConnection))
#define PROPONO_VNC_CONNECTION_CLASS(klass)     (G_TYPE_CHECK_CLASS_CAST ((klass), PROPONO_TYPE_VNC_CONNECTION, ProponoVncConnectionClass))
#define PROPONO_IS_VNC_CONNECTION(obj)          (G_TYPE_CHECK_INSTANCE_TYPE ((obj), PROPONO_TYPE_VNC_CONNECTION))
#define PROPONO_IS_VNC_CONNECTION_CLASS(klass)  (G_TYPE_CHECK_CLASS_TYPE ((klass), PROPONO_TYPE_VNC_CONNECTION))
#define PROPONO_VNC_CONNECTION_GET_CLASS(obj)   (G_TYPE_INSTANCE_GET_CLASS ((obj), PROPONO_TYPE_VNC_CONNECTION, ProponoVncConnectionClass))

typedef struct _ProponoVncConnectionClass   ProponoVncConnectionClass;
typedef struct _ProponoVncConnection        ProponoVncConnection;
typedef struct _ProponoVncConnectionPrivate ProponoVncConnectionPrivate;

struct _ProponoVncConnectionClass
{
  ProponoConnectionClass parent_class;
};

struct _ProponoVncConnection
{
  ProponoConnection parent_instance;
  ProponoVncConnectionPrivate *priv;
};


GType propono_vnc_connection_get_type (void) G_GNUC_CONST;

ProponoConnection*  propono_vnc_connection_new (void);

const gchar*	    propono_vnc_connection_get_desktop_name (ProponoVncConnection *conn);
void		    propono_vnc_connection_set_desktop_name (ProponoVncConnection *conn,
							     const gchar *desktop_name);

gboolean	    propono_vnc_connection_get_view_only    (ProponoVncConnection *conn);
void		    propono_vnc_connection_set_view_only    (ProponoVncConnection *conn,
							     gboolean value);

gboolean	    propono_vnc_connection_get_scaling      (ProponoVncConnection *conn);
void		    propono_vnc_connection_set_scaling      (ProponoVncConnection *conn,
							     gboolean value);

gboolean	    propono_vnc_connection_get_keep_ratio   (ProponoVncConnection *conn);
void		    propono_vnc_connection_set_keep_ratio   (ProponoVncConnection *conn,
							     gboolean value);

gint		    propono_vnc_connection_get_shared       (ProponoVncConnection *conn);
void		    propono_vnc_connection_set_shared       (ProponoVncConnection *conn,
							     gint value);

gint		    propono_vnc_connection_get_fd           (ProponoVncConnection *conn);
void		    propono_vnc_connection_set_fd           (ProponoVncConnection *conn,
							     gint value);

gint		    propono_vnc_connection_get_depth_profile (ProponoVncConnection *conn);
void		    propono_vnc_connection_set_depth_profile (ProponoVncConnection *conn,
							      gint value);

gboolean	    propono_vnc_connection_get_lossy_encoding (ProponoVncConnection *conn);
void		    propono_vnc_connection_set_lossy_encoding (ProponoVncConnection *conn,
							       gboolean enable);

const gchar*	    propono_vnc_connection_get_ssh_tunnel_host (ProponoVncConnection *conn);
void		    propono_vnc_connection_set_ssh_tunnel_host (ProponoVncConnection *conn,
								const gchar *host);

G_END_DECLS

#endif /* __PROPONO_VNC_CONNECTION_H__  */
/* vim: set ts=8: */
