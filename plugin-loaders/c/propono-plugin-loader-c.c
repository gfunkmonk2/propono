/*
 * propono-plugin-loader-c.c
 * This file is part of propono
 *
 * Copyright (C) 2008 - Jesse van den Kieboom
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
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, 
 * Boston, MA 02111-1307, USA. 
 */

#include "propono-plugin-loader-c.h"
#include <propono/propono-object-module.h>

#define PROPONO_PLUGIN_LOADER_C_GET_PRIVATE(object)(G_TYPE_INSTANCE_GET_PRIVATE((object), PROPONO_TYPE_PLUGIN_LOADER_C, ProponoPluginLoaderCPrivate))

struct _ProponoPluginLoaderCPrivate
{
	GHashTable *loaded_plugins;
};

static void propono_plugin_loader_iface_init (gpointer g_iface, gpointer iface_data);

PROPONO_PLUGIN_LOADER_REGISTER_TYPE (ProponoPluginLoaderC, propono_plugin_loader_c, G_TYPE_OBJECT, propono_plugin_loader_iface_init);


static const gchar *
propono_plugin_loader_iface_get_id (void)
{
	return "C";
}

static ProponoPlugin *
propono_plugin_loader_iface_load (ProponoPluginLoader *loader,
				ProponoPluginInfo   *info,
				const gchar       *path)
{
	ProponoPluginLoaderC *cloader = PROPONO_PLUGIN_LOADER_C (loader);
	ProponoObjectModule *module;
	const gchar *module_name;
	ProponoPlugin *result;

	module = (ProponoObjectModule *)g_hash_table_lookup (cloader->priv->loaded_plugins, info);
	module_name = propono_plugin_info_get_module_name (info);

	if (module == NULL)
	{
		/* For now we force all modules to be resident */
		module = propono_object_module_new (module_name,
						  path,
						  "register_propono_plugin",
						  TRUE);

		/* Infos are available for all the lifetime of the loader.
		 * If this changes, we should use weak refs or something */

		g_hash_table_insert (cloader->priv->loaded_plugins, info, module);
	}

	if (!g_type_module_use (G_TYPE_MODULE (module)))
	{
		g_warning ("Could not load plugin module: %s", propono_plugin_info_get_name (info));

		return NULL;
	}

	/* TODO: for now we force data-dir-name = module-name... if needed we can
	 * add a datadir field to the plugin descriptor file.
	 */
	result = (ProponoPlugin *)propono_object_module_new_object (module,
								"install-dir", path,
								"data-dir-name", module_name,
								NULL);

	if (!result)
	{
		g_warning ("Could not create plugin object: %s", propono_plugin_info_get_name (info));
		g_type_module_unuse (G_TYPE_MODULE (module));
		
		return NULL;
	}

	g_type_module_unuse (G_TYPE_MODULE (module));
	
	return result;
}

static void
propono_plugin_loader_iface_unload (ProponoPluginLoader *loader,
				  ProponoPluginInfo   *info)
{
	//ProponoPluginLoaderC *cloader = PROPONO_PLUGIN_LOADER_C (loader);
	
	/* this is a no-op, since the type module will be properly unused as
	   the last reference to the plugin dies. When the plugin is activated
	   again, the library will be reloaded */
}

static void
propono_plugin_loader_iface_init (gpointer g_iface, 
				gpointer iface_data)
{
	ProponoPluginLoaderInterface *iface = (ProponoPluginLoaderInterface *)g_iface;
	
	iface->get_id = propono_plugin_loader_iface_get_id;
	iface->load = propono_plugin_loader_iface_load;
	iface->unload = propono_plugin_loader_iface_unload;
}

static void
propono_plugin_loader_c_finalize (GObject *object)
{
	ProponoPluginLoaderC *cloader = PROPONO_PLUGIN_LOADER_C (object);
	GList *infos;
	GList *item;

	/* FIXME: this sanity check it's not efficient. Let's remove it
	 * once we are confident with the code */

	infos = g_hash_table_get_keys (cloader->priv->loaded_plugins);
	
	for (item = infos; item; item = item->next)
	{
		ProponoPluginInfo *info = (ProponoPluginInfo *)item->data;

		if (propono_plugin_info_is_active (info))
		{
			g_warning ("There are still C plugins loaded during destruction");
			break;
		}
	}

	g_list_free (infos);	

	g_hash_table_destroy (cloader->priv->loaded_plugins);
	
	G_OBJECT_CLASS (propono_plugin_loader_c_parent_class)->finalize (object);
}

static void
propono_plugin_loader_c_class_init (ProponoPluginLoaderCClass *klass)
{
	GObjectClass *object_class = G_OBJECT_CLASS (klass);
	
	object_class->finalize = propono_plugin_loader_c_finalize;

	g_type_class_add_private (object_class, sizeof (ProponoPluginLoaderCPrivate));
}

static void
propono_plugin_loader_c_class_finalize (ProponoPluginLoaderCClass *klass)
{
}

static void
propono_plugin_loader_c_init (ProponoPluginLoaderC *self)
{
	self->priv = PROPONO_PLUGIN_LOADER_C_GET_PRIVATE (self);
	
	/* loaded_plugins maps ProponoPluginInfo to a ProponoObjectModule */
	self->priv->loaded_plugins = g_hash_table_new (g_direct_hash,
						       g_direct_equal);
}

ProponoPluginLoaderC *
propono_plugin_loader_c_new ()
{
	GObject *loader = g_object_new (PROPONO_TYPE_PLUGIN_LOADER_C, NULL);

	return PROPONO_PLUGIN_LOADER_C (loader);
}
