/*
 * propono-connection.h
 * Abstract base class for all types of connections: VNC, RDP, etc.
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

#ifndef __PROPONO_CONNECTION_H__
#define __PROPONO_CONNECTION_H__

#include <glib.h>
#include <gtk/gtk.h>
#include <libxml/xmlwriter.h>

G_BEGIN_DECLS

#define PROPONO_TYPE_CONNECTION             (propono_connection_get_type ())
#define PROPONO_CONNECTION(obj)             (G_TYPE_CHECK_INSTANCE_CAST ((obj), PROPONO_TYPE_CONNECTION, ProponoConnection))
#define PROPONO_CONNECTION_CLASS(klass)     (G_TYPE_CHECK_CLASS_CAST ((klass), PROPONO_TYPE_CONNECTION, ProponoConnectionClass))
#define PROPONO_IS_CONNECTION(obj)          (G_TYPE_CHECK_INSTANCE_TYPE ((obj), PROPONO_TYPE_CONNECTION))
#define PROPONO_IS_CONNECTION_CLASS(klass)  (G_TYPE_CHECK_CLASS_TYPE ((klass), PROPONO_TYPE_CONNECTION))
#define PROPONO_CONNECTION_GET_CLASS(obj)   (G_TYPE_INSTANCE_GET_CLASS ((obj), PROPONO_TYPE_CONNECTION, ProponoConnectionClass))

typedef struct _ProponoConnectionClass   ProponoConnectionClass;
typedef struct _ProponoConnection        ProponoConnection;
typedef struct _ProponoConnectionPrivate ProponoConnectionPrivate;

struct _ProponoConnectionClass
{
  GObjectClass	parent_class;

  /* Virtual functions */
  void		(*impl_fill_writer)		(ProponoConnection *conn, xmlTextWriter *writer);
  void		(*impl_parse_item)		(ProponoConnection *conn, xmlNode *root);
  gchar *	(*impl_get_best_name)		(ProponoConnection *conn);
  void		(*impl_fill_conn_from_file)	(ProponoConnection *conn, GKeyFile *file);
  void		(*impl_parse_options_widget)	(ProponoConnection *conn, GtkWidget *widget);
};

struct _ProponoConnection
{
  GObject parent_instance;
  ProponoConnectionPrivate *priv;
};


GType propono_connection_get_type (void) G_GNUC_CONST;

const gchar*	    propono_connection_get_protocol	(ProponoConnection *conn);
void		    propono_connection_set_protocol	(ProponoConnection *conn,
							 const gchar       *protocol);

const gchar*	    propono_connection_get_host		(ProponoConnection *conn);
void		    propono_connection_set_host		(ProponoConnection *conn,
							 const gchar *host);

gint		    propono_connection_get_port		(ProponoConnection *conn);
void		    propono_connection_set_port		(ProponoConnection *conn,
							 gint port);

const gchar*	    propono_connection_get_username	(ProponoConnection *conn);
void		    propono_connection_set_username	(ProponoConnection *conn,
							 const gchar *username);

const gchar*	    propono_connection_get_password	(ProponoConnection *conn);
void		    propono_connection_set_password	(ProponoConnection *conn,
							 const gchar *password);

const gchar*	    propono_connection_get_name		(ProponoConnection *conn);
void		    propono_connection_set_name		(ProponoConnection *conn,
							 const gchar *name);

gboolean	    propono_connection_get_fullscreen	(ProponoConnection *conn);
void		    propono_connection_set_fullscreen	(ProponoConnection *conn,
							 gboolean value);

ProponoConnection*  propono_connection_new_from_string	(const gchar *url, gchar **error_msg, gboolean use_bookmarks);
ProponoConnection*  propono_connection_new_from_file	(const gchar *uri, gchar **error_msg, gboolean use_bookmarks);

gboolean	    propono_connection_split_string	(const gchar *uri,
							 const gchar *known_protocol,
							 gchar       **protocol,
							 gchar       **host,
							 gint        *port,
							 gchar       **error_msg);

gchar*		    propono_connection_get_string_rep	(ProponoConnection *conn,
							 gboolean has_protocol);

/* Methods that can be overrided */

/* propono_connection_fill_writer(): Used to fill a xml writer when saving bookmarks.
   subclasses must inherit from it and call super() */
void                propono_connection_fill_writer	(ProponoConnection *conn,
							 xmlTextWriter *writer);

/* propono_connection_parse_item(): Used to parse a xml item when loading bookmarks.
   subclasses must inherit from it and call super() */
void                propono_connection_parse_item	(ProponoConnection *conn,
							 xmlNode *root);

gchar*		    propono_connection_get_best_name    (ProponoConnection *conn);

void                propono_connection_parse_options_widget (ProponoConnection *conn, GtkWidget *widget);

G_END_DECLS

#endif /* __PROPONO_CONNECTION_H__  */
/* vim: set ts=8: */
