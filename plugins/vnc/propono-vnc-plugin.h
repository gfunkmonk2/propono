/*
 * propono-vnc-plugin.h
 * This file is part of propono
 *
 * Copyright (C) 2009 Jorge Pereira <jorge@jorgepereira.com.br>
 * 
 * propono-vnc-plugin.h is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation, either version 2 of the License, or
 * (at your option) any later version.
 * 
 * propono-vnc-plugin.h is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License along
 * with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef __PROPONO_VNC_PLUGIN_H__
#define __PROPONO_VNC_PLUGIN_H__

#include <glib.h>
#include <glib-object.h>
#include <propono/propono-plugin.h>

G_BEGIN_DECLS

/*
 * Type checking and casting macros
 */
#define PROPONO_TYPE_VNC_PLUGIN                 (propono_vnc_plugin_get_type ())
#define PROPONO_VNC_PLUGIN(o)                   (G_TYPE_CHECK_INSTANCE_CAST ((o), PROPONO_TYPE_VNC_PLUGIN, ProponoVncPlugin))
#define PROPONO_VNC_PLUGIN_CLASS(k)             (G_TYPE_CHECK_CLASS_CAST((k), PROPONO_TYPE_VNC_PLUGIN, ProponoVncPluginClass))
#define PROPONO_IS_VNC_PLUGIN(o)                (G_TYPE_CHECK_INSTANCE_TYPE ((o), PROPONO_TYPE_VNC_PLUGIN))
#define PROPONO_IS_VNC_PLUGIN_CLASS(k)          (G_TYPE_CHECK_CLASS_TYPE ((k), PROPONO_TYPE_VNC_PLUGIN))
#define PROPONO_VNC_PLUGIN_GET_CLASS(o)         (G_TYPE_INSTANCE_GET_CLASS ((o), PROPONO_TYPE_VNC_PLUGIN, ProponoVncPluginClass))

/* Private structure type */
typedef struct _ProponoVncPluginPrivate	ProponoVncPluginPrivate;

/*
 * Main object structure
 */
typedef struct _ProponoVncPlugin		ProponoVncPlugin;

struct _ProponoVncPlugin
{
  ProponoPlugin parent_instance;
};

/*
 * Class definition
 */
typedef struct _ProponoVncPluginClass	ProponoVncPluginClass;

struct _ProponoVncPluginClass
{
  ProponoPluginClass parent_class;
};

/*
 * Public methods
 */
GType	propono_vnc_plugin_get_type		(void) G_GNUC_CONST;

/* All the plugins must implement this function */
G_MODULE_EXPORT GType register_propono_plugin (GTypeModule *module);

G_END_DECLS

#endif /* __PROPONO_VNC_PLUGIN_H__ */
/* vim: set ts=8: */
