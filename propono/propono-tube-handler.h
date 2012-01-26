/*
 * propono-tube-handler.h
 * This file is part of propono
 *
 * © 2009, Collabora Ltd
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
 *
 * Authors:
 *      Arnaud Maillet <arnaud.maillet@collabora.co.uk>
 */

#ifndef __PROPONO_TUBE_HANDLER_H__
#define __PROPONO_TUBE_HANDLER_H__

#include <glib-object.h>

#include <telepathy-glib/channel.h>

#include "propono-window.h"

G_BEGIN_DECLS

#define PROPONO_TYPE_TUBE_HANDLER (propono_tube_handler_get_type())
#define PROPONO_TUBE_HANDLER(obj) (G_TYPE_CHECK_INSTANCE_CAST ((obj),\
    PROPONO_TYPE_TUBE_HANDLER, ProponoTubeHandler))
#define PROPONO_IS_TUBE_HANDLER(obj) (G_TYPE_CHECK_INSTANCE_TYPE ((obj),\
    PROPONO_TYPE_TUBE_HANDLER))
#define PROPONO_TUBE_HANDLER_CLASS(klass) (G_TYPE_CHECK_CLASS_CAST ((klass),\
    PROPONO_TYPE_TUBE_HANDLER, ProponoTubeHandlerClass))
#define PROPONO_IS_TUBE_HANDLER_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE \
    ((klass), PROPONO_TYPE_TUBE_HANDLER))
#define PROPONO_TUBE_HANDLER_GET_CLASS(obj) (G_TYPE_INSTANCE_GET_CLASS \
    ((obj), PROPONO_TYPE_TUBE_HANDLER, ProponoTubeHandlerClass))

typedef struct _ProponoTubeHandler ProponoTubeHandler;
typedef struct _ProponoTubeHandlerClass ProponoTubeHandlerClass;


struct _ProponoTubeHandler
{
  GObject parent_instance;
};

struct _ProponoTubeHandlerClass
{
  GObjectClass parent_class;

  void (* disconnected) (ProponoTubeHandler *htube);
};

GType propono_tube_handler_get_type (void) G_GNUC_CONST;
ProponoTubeHandler* propono_tube_handler_new (ProponoWindow *propono_window,
    TpChannel *channel);

G_END_DECLS

#endif
