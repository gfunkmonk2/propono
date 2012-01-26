/*
 * propono-plugin-info-priv.h
 * This file is part of propono
 *
 * Based on pluma plugin system
 * Copyright (C) 2002-2005 - Paolo Maggi 
 * Copyright (C) 2007 - Paolo Maggi, Steve Frécinaux
 * 
 * propono-plugin-info-priv.h is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation, either version 2 of the License, or
 * (at your option) any later version.
 * 
 * propono-plugin-info-priv.h is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License along
 * with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef __PROPONO_PLUGIN_INFO_PRIV_H__
#define __PROPONO_PLUGIN_INFO_PRIV_H__

#include "propono-plugin-info.h"
#include "propono-plugin.h"

struct _ProponoPluginInfo
{
  gint               refcount;

  ProponoPlugin     *plugin;
  gchar             *file;

  gchar             *module_name;
  gchar	            *loader;
  gchar            **dependencies;

  gchar             *name;
  gchar             *desc;
  gchar             *icon_name;
  gchar            **authors;
  gchar             *copyright;
  gchar             *website;
  gchar             *version;

  /* A plugin is unavailable if it is not possible to activate it
     due to an error loading the plugin module (e.g. for Python plugins
     when the interpreter has not been correctly initializated) */
  gint               available : 1;
  gint               engine : 1;
};

ProponoPluginInfo	*_propono_plugin_info_new	(const gchar *file);
void			 _propono_plugin_info_ref	(ProponoPluginInfo *info);
void			 _propono_plugin_info_unref	(ProponoPluginInfo *info);

#endif /* __PROPONO_PLUGIN_INFO_PRIV_H__ */
/* vim: set ts=8: */
