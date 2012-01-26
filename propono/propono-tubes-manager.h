/*
 * propono-tubes-manager.h
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
 *      Danielle Madeley <danielle.madeley@collabora.co.uk>
 */

#ifndef __PROPONO_TUBES_MANAGER_H__
#define __PROPONO_TUBES_MANAGER_H__

#include <glib-object.h>

#include "handler.h"

#include "propono-window.h"

G_BEGIN_DECLS

#define PROPONO_TYPE_TUBES_MANAGER (propono_tubes_manager_get_type())
#define PROPONO_TUBES_MANAGER(obj) (G_TYPE_CHECK_INSTANCE_CAST ((obj),\
    PROPONO_TYPE_TUBES_MANAGER, ProponoTubesManager))
#define PROPONO_IS_TUBES_MANAGER(obj) (G_TYPE_CHECK_INSTANCE_TYPE ((obj),\
    PROPONO_TYPE_TUBES_MANAGER))
#define PROPONO_TUBES_MANAGER_CLASS(klass) (G_TYPE_CHECK_CLASS_CAST ((klass),\
    PROPONO_TYPE_TUBES_MANAGER, ProponoTubesManagerClass))
#define PROPONO_IS_TUBES_MANAGER_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE \
    ((klass), PROPONO_TYPE_TUBES_MANAGER))
#define PROPONO_TUBES_MANAGER_GET_CLASS(obj) (G_TYPE_INSTANCE_GET_CLASS \
    ((obj), PROPONO_TYPE_TUBES_MANAGER, ProponoTubesManagerClass))

typedef struct _ProponoTubesManager ProponoTubesManager;
typedef struct _ProponoTubesManagerClass ProponoTubesManagerClass;


struct _ProponoTubesManager
{
  ProponoHandler parent_instance;
};

struct _ProponoTubesManagerClass
{
  ProponoHandlerClass parent_class;
};

GType propono_tubes_manager_get_type (void) G_GNUC_CONST;
ProponoTubesManager* propono_tubes_manager_new (ProponoWindow
    *propono_window);

G_END_DECLS

#endif
