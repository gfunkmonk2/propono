/*
 * propono-plugin.c
 * This file is part of propono

 * Based on pluma plugin system
 * Copyright (C) 2002-2005 Paolo Maggi
 * Copyright (C) 2009 Jorge Pereira <jorge@jorgepereira.com.br>
 * Copyright (C) 2009 Jonh Wendell <wendell@bani.com.br>
 * 
 * propono-plugin.c is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation, either version 2 of the License, or
 * (at your option) any later version.
 * 
 * propono-plugin.c is distributed in the hope that it will be useful, but
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

#include "propono-plugin.h"
#include "propono-plugin-info.h"
#include "propono-dirs.h"

/* properties */
enum {
	PROP_0,
	PROP_INSTALL_DIR,
	PROP_DATA_DIR_NAME,
	PROP_DATA_DIR
};

typedef struct _ProponoPluginPrivate ProponoPluginPrivate;

struct _ProponoPluginPrivate
{
  gchar *install_dir;
  gchar *data_dir_name;
};

#define PROPONO_PLUGIN_GET_PRIVATE(object)(G_TYPE_INSTANCE_GET_PRIVATE ((object), PROPONO_TYPE_PLUGIN, ProponoPluginPrivate))

G_DEFINE_TYPE(ProponoPlugin, propono_plugin, G_TYPE_OBJECT)

static void
dummy (ProponoPlugin *plugin, ProponoWindow *window)
{
  /* Empty */
}

static GtkWidget *
create_configure_dialog	(ProponoPlugin *plugin)
{
  return NULL;
}

static GSList *
default_context_groups (ProponoPlugin *plugin)
{
  return NULL;
}

static const gchar *
default_get_protocol (ProponoPlugin *plugin)
{
  return NULL;
}

static gchar **
default_get_public_description (ProponoPlugin *plugin)
{
  return NULL;
}

static ProponoConnection *
default_new_connection (ProponoPlugin *plugin)
{
  return NULL;
}

static GtkWidget *
default_new_tab (ProponoPlugin *plugin,
		 ProponoConnection *conn,
		 ProponoWindow     *window)
{
  return NULL;
}

static ProponoConnection *
default_new_connection_from_file (ProponoPlugin *plugin,
				  const gchar   *data,
				  gboolean       use_bookmarks,
				  gchar        **error_msg)
{
  return NULL;
}

static gint
default_get_default_port (ProponoPlugin *plugin)
{
  return -1;
}

static gboolean
is_configurable (ProponoPlugin *plugin)
{
	return (PROPONO_PLUGIN_GET_CLASS (plugin)->create_configure_dialog !=
		create_configure_dialog);
}

static GtkWidget *
default_get_connect_widget (ProponoPlugin     *plugin,
			    ProponoConnection *initial_settings)
{
  return NULL;
}

static GtkFileFilter *
default_get_file_filter (ProponoPlugin *plugin)
{
  return NULL;
}

static void
default_parse_mdns_dialog (ProponoPlugin *plugin,
			   GtkWidget *connect_widget,
			   GtkWidget *dialog)
{
}

static void
propono_plugin_get_property (GObject    *object,
			   guint       prop_id,
			   GValue     *value,
			   GParamSpec *pspec)
{
	switch (prop_id)
	{
		case PROP_INSTALL_DIR:
			g_value_take_string (value, propono_plugin_get_install_dir (PROPONO_PLUGIN (object)));
			break;
		case PROP_DATA_DIR:
			g_value_take_string (value, propono_plugin_get_data_dir (PROPONO_PLUGIN (object)));
			break;
		default:
			g_return_if_reached ();
	}
}

static void
propono_plugin_set_property (GObject      *object,
			   guint         prop_id,
			   const GValue *value,
			   GParamSpec   *pspec)
{
	ProponoPluginPrivate *priv = PROPONO_PLUGIN_GET_PRIVATE (object);

	switch (prop_id)
	{
		case PROP_INSTALL_DIR:
			priv->install_dir = g_value_dup_string (value);
			break;
		case PROP_DATA_DIR_NAME:
			priv->data_dir_name = g_value_dup_string (value);
			break;
		default:
			g_return_if_reached ();
	}
}

static void
propono_plugin_finalize (GObject *object)
{
	ProponoPluginPrivate *priv = PROPONO_PLUGIN_GET_PRIVATE (object);

	g_free (priv->install_dir);
	g_free (priv->data_dir_name);

	G_OBJECT_CLASS (propono_plugin_parent_class)->finalize (object);
}

