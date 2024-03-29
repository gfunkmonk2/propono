/*
 * propono-plugin-info.c
 * This file is part of propono
 *
 * Based on pluma plugin system
 * Copyright (C) 2002-2005 - Paolo Maggi 
 * Copyright (C) 2007 - Paolo Maggi, Steve Frécinaux
 * 
 * propono-plugin-info.c is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation, either version 2 of the License, or
 * (at your option) any later version.
 * 
 * propono-plugin-info.c is distributed in the hope that it will be useful, but
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
#include <glib.h>

#include "propono-debug.h"
#include "propono-plugin.h"
#include "propono-plugin-info.h"
#include "propono-plugin-info-priv.h"

void
_propono_plugin_info_ref (ProponoPluginInfo *info)
{
  g_atomic_int_inc (&info->refcount);
}

static ProponoPluginInfo *
propono_plugin_info_copy (ProponoPluginInfo *info)
{
  _propono_plugin_info_ref (info);
  return info;
}

void
_propono_plugin_info_unref (ProponoPluginInfo *info)
{
  if (!g_atomic_int_dec_and_test (&info->refcount))
    return;

  if (info->plugin != NULL)
    {
      propono_debug_message (DEBUG_PLUGINS, "Unref plugin %s", info->name);
      g_object_unref (info->plugin);
    }

  g_free (info->file);
  g_free (info->module_name);
  g_strfreev (info->dependencies);
  g_free (info->name);
  g_free (info->desc);
  g_free (info->icon_name);
  g_free (info->website);
  g_free (info->copyright);
  g_free (info->loader);
  g_free (info->version);
  g_strfreev (info->authors);

  g_free (info);
}

/**
 * propono_plugin_info_get_type:
 *
 * Retrieves the #GType object which is associated with the #ProponoPluginInfo
 * class.
 *
 * Return value: the GType associated with #ProponoPluginInfo.
 **/
GType
propono_plugin_info_get_type (void)
{
  static GType the_type = 0;

  if (G_UNLIKELY (!the_type))
    the_type = g_boxed_type_register_static ("ProponoPluginInfo",
					     (GBoxedCopyFunc) propono_plugin_info_copy,
					     (GBoxedFreeFunc) _propono_plugin_info_unref);

  return the_type;
} 

/**
 * propono_plugin_info_new:
 * @filename: the filename where to read the plugin information
 *
 * Creates a new #ProponoPluginInfo from a file on the disk.
 *
 * Return value: a newly created #ProponoPluginInfo.
 */
ProponoPluginInfo *
_propono_plugin_info_new (const gchar *file)
{
  ProponoPluginInfo *info;
  GKeyFile *plugin_file = NULL;
  gchar *str;

  g_return_val_if_fail (file != NULL, NULL);

  propono_debug_message (DEBUG_PLUGINS, "Loading plugin: %s", file);

  info = g_new0 (ProponoPluginInfo, 1);
  info->refcount = 1;
  info->file = g_strdup (file);

  plugin_file = g_key_file_new ();
  if (!g_key_file_load_from_file (plugin_file, file, G_KEY_FILE_NONE, NULL))
    {
      g_warning ("Bad plugin file: %s", file);
      goto error;
    }

  if (!g_key_file_has_key (plugin_file,
			   "Propono Plugin",
			   "IAge",
			   NULL))
    {
      propono_debug_message (DEBUG_PLUGINS,
			     "IAge key does not exist in file: %s", file);
      goto error;
    }
	
  /* Check IAge=1 */
  if (g_key_file_get_integer (plugin_file,
			      "Propono Plugin",
			      "IAge",
			      NULL) != 1)
    {
      propono_debug_message (DEBUG_PLUGINS,
			     "Wrong IAge in file: %s", file);
      goto error;
    }
				    
  /* Get module name */
  str = g_key_file_get_string (plugin_file,
			       "Propono Plugin",
			       "Module",
			       NULL);

  if ((str != NULL) && (*str != '\0'))
    info->module_name = str;
  else
    {
      g_warning ("Could not find 'Module' in %s", file);
      goto error;
    }

  /* Get the dependency list */
  info->dependencies = g_key_file_get_string_list (plugin_file,
						   "Propono Plugin",
						   "Depends",
						    NULL,
						    NULL);
  if (info->dependencies == NULL)
    {
      propono_debug_message (DEBUG_PLUGINS, "Could not find 'Depends' in %s", file);
      info->dependencies = g_new0 (gchar *, 1);
    }

  /* Get the loader for this plugin */
  str = g_key_file_get_string (plugin_file,
			       "Propono Plugin",
			       "Loader",
			       NULL);
  if ((str != NULL) && (*str != '\0'))
    info->loader = str;
  else
    /* default to the C loader */
    info->loader = g_strdup("c");

  /* Get Name */
  str = g_key_file_get_locale_string (plugin_file,
				      "Propono Plugin",
				      "Name",
				      NULL,
				      NULL);
  if (str)
    info->name = str;
  else
    {
      g_warning ("Could not find 'Name' in %s", file);
      goto error;
    }

  /* Get Description */
  str = g_key_file_get_locale_string (plugin_file,
				      "Propono Plugin",
				      "Description",
				      NULL,
				      NULL);
  if (str)
    info->desc = str;
  else
    propono_debug_message (DEBUG_PLUGINS, "Could not find 'Description' in %s", file);

  /* Get Icon */
  str = g_key_file_get_locale_string (plugin_file,
				      "Propono Plugin",
				      "Icon",
				      NULL,
				      NULL);
  if (str)
    info->icon_name = str;
  else
    propono_debug_message (DEBUG_PLUGINS, "Could not find 'Icon' in %s, using 'propono'", file);
	
  /* Get Authors */
  info->authors = g_key_file_get_string_list (plugin_file,
					      "Propono Plugin",
					      "Authors",
					      NULL,
					      NULL);
  if (info->authors == NULL)
    propono_debug_message (DEBUG_PLUGINS, "Could not find 'Authors' in %s", file);

  /* Get Copyright */
  str = g_key_file_get_string (plugin_file,
			       "Propono Plugin",
			       "Copyright",
			       NULL);
  if (str)
    info->copyright = str;
  else
    propono_debug_message (DEBUG_PLUGINS, "Could not find 'Copyright' in %s", file);

  /* Get Website */
  str = g_key_file_get_string (plugin_file,
			       "Propono Plugin",
			       "Website",
			       NULL);
  if (str)
    info->website = str;
  else
    propono_debug_message (DEBUG_PLUGINS, "Could not find 'Website' in %s", file);
	
  /* Get Version */
  str = g_key_file_get_string (plugin_file,
			       "Propono Plugin",
			       "Version",
			       NULL);
  if (str)
    info->version = str;
  else
    propono_debug_message (DEBUG_PLUGINS, "Could not find 'Version' in %s", file);

  /* Check if is engine or normal plugin */
  info->engine = g_key_file_get_boolean (plugin_file,
					"Propono Plugin",
					"Engine",
					NULL);
  if (str)
    info->version = str;
  else
    propono_debug_message (DEBUG_PLUGINS, "Could not find 'Version' in %s", file);
	
  g_key_file_free (plugin_file);
	
  /* If we know nothing about the availability of the plugin,
     set it as available */
  info->available = TRUE;
	
  return info;

error:
  g_free (info->file);
  g_free (info->module_name);
  g_free (info->name);
  g_free (info->loader);
  g_free (info);
  g_key_file_free (plugin_file);

  return NULL;
}

