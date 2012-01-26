/*
 * propono-plugins-engine.c
 * This file is part of propono
 *
 * Based on pluma plugin system
 * Copyright (C) 2002-2005 Paolo Maggi
 * Copyright (C) 2009 Jonh Wendell <wendell@bani.com.br>s
 * 
 * propono-plugins-engine.c is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation, either version 2 of the License, or
 * (at your option) any later version.
 * 
 * propono-plugins-engine.c is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License along
 * with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <string.h>
#include <glib/gi18n.h>

#include "propono-plugins-engine.h"
#include "propono-plugin-info-priv.h"
#include "propono-plugin.h"
#include "propono-debug.h"
#include "propono-app.h"
#include "propono-prefs.h"
#include "propono-plugin-loader.h"
#include "propono-object-module.h"
#include "propono-dirs.h"

#define PROPONO_PLUGINS_ENGINE_BASE_KEY "/apps/propono/plugins"
#define PROPONO_PLUGINS_ENGINE_KEY PROPONO_PLUGINS_ENGINE_BASE_KEY "/active-plugins"

#define PLUGIN_EXT	".propono-plugin"
#define LOADER_EXT	G_MODULE_SUFFIX

typedef struct
{
  ProponoPluginLoader *loader;
  ProponoObjectModule *module;
} LoaderInfo;

/* Signals */
enum
{
  ACTIVATE_PLUGIN,
  DEACTIVATE_PLUGIN,
  LAST_SIGNAL
};

static guint signals[LAST_SIGNAL];

G_DEFINE_TYPE(ProponoPluginsEngine, propono_plugins_engine, G_TYPE_OBJECT)

struct _ProponoPluginsEnginePrivate
{
  GSList *plugin_list;
  GHashTable *loaders;
  GHashTable *protocols;

  gboolean activate_from_prefs;
};

ProponoPluginsEngine *default_engine = NULL;

static void	propono_plugins_engine_activate_plugin_real (ProponoPluginsEngine *engine,
							     ProponoPluginInfo    *info);
static void	propono_plugins_engine_deactivate_plugin_real (ProponoPluginsEngine *engine,
							       ProponoPluginInfo    *info);

typedef gboolean (*LoadDirCallback)(ProponoPluginsEngine *engine, const gchar *filename, gpointer userdata);

static gboolean
load_dir_real (ProponoPluginsEngine *engine,
	       const gchar          *dir,
	       const gchar          *suffix,
	       LoadDirCallback      callback,
	       gpointer             userdata)
{
	GError *error = NULL;
	GDir *d;
	const gchar *dirent;
	gboolean ret = TRUE;

	g_return_val_if_fail (dir != NULL, TRUE);

	propono_debug_message (DEBUG_PLUGINS, "DIR: %s", dir);

	d = g_dir_open (dir, 0, &error);
	if (!d)
	{
		g_warning ("%s", error->message);
		g_error_free (error);
		return TRUE;
	}
	
	while ((dirent = g_dir_read_name (d)))
	{
		gchar *filename;

		if (!g_str_has_suffix (dirent, suffix))
			continue;

		filename = g_build_filename (dir, dirent, NULL);

		ret = callback (engine, filename, userdata);

		g_free (filename);

		if (!ret)
			break;
	}

	g_dir_close (d);
	return ret;
}

static gboolean
load_plugin_info (ProponoPluginsEngine *engine,
            		  const gchar        *filename,
            		  gpointer            userdata)
{
	ProponoPluginInfo *info;
	
	info = _propono_plugin_info_new (filename);

	if (info == NULL)
		return TRUE;

	/* If a plugin with this name has already been loaded
	 * drop this one (user plugins override system plugins) */
	if (propono_plugins_engine_get_plugin_info (engine, propono_plugin_info_get_module_name (info)) != NULL)
	{
		propono_debug_message (DEBUG_PLUGINS, "Two or more plugins named '%s'. "
				     "Only the first will be considered.\n",
				     propono_plugin_info_get_module_name (info));

		_propono_plugin_info_unref (info);

		return TRUE;
	}

	engine->priv->plugin_list = g_slist_prepend (engine->priv->plugin_list, info);

	propono_debug_message (DEBUG_PLUGINS, "Plugin %s loaded", info->name);
	return TRUE;
}

