/*
 * propono-plugin-info.h
 * This file is part of propono
 * Based on pluma plugin system
 * Copyright (C) 2002-2005 - Paolo Maggi 
 * Copyright (C) 2007 - Paolo Maggi, Steve Frécinaux
 * 
 * propono-plugin-info.h is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation, either version 2 of the License, or
 * (at your option) any later version.
 * 
 * propono-plugin-info.h is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License along
 * with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef __PROPONO_PLUGIN_INFO_H__
#define __PROPONO_PLUGIN_INFO_H__

#include <glib-object.h>

G_BEGIN_DECLS

#define PROPONO_TYPE_PLUGIN_INFO			(propono_plugin_info_get_type ())
#define PROPONO_PLUGIN_INFO(obj)			((ProponoPluginInfo *) (obj))

typedef struct _ProponoPluginInfo			ProponoPluginInfo;

GType		 propono_plugin_info_get_type		(void) G_GNUC_CONST;

gboolean 	 propono_plugin_info_is_active		(ProponoPluginInfo *info);
gboolean 	 propono_plugin_info_is_available	(ProponoPluginInfo *info);
gboolean	 propono_plugin_info_is_configurable	(ProponoPluginInfo *info);

const gchar	*propono_plugin_info_get_module_name	(ProponoPluginInfo *info);

const gchar	*propono_plugin_info_get_name		(ProponoPluginInfo *info);
const gchar	*propono_plugin_info_get_description	(ProponoPluginInfo *info);
const gchar	*propono_plugin_info_get_icon_name	(ProponoPluginInfo *info);
const gchar    **propono_plugin_info_get_authors	(ProponoPluginInfo *info);
const gchar	*propono_plugin_info_get_website	(ProponoPluginInfo *info);
const gchar	*propono_plugin_info_get_copyright	(ProponoPluginInfo *info);
const gchar	*propono_plugin_info_get_version	(ProponoPluginInfo *info);
gboolean	 propono_plugin_info_is_engine		(ProponoPluginInfo *info);

G_END_DECLS

#endif /* __PROPONO_PLUGIN_INFO_H__ */
/* vim: set ts=8: */
