/*
 * propono-vnc-listener.h
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

#ifndef __PROPONO_VNC_LISTENER_H__
#define __PROPONO_VNC_LISTENER_H__

#include <glib.h>

G_BEGIN_DECLS

#define PROPONO_TYPE_VNC_LISTENER             (propono_vnc_listener_get_type ())
#define PROPONO_VNC_LISTENER(obj)             (G_TYPE_CHECK_INSTANCE_CAST ((obj), PROPONO_TYPE_VNC_LISTENER, ProponoVncListener))
#define PROPONO_VNC_LISTENER_CLASS(klass)     (G_TYPE_CHECK_CLASS_CAST ((klass), PROPONO_TYPE_VNC_LISTENER, ProponoVncListenerClass))
#define PROPONO_IS_VNC_LISTENER(obj)          (G_TYPE_CHECK_INSTANCE_TYPE ((obj), PROPONO_TYPE_VNC_LISTENER))
#define PROPONO_IS_VNC_LISTENER_CLASS(klass)  (G_TYPE_CHECK_CLASS_TYPE ((klass), PROPONO_TYPE_VNC_LISTENER))
#define PROPONO_VNC_LISTENER_GET_CLASS(obj)   (G_TYPE_INSTANCE_GET_CLASS ((obj), PROPONO_TYPE_VNC_LISTENER, ProponoVncListenerClass))

typedef struct _ProponoVncListenerClass   ProponoVncListenerClass;
typedef struct _ProponoVncListener        ProponoVncListener;
typedef struct _ProponoVncListenerPrivate ProponoVncListenerPrivate;

struct _ProponoVncListenerClass
{
  GObjectClass parent_class;
};

struct _ProponoVncListener
{
  GObject parent_instance;
  ProponoVncListenerPrivate *priv;
};


GType propono_vnc_listener_get_type (void) G_GNUC_CONST;

ProponoVncListener*	propono_vnc_listener_get_default (void);
void			propono_vnc_listener_start (ProponoVncListener *listener);
void			propono_vnc_listener_stop  (ProponoVncListener *listener);
gboolean		propono_vnc_listener_is_listening (ProponoVncListener *listener);
gint			propono_vnc_listener_get_port (ProponoVncListener *listener);

G_END_DECLS

#endif /* __PROPONO_VNC_LISTENER_H__  */
/* vim: set ts=8: */