static void
load_all_plugins (ProponoPluginsEngine *engine)
{
	gchar *plugin_dir;
	const gchar *pdirs_env = NULL;

	/* load user plugins */
	plugin_dir = propono_dirs_get_user_plugins_dir ();
	if (g_file_test (plugin_dir, G_FILE_TEST_IS_DIR))
	{
		load_dir_real (engine,
			       plugin_dir,
			       PLUGIN_EXT,
			       load_plugin_info,
			       NULL);

	}
	g_free (plugin_dir);

	/* load system plugins */
	pdirs_env = g_getenv ("PROPONO_PLUGINS_PATH");

	propono_debug_message (DEBUG_PLUGINS, "PROPONO_PLUGINS_PATH=%s", pdirs_env);

	if (pdirs_env != NULL)
	{
		gchar **pdirs;
		gint i;

		pdirs = g_strsplit (pdirs_env, G_SEARCHPATH_SEPARATOR_S, 0);

		for (i = 0; pdirs[i] != NULL; i++)
		{
			if (!load_dir_real (engine,
					    pdirs[i],
					    PLUGIN_EXT,
					    load_plugin_info,
					    NULL))
			{
				break;
			}
		}

		g_strfreev (pdirs);
	}
	else
	{
		plugin_dir = propono_dirs_get_propono_plugins_dir ();

		load_dir_real (engine,
			       plugin_dir,
			       PLUGIN_EXT,
			       load_plugin_info,
			       NULL);

		g_free (plugin_dir);
	}
}

static guint
hash_lowercase (gconstpointer data)
{
	gchar *lowercase;
	guint ret;
	
	lowercase = g_ascii_strdown ((const gchar *)data, -1);
	ret = g_str_hash (lowercase);
	g_free (lowercase);
	
	return ret;
}

static gboolean
equal_lowercase (gconstpointer a, gconstpointer b)
{
	return g_ascii_strcasecmp ((const gchar *)a, (const gchar *)b) == 0;
}

static void
loader_destroy (LoaderInfo *info)
{
	if (!info)
		return;
	
	if (info->loader)
		g_object_unref (info->loader);
	
	g_free (info);
}

static void
add_loader (ProponoPluginsEngine *engine,
      	    const gchar        *loader_id,
      	    ProponoObjectModule  *module)
{
	LoaderInfo *info;

	info = g_new (LoaderInfo, 1);
	info->loader = NULL;
	info->module = module;

	g_hash_table_insert (engine->priv->loaders, g_strdup (loader_id), info);
}

static void
free_plugin_list (gpointer data, gpointer user_data G_GNUC_UNUSED)
{
  g_free (data);
}

static void
activate_engine_plugins (ProponoPluginsEngine *engine)
{
  GSList *active_plugins, *l;

  propono_debug_message (DEBUG_PLUGINS, "Activating engine plugins");

  g_object_get (propono_prefs_get_default (),
		"active-plugins", &active_plugins,
		NULL);

  for (l = engine->priv->plugin_list; l; l = l->next)
    {
      ProponoPluginInfo *info = (ProponoPluginInfo*)l->data;
		
      if (g_slist_find_custom (active_plugins,
			       propono_plugin_info_get_module_name (info),
			       (GCompareFunc)strcmp) == NULL)
	continue;
		
      if (propono_plugin_info_is_engine (info))
	propono_plugins_engine_activate_plugin (engine, info);
    }

  g_slist_foreach (active_plugins, free_plugin_list, NULL);
  g_slist_free (active_plugins);
}