static void 
propono_plugin_class_init (ProponoPluginClass *klass)
{
    	GObjectClass *object_class = G_OBJECT_CLASS (klass);

	klass->activate = dummy;
	klass->deactivate = dummy;
	klass->update_ui = dummy;
	klass->get_context_groups = default_context_groups;
	klass->get_protocol = default_get_protocol;
	klass->get_public_description = default_get_public_description;
	klass->get_default_port = default_get_default_port;
	klass->new_connection = default_new_connection;
	klass->new_connection_from_file = default_new_connection_from_file;
	klass->get_mdns_service = default_get_protocol;
	klass->new_tab = default_new_tab;
	klass->get_connect_widget = default_get_connect_widget;
	klass->get_file_filter = default_get_file_filter;
	klass->parse_mdns_dialog = default_parse_mdns_dialog;
	
	klass->create_configure_dialog = create_configure_dialog;
	klass->is_configurable = is_configurable;

	object_class->get_property = propono_plugin_get_property;
	object_class->set_property = propono_plugin_set_property;
	object_class->finalize = propono_plugin_finalize;

	g_object_class_install_property (object_class,
					 PROP_INSTALL_DIR,
					 g_param_spec_string ("install-dir",
							      "Install Directory",
							      "The directory where the plugin is installed",
							      NULL,
							      G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY));

	/* the basename of the data dir is set at construction time by the plugin loader
	 * while the full path is constructed on the fly to take into account relocability
	 * that's why we have a writeonly prop and a readonly prop */
	g_object_class_install_property (object_class,
					 PROP_DATA_DIR_NAME,
					 g_param_spec_string ("data-dir-name",
							      "Basename of the data directory",
							      "The basename of the directory where the plugin should look for its data files",
							      NULL,
							      G_PARAM_WRITABLE | G_PARAM_CONSTRUCT_ONLY));
	g_object_class_install_property (object_class,
					 PROP_DATA_DIR,
					 g_param_spec_string ("data-dir",
							      "Data Directory",
							      "The full path of the directory where the plugin should look for its data files",
							      NULL,
							      G_PARAM_READABLE));

	g_type_class_add_private (klass, sizeof (ProponoPluginPrivate));
}

static void
propono_plugin_init (ProponoPlugin *plugin)
{
	/* Empty */
}

/**
 * propono_plugin_get_install_dir:
 * @plugin: a #ProponoPlugin
 *
 * Get the path of the directory where the plugin is installed.
 *
 * Return value: a newly allocated string with the path of the
 * directory where the plugin is installed
 */
gchar *
propono_plugin_get_install_dir (ProponoPlugin *plugin)
{
	g_return_val_if_fail (PROPONO_IS_PLUGIN (plugin), NULL);

	return g_strdup (PROPONO_PLUGIN_GET_PRIVATE (plugin)->install_dir);
}

/**
 * propono_plugin_get_data_dir:
 * @plugin: a #ProponoPlugin
 *
 * Get the path of the directory where the plugin should look for
 * its data files.
 *
 * Return value: a newly allocated string with the path of the
 * directory where the plugin should look for its data files
 */
gchar *
propono_plugin_get_data_dir (ProponoPlugin *plugin)
{
	ProponoPluginPrivate *priv;
	gchar *propono_lib_dir;
	gchar *data_dir;

	g_return_val_if_fail (PROPONO_IS_PLUGIN (plugin), NULL);

	priv = PROPONO_PLUGIN_GET_PRIVATE (plugin);

	/* If it's a "user" plugin the data dir is
	 * install_dir/data_dir_name if instead it's a
	 * "system" plugin the data dir is under propono_data_dir,
	 * so it's under $prefix/share/propono-2/plugins/data_dir_name
	 * where data_dir_name usually it's the name of the plugin
	 */
	propono_lib_dir = propono_dirs_get_propono_lib_dir ();

	/* CHECK: is checking the prefix enough or should we be more
	 * careful about normalizing paths etc? */
	if (g_str_has_prefix (priv->install_dir, propono_lib_dir))
	{
		gchar *propono_data_dir;

		propono_data_dir = propono_dirs_get_propono_data_dir ();

		data_dir = g_build_filename (propono_data_dir,
					     "plugins",
					     priv->data_dir_name,
					     NULL);

		g_free (propono_data_dir);
	}
	else
	{
		data_dir = g_build_filename (priv->install_dir,
					     priv->data_dir_name,
					     NULL);
	}

	g_free (propono_lib_dir);

	return data_dir;
}