gboolean
propono_plugin_info_is_active (ProponoPluginInfo *info)
{
  g_return_val_if_fail (info != NULL, FALSE);

  return info->available && info->plugin != NULL;
}

gboolean
propono_plugin_info_is_available (ProponoPluginInfo *info)
{
  g_return_val_if_fail (info != NULL, FALSE);

  return info->available != FALSE;
}

gboolean
propono_plugin_info_is_configurable (ProponoPluginInfo *info)
{
  propono_debug_message (DEBUG_PLUGINS, "Is '%s' configurable?", info->name);

  g_return_val_if_fail (info != NULL, FALSE);

  if (info->plugin == NULL || !info->available)
    return FALSE;

  return propono_plugin_is_configurable (info->plugin);
}

const gchar *
propono_plugin_info_get_module_name (ProponoPluginInfo *info)
{
  g_return_val_if_fail (info != NULL, NULL);

  return info->module_name;
}

const gchar *
propono_plugin_info_get_name (ProponoPluginInfo *info)
{
  g_return_val_if_fail (info != NULL, NULL);

  return info->name;
}

const gchar *
propono_plugin_info_get_description (ProponoPluginInfo *info)
{
  g_return_val_if_fail (info != NULL, NULL);

  return info->desc;
}

const gchar *
propono_plugin_info_get_icon_name (ProponoPluginInfo *info)
{
  g_return_val_if_fail (info != NULL, NULL);

  /* use the propono-plugin icon as a default if the plugin does not
     have its own */
  if (info->icon_name != NULL && 
      gtk_icon_theme_has_icon (gtk_icon_theme_get_default (),
			       info->icon_name))
    return info->icon_name;
  else
    return "propono";
}

const gchar **
propono_plugin_info_get_authors (ProponoPluginInfo *info)
{
  g_return_val_if_fail (info != NULL, (const gchar **)NULL);

  return (const gchar **) info->authors;
}

const gchar *
propono_plugin_info_get_website (ProponoPluginInfo *info)
{
  g_return_val_if_fail (info != NULL, NULL);

  return info->website;
}

const gchar *
propono_plugin_info_get_copyright (ProponoPluginInfo *info)
{
  g_return_val_if_fail (info != NULL, NULL);

  return info->copyright;
}

const gchar *
propono_plugin_info_get_version (ProponoPluginInfo *info)
{
  g_return_val_if_fail (info != NULL, NULL);

  return info->version;
}

gboolean
propono_plugin_info_is_engine (ProponoPluginInfo *info)
{
  g_return_val_if_fail (info != NULL, FALSE);

  return info->engine;
}

/* vim: set ts=8: */