static void
propono_plugins_engine_init (ProponoPluginsEngine *engine)
{
  propono_debug (DEBUG_PLUGINS);

  if (!g_module_supported ())
    {
      g_warning ("propono is not able to initialize the plugins engine.");
      return;
    }

  engine->priv = G_TYPE_INSTANCE_GET_PRIVATE (engine,
					      PROPONO_TYPE_PLUGINS_ENGINE,
					      ProponoPluginsEnginePrivate);

  load_all_plugins (engine);

  /* make sure that the first reactivation will read active plugins
     from the prefs */
  engine->priv->activate_from_prefs = TRUE;

  /* mapping from loadername -> loader object */
  engine->priv->loaders = g_hash_table_new_full (hash_lowercase,
						 equal_lowercase,
						 (GDestroyNotify)g_free,
						 (GDestroyNotify)loader_destroy);

  engine->priv->protocols = g_hash_table_new (g_str_hash, g_str_equal);
  activate_engine_plugins (engine);
}

static void
loader_garbage_collect (const char *id, 
                        LoaderInfo *info)
{
	if (info->loader)
		propono_plugin_loader_garbage_collect (info->loader);
}

void
propono_plugins_engine_garbage_collect (ProponoPluginsEngine *engine)
{
	g_hash_table_foreach (engine->priv->loaders,
			      (GHFunc) loader_garbage_collect,
			      NULL);
}

static void
propono_plugins_engine_finalize (GObject *object)
{
	ProponoPluginsEngine *engine = PROPONO_PLUGINS_ENGINE (object);
	GSList *item;
	
	propono_debug (DEBUG_PLUGINS);

	/* Firs deactivate all plugins */
	for (item = engine->priv->plugin_list; item; item = item->next)
	{
		ProponoPluginInfo *info = PROPONO_PLUGIN_INFO (item->data);
		
		if (propono_plugin_info_is_active (info))
			propono_plugins_engine_deactivate_plugin_real (engine, info);
	}
	
	/* unref the loaders */	
	g_hash_table_destroy (engine->priv->loaders);

	g_hash_table_destroy (engine->priv->protocols);

	/* and finally free the infos */
	for (item = engine->priv->plugin_list; item; item = item->next)
	{
		ProponoPluginInfo *info = PROPONO_PLUGIN_INFO (item->data);

		_propono_plugin_info_unref (info);
	}

	g_slist_free (engine->priv->plugin_list);

	G_OBJECT_CLASS (propono_plugins_engine_parent_class)->finalize (object);
}

static void
propono_plugins_engine_class_init (ProponoPluginsEngineClass *klass)
{
	GType the_type = G_TYPE_FROM_CLASS (klass);
	GObjectClass *object_class = G_OBJECT_CLASS (klass);

	object_class->finalize = propono_plugins_engine_finalize;
	klass->activate_plugin = propono_plugins_engine_activate_plugin_real;
	klass->deactivate_plugin = propono_plugins_engine_deactivate_plugin_real;

	signals[ACTIVATE_PLUGIN] =
		g_signal_new ("activate-plugin",
			      the_type,
			      G_SIGNAL_RUN_LAST,
			      G_STRUCT_OFFSET (ProponoPluginsEngineClass, activate_plugin),
			      NULL, NULL,
			      g_cclosure_marshal_VOID__BOXED,
			      G_TYPE_NONE,
			      1,
			      PROPONO_TYPE_PLUGIN_INFO | G_SIGNAL_TYPE_STATIC_SCOPE);

	signals[DEACTIVATE_PLUGIN] =
		g_signal_new ("deactivate-plugin",
			      the_type,
			      G_SIGNAL_RUN_LAST,
			      G_STRUCT_OFFSET (ProponoPluginsEngineClass, deactivate_plugin),
			      NULL, NULL,
			      g_cclosure_marshal_VOID__BOXED,
			      G_TYPE_NONE,
			      1,
			      PROPONO_TYPE_PLUGIN_INFO | G_SIGNAL_TYPE_STATIC_SCOPE);

	g_type_class_add_private (klass, sizeof (ProponoPluginsEnginePrivate));
}