/**
 * propono_plugin_activate:
 * @plugin: a #ProponoPlugin
 * @window: a #ProponoWindow
 * 
 * Activates the plugin.
 */
void
propono_plugin_activate (ProponoPlugin *plugin,
			 ProponoWindow *window)
{
  g_return_if_fail (PROPONO_IS_PLUGIN (plugin));
	
  PROPONO_PLUGIN_GET_CLASS (plugin)->activate (plugin, window);
}

/**
 * propono_plugin_deactivate:
 * @plugin: a #ProponoPlugin
 * @window: a #ProponoWindow
 * 
 * Deactivates the plugin.
 */
void
propono_plugin_deactivate (ProponoPlugin *plugin,
			   ProponoWindow *window)
{
  g_return_if_fail (PROPONO_IS_PLUGIN (plugin));

  PROPONO_PLUGIN_GET_CLASS (plugin)->deactivate (plugin, window);
}

/**
 * propono_plugin_update_ui:
 * @plugin: a #ProponoPlugin
 * @window: a #ProponoWindow
 *
 * Triggers an update of the user interface to take into account state changes
 * caused by the plugin.
 */		 
void
propono_plugin_update_ui	(ProponoPlugin *plugin,
			 ProponoWindow *window)
{
	g_return_if_fail (PROPONO_IS_PLUGIN (plugin));
	g_return_if_fail (PROPONO_IS_WINDOW (window));

	PROPONO_PLUGIN_GET_CLASS (plugin)->update_ui (plugin, window);
}

/**
 * propono_plugin_is_configurable:
 * @plugin: a #ProponoPlugin
 *
 * Whether the plugin is configurable.
 *
 * Returns: TRUE if the plugin is configurable:
 */
gboolean
propono_plugin_is_configurable (ProponoPlugin *plugin)
{
	g_return_val_if_fail (PROPONO_IS_PLUGIN (plugin), FALSE);

	return PROPONO_PLUGIN_GET_CLASS (plugin)->is_configurable (plugin);
}

/**
 * propono_plugin_create_configure_dialog:
 * @plugin: a #ProponoPlugin
 *
 * Creates the configure dialog widget for the plugin.
 *
 * Returns: the configure dialog widget for the plugin.
 */
GtkWidget *
propono_plugin_create_configure_dialog (ProponoPlugin *plugin)
{
  g_return_val_if_fail (PROPONO_IS_PLUGIN (plugin), NULL);
	
  return PROPONO_PLUGIN_GET_CLASS (plugin)->create_configure_dialog (plugin);
}

/**
 * propono_plugin_get_context_groups
 * @plugin: a #ProponoPlugin
 *
 *
 * Returns: a list of context groups to be used on command line, if available
 */
GSList *
propono_plugin_get_context_groups (ProponoPlugin *plugin)
{
  g_return_val_if_fail (PROPONO_IS_PLUGIN (plugin), NULL);
	
  return PROPONO_PLUGIN_GET_CLASS (plugin)->get_context_groups (plugin);
}

/**
 * propono_plugin_get_protocol
 * @plugin: a #ProponoPlugin
 *
 *
 * Returns: a protocol, like "vnc" or "rdp"
 */
const gchar *
propono_plugin_get_protocol (ProponoPlugin *plugin)
{
  g_return_val_if_fail (PROPONO_IS_PLUGIN (plugin), NULL);
	
  return PROPONO_PLUGIN_GET_CLASS (plugin)->get_protocol (plugin);
}

/**
 * propono_plugin_get_public_description
 * @plugin: a #ProponoPlugin
 *
 *
 * Returns: an array of strings:
 * [0] -> the protocol name, like VNC, or SSH
 * [1] -> the protocol description, like "A secure shell access"
 */
gchar **
propono_plugin_get_public_description (ProponoPlugin *plugin)
{
  g_return_val_if_fail (PROPONO_IS_PLUGIN (plugin), NULL);
	
  return PROPONO_PLUGIN_GET_CLASS (plugin)->get_public_description (plugin);
}

/**
 * propono_plugin_new_connection
 * @plugin: a #ProponoPlugin
 *
 *
 * Returns: a subclass of the Connection class
 */
ProponoConnection *
propono_plugin_new_connection (ProponoPlugin *plugin)
{
  g_return_val_if_fail (PROPONO_IS_PLUGIN (plugin), NULL);
	
  return PROPONO_PLUGIN_GET_CLASS (plugin)->new_connection (plugin);
}

