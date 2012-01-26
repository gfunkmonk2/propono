/*
 * propono-ssh-plugin.h
 * This file is part of propono
 *
 * Copyright (C) 2009 Jonh Wendell <wendell@bani.com.br>
 * 
 * propono-ssh-plugin.h is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation, either version 2 of the License, or
 * (at your option) any later version.
 * 
 * propono-ssh-plugin.h is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License along
 * with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef __PROPONO_SSH_PLUGIN_H__
#define __PROPONO_SSH_PLUGIN_H__

#include <glib.h>
#include <glib-object.h>
#include <propono/propono-plugin.h>

G_BEGIN_DECLS

/*
 * Type checking and casting macros
 */
#define PROPONO_TYPE_SSH_PLUGIN                 (propono_ssh_plugin_get_type ())
#define PROPONO_SSH_PLUGIN(o)                   (G_TYPE_CHECK_INSTANCE_CAST ((o), PROPONO_TYPE_SSH_PLUGIN, ProponoSshPlugin))
#define PROPONO_SSH_PLUGIN_CLASS(k)             (G_TYPE_CHECK_CLASS_CAST((k), PROPONO_TYPE_SSH_PLUGIN, ProponoSshPluginClass))
#define PROPONO_IS_SSH_PLUGIN(o)                (G_TYPE_CHECK_INSTANCE_TYPE ((o), PROPONO_TYPE_SSH_PLUGIN))
#define PROPONO_IS_SSH_PLUGIN_CLASS(k)          (G_TYPE_CHECK_CLASS_TYPE ((k), PROPONO_TYPE_SSH_PLUGIN))
#define PROPONO_SSH_PLUGIN_GET_CLASS(o)         (G_TYPE_INSTANCE_GET_CLASS ((o), PROPONO_TYPE_SSH_PLUGIN, ProponoSshPluginClass))

/* Private structure type */
typedef struct _ProponoSshPluginPrivate	ProponoSshPluginPrivate;

/*
 * Main object structure
 */
typedef struct _ProponoSshPlugin		ProponoSshPlugin;

struct _ProponoSshPlugin
{
  ProponoPlugin parent_instance;
};

/*
 * Class definition
 */
typedef struct _ProponoSshPluginClass	ProponoSshPluginClass;

struct _ProponoSshPluginClass
{
  ProponoPluginClass parent_class;
};

/*
 * Public methods
 */
GType	propono_ssh_plugin_get_type		(void) G_GNUC_CONST;

/* All the plugins must implement this function */
G_MODULE_EXPORT GType register_propono_plugin (GTypeModule *module);

G_END_DECLS

#endif /* __PROPONO_SSH_PLUGIN_H__ */
/* vim: set ts=8: */