static gboolean
load_loader (ProponoPluginsEngine *engine,
      	     const gchar        *filename,
	           gpointer		 data)
{
	ProponoObjectModule *module;
	gchar *base;
	gchar *path;
	const gchar *id;
	GType type;
	
	/* try to load in the module */
	path = g_path_get_dirname (filename);
	base = g_path_get_basename (filename);

	/* for now they are all resident */
	module = propono_object_module_new (base,
					  path,
					  "register_propono_plugin_loader",
					  TRUE);

	g_free (base);
	g_free (path);

	/* make sure to load the type definition */
	if (!g_type_module_use (G_TYPE_MODULE (module)))
	{
		g_object_unref (module);
		g_warning ("Plugin loader module `%s' could not be loaded", filename);

		return TRUE;
	}

	/* get the exported type and check the name as exported by the 
	 * loader interface */
	type = propono_object_module_get_object_type (module);
	id = propono_plugin_loader_type_get_id (type);
	
	add_loader (engine, id, module);	
	g_type_module_unuse (G_TYPE_MODULE (module));

	return TRUE;
}

static void
ensure_loader (LoaderInfo *info)
{
	if (info->loader == NULL && info->module != NULL)
	{
		/* create a new loader object */
		ProponoPluginLoader *loader;
		loader = (ProponoPluginLoader *)propono_object_module_new_object (info->module, NULL);
	
		if (loader == NULL || !PROPONO_IS_PLUGIN_LOADER (loader))
		{
			g_warning ("Loader object is not a valid ProponoPluginLoader instance");
		
			if (loader != NULL && G_IS_OBJECT (loader))
				g_object_unref (loader);
		}
		else
		{
			info->loader = loader;
		}
	}
}

static ProponoPluginLoader *
get_plugin_loader (ProponoPluginsEngine *engine, 
                   ProponoPluginInfo *info)
{
	const gchar *loader_id;
	LoaderInfo *loader_info;

	loader_id = info->loader;

	loader_info = (LoaderInfo *)g_hash_table_lookup (
			engine->priv->loaders, 
			loader_id);

	if (loader_info == NULL)
	{
		gchar *loader_dir;

		loader_dir = propono_dirs_get_propono_plugin_loaders_dir ();

		/* loader could not be found in the hash, try to find it by 
		   scanning */
		load_dir_real (engine, 
			       loader_dir,
			       LOADER_EXT,
			       (LoadDirCallback)load_loader,
			       NULL);
		g_free (loader_dir);
		
		loader_info = (LoaderInfo *)g_hash_table_lookup (
				engine->priv->loaders, 
				loader_id);
	}

	if (loader_info == NULL)
	{
		/* cache non-existent so we don't scan again */
		add_loader (engine, loader_id, NULL);
		return NULL;
	}
	
	ensure_loader (loader_info);
	return loader_info->loader;
}

ProponoPluginsEngine *
propono_plugins_engine_get_default (void)
{
  if (default_engine != NULL)
    return default_engine;

  default_engine = PROPONO_PLUGINS_ENGINE (g_object_new (PROPONO_TYPE_PLUGINS_ENGINE, NULL));
  g_object_add_weak_pointer (G_OBJECT (default_engine),
			     (gpointer) &default_engine);
  return default_engine;
}

const GSList *
propono_plugins_engine_get_plugin_list (ProponoPluginsEngine *engine)
{
  propono_debug (DEBUG_PLUGINS);
  return engine->priv->plugin_list;
}

static gint
compare_plugin_info_and_name (ProponoPluginInfo *info,
			      const gchar *module_name)
{
  return strcmp (propono_plugin_info_get_module_name (info), module_name);
}

ProponoPluginInfo *
propono_plugins_engine_get_plugin_info (ProponoPluginsEngine *engine,
					const gchar          *name)
{
  GSList *l = g_slist_find_custom (engine->priv->plugin_list,
				   name,
				   (GCompareFunc) compare_plugin_info_and_name);

  return l == NULL ? NULL : (ProponoPluginInfo *) l->data;
}