/**
 * propono_plugin_new_connection_from_file
 * @plugin: a #ProponoPlugin
 *
 *
 * Returns: a subclass of the Connection class
 */
ProponoConnection *
propono_plugin_new_connection_from_file (ProponoPlugin *plugin,
					 const gchar   *data,
					 gboolean       use_bookmarks,
					 gchar        **error_msg)
{
  g_return_val_if_fail (PROPONO_IS_PLUGIN (plugin), NULL);
	
  return PROPONO_PLUGIN_GET_CLASS (plugin)->new_connection_from_file (plugin, data, use_bookmarks, error_msg);
}

/**
 * propono_plugin_get_default_port
 * @plugin: a #ProponoPlugin
 *
 *
 * Returns: the default port (ex: 5900 for vnc)
 */
gint
propono_plugin_get_default_port (ProponoPlugin *plugin)
{
  g_return_val_if_fail (PROPONO_IS_PLUGIN (plugin), -1);
	
  return PROPONO_PLUGIN_GET_CLASS (plugin)->get_default_port (plugin);
}

/**
 * propono_plugin_get_mdns_service
 * @plugin: a #ProponoPlugin
 *
 *
 * Returns: a mDNS service for this plugin, like rfb.tcp
 */
const gchar *
propono_plugin_get_mdns_service (ProponoPlugin *plugin)
{
  g_return_val_if_fail (PROPONO_IS_PLUGIN (plugin), NULL);
	
  return PROPONO_PLUGIN_GET_CLASS (plugin)->get_mdns_service (plugin);
}

/**
 * propono_plugin_new_tab
 * @plugin: a #ProponoTab
 *
 *
 * Returns: a subclass of the Tab class
 */
GtkWidget *
propono_plugin_new_tab (ProponoPlugin     *plugin,
			ProponoConnection *conn,
			ProponoWindow     *window)
{
  g_return_val_if_fail (PROPONO_IS_PLUGIN (plugin), NULL);

  return PROPONO_PLUGIN_GET_CLASS (plugin)->new_tab (plugin, conn, window);
}

/**
 * propono_plugin_get_connect_widget
 * @plugin: a #ProponoPlugin
 * @initial_settings: a #ProponoConnection object, or NULL
 *
 *
 * Returns: a widget to be put inside connect dialog
 */
GtkWidget *
propono_plugin_get_connect_widget (ProponoPlugin     *plugin,
				   ProponoConnection *initial_settings)
{
  g_return_val_if_fail (PROPONO_IS_PLUGIN (plugin), NULL);

  return PROPONO_PLUGIN_GET_CLASS (plugin)->get_connect_widget (plugin, initial_settings);
}

/**
 * propono_plugin_get_file_filter
 * @plugin: a #ProponoPlugin
 *
 *
 * Returns: a filter to be used at Open File dialog
 */
GtkFileFilter *
propono_plugin_get_file_filter (ProponoPlugin *plugin)
{
  g_return_val_if_fail (PROPONO_IS_PLUGIN (plugin), NULL);

  return PROPONO_PLUGIN_GET_CLASS (plugin)->get_file_filter (plugin);
}

GdkPixbuf *
propono_plugin_get_icon (ProponoPlugin *plugin, gint size)
{
  ProponoPluginInfo *info;

  g_return_val_if_fail (PROPONO_IS_PLUGIN (plugin), NULL);

  info = g_object_get_data (G_OBJECT (plugin), "info");
  g_return_val_if_fail (info != NULL, NULL);

  return gtk_icon_theme_load_icon (gtk_icon_theme_get_default (),
				   propono_plugin_info_get_icon_name (info),
				   size,
				   0,
				   NULL);
}

const gchar *
propono_plugin_get_icon_name (ProponoPlugin *plugin)
{
  ProponoPluginInfo *info;

  g_return_val_if_fail (PROPONO_IS_PLUGIN (plugin), NULL);

  info = g_object_get_data (G_OBJECT (plugin), "info");
  g_return_val_if_fail (info != NULL, NULL);

  return propono_plugin_info_get_icon_name (info);
}

void
propono_plugin_parse_mdns_dialog (ProponoPlugin *plugin,
				  GtkWidget *connect_widget,
				  GtkWidget *dialog)
{
  g_return_if_fail (PROPONO_IS_PLUGIN (plugin));

  PROPONO_PLUGIN_GET_CLASS (plugin)->parse_mdns_dialog (plugin, connect_widget, dialog);
}

/* vim: set ts=8: */
