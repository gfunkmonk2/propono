/*
 * propono-plugins-manager.h
 * This file is part of propono
 *
 * Based on pluma plugin system
 * Copyright (C) 2002-2005 Paolo Maggi
 * 
 * propono-plugins-manager.h is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation, either version 2 of the License, or
 * (at your option) any later version.
 * 
 * propono-plugins-manager.h is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License along
 * with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef __PROPONO_PLUGIN_MANAGER_H__
#define __PROPONO_PLUGIN_MANAGER_H__

#include <gtk/gtk.h>

G_BEGIN_DECLS

/*
 * Type checking and casting macros
 */
#define PROPONO_TYPE_PLUGIN_MANAGER              (propono_plugin_manager_get_type())
#define PROPONO_PLUGIN_MANAGER(obj)              (G_TYPE_CHECK_INSTANCE_CAST((obj), PROPONO_TYPE_PLUGIN_MANAGER, ProponoPluginManager))
#define PROPONO_PLUGIN_MANAGER_CLASS(klass)      (G_TYPE_CHECK_CLASS_CAST((klass), PROPONO_TYPE_PLUGIN_MANAGER, ProponoPluginManagerClass))
#define PROPONO_IS_PLUGIN_MANAGER(obj)           (G_TYPE_CHECK_INSTANCE_TYPE((obj), PROPONO_TYPE_PLUGIN_MANAGER))
#define PROPONO_IS_PLUGIN_MANAGER_CLASS(klass)   (G_TYPE_CHECK_CLASS_TYPE ((klass), PROPONO_TYPE_PLUGIN_MANAGER))
#define PROPONO_PLUGIN_MANAGER_GET_CLASS(obj)    (G_TYPE_INSTANCE_GET_CLASS((obj), PROPONO_TYPE_PLUGIN_MANAGER, ProponoPluginManagerClass))

/* Private structure type */
typedef struct _ProponoPluginManagerPrivate ProponoPluginManagerPrivate;

/*
 * Main object structure
 */
typedef struct _ProponoPluginManager ProponoPluginManager;

struct _ProponoPluginManager 
{
	GtkVBox vbox;

	/*< private > */
	ProponoPluginManagerPrivate *priv;
};

/*
 * Class definition
 */
typedef struct _ProponoPluginManagerClass ProponoPluginManagerClass;

struct _ProponoPluginManagerClass 
{
	GtkVBoxClass parent_class;
};

/*
 * Public methods
 */
GType		 propono_plugin_manager_get_type		(void) G_GNUC_CONST;

GtkWidget	*propono_plugin_manager_new		(void);
   
G_END_DECLS

#endif  /* __PROPONO_PLUGIN_MANAGER_H__  */