static void
save_active_plugin_list (ProponoPluginsEngine *engine)
{
  GSList *l, *active_plugins = NULL;

  for (l = engine->priv->plugin_list; l != NULL; l = l->next)
    {
      ProponoPluginInfo *info = (ProponoPluginInfo *) l->data;

      if (propono_plugin_info_is_active (info))
	active_plugins = g_slist_prepend (active_plugins,
					  (gpointer)propono_plugin_info_get_module_name (info));
    }

  g_object_set (propono_prefs_get_default (),
		"active-plugins", active_plugins,
		NULL);

  g_slist_free (active_plugins);
}

static gboolean
load_plugin (ProponoPluginsEngine *engine,
      	     ProponoPluginInfo    *info)
{
	ProponoPluginLoader *loader;
	gchar *path;

	if (propono_plugin_info_is_active (info))
		return TRUE;
	
	if (!propono_plugin_info_is_available (info))
		return FALSE;

	loader = get_plugin_loader (engine, info);
	
	if (loader == NULL)
	{
		g_warning ("Could not find loader `%s' for plugin `%s'", info->loader, info->name);
		info->available = FALSE;
		return FALSE;
	}
	
	path = g_path_get_dirname (info->file);
	g_return_val_if_fail (path != NULL, FALSE);

	info->plugin = propono_plugin_loader_load (loader, info, path);
	
	g_free (path);
	
	if (info->plugin == NULL)
	{
		g_warning ("Error loading plugin '%s'", info->name);
		info->available = FALSE;
		return FALSE;
	}

	g_object_set_data (G_OBJECT (info->plugin), "info", info);

	return TRUE;
}

static void
propono_plugins_engine_activate_plugin_real (ProponoPluginsEngine *engine,
					     ProponoPluginInfo    *info)
{
  const GList *wins;
  const gchar *protocol;
  ProponoPluginInfo *plugin_protocol;

  if (!load_plugin (engine, info))
    return;

  if (propono_plugin_info_is_engine (info))
    {
      protocol = propono_plugin_get_protocol (info->plugin);
      plugin_protocol = g_hash_table_lookup (engine->priv->protocols, protocol);
      if (plugin_protocol)
	{
	  g_warning ("The protocol %s was already registered by the plugin %s",
		     protocol,
		     propono_plugin_info_get_name (plugin_protocol));
	  return;
	}

      g_hash_table_insert (engine->priv->protocols, (gpointer)protocol, info->plugin);
      return;
    }

  /* activate plugin for all windows */
  wins = propono_app_get_windows (propono_app_get_default ());
  for (; wins != NULL; wins = wins->next)
    propono_plugin_activate (info->plugin, PROPONO_WINDOW (wins->data));
}

gboolean
propono_plugins_engine_activate_plugin (ProponoPluginsEngine *engine,
					ProponoPluginInfo    *info)
{
  propono_debug (DEBUG_PLUGINS);

  g_return_val_if_fail (info != NULL, FALSE);

  if (!propono_plugin_info_is_available (info))
    return FALSE;
		
  if (propono_plugin_info_is_active (info))
    return TRUE;

  g_signal_emit (engine, signals[ACTIVATE_PLUGIN], 0, info);

  if (propono_plugin_info_is_active (info))
    save_active_plugin_list (engine);

  return propono_plugin_info_is_active (info);
}

static void
call_plugin_deactivate (ProponoPlugin *plugin, 
			ProponoWindow *window)
{
  propono_plugin_deactivate (plugin, window);

  /* ensure update of ui manager, because we suspect it does something
     with expected static strings in the type module (when unloaded the
     strings don't exist anymore, and ui manager updates in an idle
     func) */
  gtk_ui_manager_ensure_update (propono_window_get_ui_manager (window));
}

