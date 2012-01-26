/*
 * propono-plugins-engine.h
 * This file is part of propono
 *
 * Based on pluma plugin system
 * Copyright (C) 2002-2005 Paolo Maggi
 * Copyright (C) 2009 Jonh Wendell <wendell@bani.com.br>
 * 
 * propono-plugins-engine.h is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation, either version 2 of the License, or
 * (at your option) any later version.
 * 
 * propono-plugins-engine.h is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License along
 * with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef __PROPONO_PLUGINS_ENGINE_H__
#define __PROPONO_PLUGINS_ENGINE_H__

#include <glib.h>
#include "propono-window.h"
#include "propono-plugin-info.h"
#include "propono-plugin.h"
#include "propono-connection.h"

G_BEGIN_DECLS

#define PROPONO_TYPE_PLUGINS_ENGINE              (propono_plugins_engine_get_type ())
#define PROPONO_PLUGINS_ENGINE(obj)              (G_TYPE_CHECK_INSTANCE_CAST((obj), PROPONO_TYPE_PLUGINS_ENGINE, ProponoPluginsEngine))
#define PROPONO_PLUGINS_ENGINE_CLASS(klass)      (G_TYPE_CHECK_CLASS_CAST((klass), PROPONO_TYPE_PLUGINS_ENGINE, ProponoPluginsEngineClass))
#define PROPONO_IS_PLUGINS_ENGINE(obj)           (G_TYPE_CHECK_INSTANCE_TYPE((obj), PROPONO_TYPE_PLUGINS_ENGINE))
#define PROPONO_IS_PLUGINS_ENGINE_CLASS(klass)   (G_TYPE_CHECK_CLASS_TYPE ((klass), PROPONO_TYPE_PLUGINS_ENGINE))
#define PROPONO_PLUGINS_ENGINE_GET_CLASS(obj)    (G_TYPE_INSTANCE_GET_CLASS((obj), PROPONO_TYPE_PLUGINS_ENGINE, ProponoPluginsEngineClass))

typedef struct _ProponoPluginsEngine		ProponoPluginsEngine;
typedef struct _ProponoPluginsEnginePrivate	ProponoPluginsEnginePrivate;

struct _ProponoPluginsEngine
{
  GObject parent;
  ProponoPluginsEnginePrivate *priv;
};

typedef struct _ProponoPluginsEngineClass	ProponoPluginsEngineClass;

struct _ProponoPluginsEngineClass
{
  GObjectClass parent_class;

  void	 (* activate_plugin)		(ProponoPluginsEngine *engine,
					 ProponoPluginInfo    *info);

  void	 (* deactivate_plugin)		(ProponoPluginsEngine *engine,
					 ProponoPluginInfo    *info);
};

GType			 propono_plugins_engine_get_type		(void) G_GNUC_CONST;

ProponoPluginsEngine	*propono_plugins_engine_get_default		(void);

void			 propono_plugins_engine_garbage_collect		(ProponoPluginsEngine *engine);

const GSList		*propono_plugins_engine_get_plugin_list 	(ProponoPluginsEngine *engine);

ProponoPluginInfo	*propono_plugins_engine_get_plugin_info		(ProponoPluginsEngine *engine,
									 const gchar        *name);

/* plugin load and unloading (overall, for all windows) */
gboolean 		 propono_plugins_engine_activate_plugin 	(ProponoPluginsEngine *engine,
									 ProponoPluginInfo    *info);
gboolean 		 propono_plugins_engine_deactivate_plugin	(ProponoPluginsEngine *engine,
									 ProponoPluginInfo    *info);

void		 	 propono_plugins_engine_configure_plugin	(ProponoPluginsEngine *engine,
									 ProponoPluginInfo    *info,
									 GtkWindow            *parent);

/* plugin activation/deactivation per window, private to ProponoWindow */
void 			 propono_plugins_engine_activate_plugins 	 (ProponoPluginsEngine *engine,
									  ProponoWindow        *window);
void 			 propono_plugins_engine_deactivate_plugins	 (ProponoPluginsEngine *engine,
									  ProponoWindow        *window);
void			 propono_plugins_engine_update_plugins_ui	 (ProponoPluginsEngine *engine,
									  ProponoWindow        *window);

/* private for mateconf notification */
void			 propono_plugins_engine_active_plugins_changed	(ProponoPluginsEngine *engine);

void			 propono_plugins_engine_rescan_plugins		(ProponoPluginsEngine *engine);

GHashTable		*propono_plugin_engine_get_plugins_by_protocol	(ProponoPluginsEngine *engine);
ProponoPlugin		*propono_plugins_engine_get_plugin_by_protocol	(ProponoPluginsEngine *engine,
									 const gchar          *protocol);

G_END_DECLS

#endif  /* __PROPONO_PLUGINS_ENGINE_H__ */