static void
propono_plugins_engine_deactivate_plugin_real (ProponoPluginsEngine *engine,
					       ProponoPluginInfo    *info)
{
  const GList *wins;
  ProponoPluginLoader *loader;

  if (!propono_plugin_info_is_active (info) || 
      !propono_plugin_info_is_available (info))
    return;

  if (propono_plugin_info_is_engine (info))
    {
      g_hash_table_remove (engine->priv->protocols,
			   propono_plugin_get_protocol (info->plugin));
      propono_plugin_deactivate (info->plugin, NULL);
    }
  else
    {
      wins = propono_app_get_windows (propono_app_get_default ());
      for (; wins != NULL; wins = wins->next)
	call_plugin_deactivate (info->plugin, PROPONO_WINDOW (wins->data));
    }

  /* first unref the plugin (the loader still has one) */
  g_object_unref (info->plugin);
	
  /* find the loader and tell it to gc and unload the plugin */
  loader = get_plugin_loader (engine, info);
	
  propono_plugin_loader_garbage_collect (loader);
  propono_plugin_loader_unload (loader, info);
	
  info->plugin = NULL;
}

gboolean
propono_plugins_engine_deactivate_plugin (ProponoPluginsEngine *engine,
					  ProponoPluginInfo    *info)
{
  propono_debug (DEBUG_PLUGINS);

  g_return_val_if_fail (info != NULL, FALSE);

  if (!propono_plugin_info_is_active (info))
    return TRUE;

  g_signal_emit (engine, signals[DEACTIVATE_PLUGIN], 0, info);
  if (!propono_plugin_info_is_active (info))
    save_active_plugin_list (engine);

  return !propono_plugin_info_is_active (info);
}

void
propono_plugins_engine_activate_plugins (ProponoPluginsEngine *engine,
					 ProponoWindow        *window)
{
  GSList *pl, *active_plugins = NULL;

  propono_debug (DEBUG_PLUGINS);

  g_return_if_fail (PROPONO_IS_PLUGINS_ENGINE (engine));
  g_return_if_fail (PROPONO_IS_WINDOW (window));

  /* the first time, we get the 'active' plugins from mateconf */
  if (engine->priv->activate_from_prefs)
    {
      g_object_get (propono_prefs_get_default (),
		    "active-plugins", &active_plugins,
		    NULL);
    }

  for (pl = engine->priv->plugin_list; pl; pl = pl->next)
    {
      ProponoPluginInfo *info = (ProponoPluginInfo*)pl->data;
		
      if (engine->priv->activate_from_prefs && 
	  g_slist_find_custom (active_plugins,
			       propono_plugin_info_get_module_name (info),
			       (GCompareFunc)strcmp) == NULL)
	continue;
		
      /* If plugin is not active, don't try to activate/load it */
      if (!engine->priv->activate_from_prefs && 
	  !propono_plugin_info_is_active (info))
	continue;

      if (load_plugin (engine, info))
	propono_plugin_activate (info->plugin, window);
    }
	
  if (engine->priv->activate_from_prefs)
    {
      g_slist_foreach (active_plugins, (GFunc) g_free, NULL);
      g_slist_free (active_plugins);
      engine->priv->activate_from_prefs = FALSE;
    }
	
  propono_debug_message (DEBUG_PLUGINS, "End");

  /* also call update_ui after activation */
  propono_plugins_engine_update_plugins_ui (engine, window);
}

void
propono_plugins_engine_deactivate_plugins (ProponoPluginsEngine *engine,
                              					   ProponoWindow        *window)
{
	GSList *pl;
	
	propono_debug (DEBUG_PLUGINS);

	g_return_if_fail (PROPONO_IS_PLUGINS_ENGINE (engine));
	g_return_if_fail (PROPONO_IS_WINDOW (window));
	
	for (pl = engine->priv->plugin_list; pl; pl = pl->next)
	{
		ProponoPluginInfo *info = (ProponoPluginInfo*)pl->data;
		
		/* check if the plugin is actually active */
		if (!propono_plugin_info_is_active (info))
			continue;
		
		/* call deactivate for the plugin for this window */
		propono_plugin_deactivate (info->plugin, window);
	}
	
	propono_debug_message (DEBUG_PLUGINS, "End");
}

void
propono_plugins_engine_update_plugins_ui (ProponoPluginsEngine *engine,
                                          ProponoWindow        *window)
{
	GSList *pl;

	propono_debug (DEBUG_PLUGINS);

	g_return_if_fail (PROPONO_IS_PLUGINS_ENGINE (engine));
	g_return_if_fail (PROPONO_IS_WINDOW (window));

	/* call update_ui for all active plugins */
	for (pl = engine->priv->plugin_list; pl; pl = pl->next)
	{
		ProponoPluginInfo *info = (ProponoPluginInfo*)pl->data;

		if (!propono_plugin_info_is_active (info))
			continue;
			
	       	propono_debug_message (DEBUG_PLUGINS, "Updating UI of %s", info->name);
		propono_plugin_update_ui (info->plugin, window);
	}
}

void 	 
propono_plugins_engine_configure_plugin (ProponoPluginsEngine *engine,
                          				       ProponoPluginInfo    *info,
                          				       GtkWindow          *parent)
{
	GtkWidget *conf_dlg;
	
	GtkWindowGroup *wg;
	
	propono_debug (DEBUG_PLUGINS);

	g_return_if_fail (info != NULL);

	conf_dlg = propono_plugin_create_configure_dialog (info->plugin);
	g_return_if_fail (conf_dlg != NULL);
	gtk_window_set_transient_for (GTK_WINDOW (conf_dlg),
				      parent);

	wg = parent->group;		      
	if (wg == NULL)
	{
		wg = gtk_window_group_new ();
		gtk_window_group_add_window (wg, parent);
	}
			
	gtk_window_group_add_window (wg,
				     GTK_WINDOW (conf_dlg));
		
	gtk_window_set_modal (GTK_WINDOW (conf_dlg), TRUE);		     
	gtk_widget_show (conf_dlg);
}

void 
propono_plugins_engine_active_plugins_changed (ProponoPluginsEngine *engine)
{
	gboolean to_activate;
	GSList *pl, *active_plugins;

	propono_debug (DEBUG_PLUGINS);

	g_object_get (propono_prefs_get_default (),
		      "active-plugins", &active_plugins,
		      NULL);

	for (pl = engine->priv->plugin_list; pl; pl = pl->next)
	{
		ProponoPluginInfo *info = (ProponoPluginInfo*)pl->data;

		if (!propono_plugin_info_is_available (info))
			continue;

		to_activate = (g_slist_find_custom (active_plugins,
						    propono_plugin_info_get_module_name (info),
						    (GCompareFunc)strcmp) != NULL);

		if (!propono_plugin_info_is_active (info) && to_activate)
			g_signal_emit (engine, signals[ACTIVATE_PLUGIN], 0, info);
		else if (propono_plugin_info_is_active (info) && !to_activate)
			g_signal_emit (engine, signals[DEACTIVATE_PLUGIN], 0, info);
	}

	g_slist_foreach (active_plugins, (GFunc) g_free, NULL);
	g_slist_free (active_plugins);
}

void
propono_plugins_engine_rescan_plugins (ProponoPluginsEngine *engine)
{
	propono_debug (DEBUG_PLUGINS);
	
	load_all_plugins (engine);
}

GHashTable *
propono_plugin_engine_get_plugins_by_protocol (ProponoPluginsEngine *engine)
{
  return engine->priv->protocols;
}

ProponoPlugin *
propono_plugins_engine_get_plugin_by_protocol (ProponoPluginsEngine *engine,
					      const gchar          *protocol)
{
  g_return_val_if_fail (PROPONO_IS_PLUGINS_ENGINE (engine), NULL);

  return g_hash_table_lookup (engine->priv->protocols, (gconstpointer) protocol);
}

/* vim: set ts=8: */
