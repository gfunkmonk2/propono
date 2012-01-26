/*
 * propono-plugin.h
 * This file is part of propono
 *
 * Based on pluma plugin system
 * Copyright (C) 2002-2005 Paolo Maggi
 * Copyright (C) 2009 Jonh Wendell <wendell@bani.com.br>s
 * 
 * propono-plugin.h is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation, either version 2 of the License, or
 * (at your option) any later version.
 * 
 * propono-plugin.h is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License along
 * with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef __PROPONO_PLUGIN_H__
#define __PROPONO_PLUGIN_H__

#include <glib-object.h>

#include "propono-window.h"

/* TODO: add a .h file that includes all the .h files normally needed to
 * develop a plugin */ 

G_BEGIN_DECLS

/*
 * Type checking and casting macros
 */
#define PROPONO_TYPE_PLUGIN              (propono_plugin_get_type())
#define PROPONO_PLUGIN(obj)              (G_TYPE_CHECK_INSTANCE_CAST((obj), PROPONO_TYPE_PLUGIN, ProponoPlugin))
#define PROPONO_PLUGIN_CLASS(klass)      (G_TYPE_CHECK_CLASS_CAST((klass), PROPONO_TYPE_PLUGIN, ProponoPluginClass))
#define PROPONO_IS_PLUGIN(obj)           (G_TYPE_CHECK_INSTANCE_TYPE((obj), PROPONO_TYPE_PLUGIN))
#define PROPONO_IS_PLUGIN_CLASS(klass)   (G_TYPE_CHECK_CLASS_TYPE ((klass), PROPONO_TYPE_PLUGIN))
#define PROPONO_PLUGIN_GET_CLASS(obj)    (G_TYPE_INSTANCE_GET_CLASS((obj), PROPONO_TYPE_PLUGIN, ProponoPluginClass))

/*
 * Main object structure
 */
typedef struct _ProponoPlugin ProponoPlugin;

struct _ProponoPlugin 
{
  GObject parent;
};

/*
 * Class definition
 */
typedef struct _ProponoPluginClass ProponoPluginClass;

struct _ProponoPluginClass 
{
  GObjectClass parent_class;

  /* Virtual public methods */
  void 		(*activate)			(ProponoPlugin *plugin,
						 ProponoWindow *window);
  void 		(*deactivate)			(ProponoPlugin *plugin,
						 ProponoWindow *window);

  void 		(*update_ui)			(ProponoPlugin *plugin,
						 ProponoWindow *window);

  GtkWidget 	*(*create_configure_dialog)	(ProponoPlugin *plugin);


  /* Virtual methods specific to 'engine' plugins */
  GSList	*(*get_context_groups)		(ProponoPlugin *plugin);
  const gchar	*(*get_protocol)		(ProponoPlugin *plugin);
  gchar		**(*get_public_description)	(ProponoPlugin *plugin);
  gint		(*get_default_port)		(ProponoPlugin *plugin);
  ProponoConnection *(*new_connection)		(ProponoPlugin *plugin);
  ProponoConnection *(*new_connection_from_file)(ProponoPlugin *plugin,
						 const gchar   *data,
						 gboolean       use_bookmarks,
						 gchar        **error_msg);
  const gchar	*(*get_mdns_service)		(ProponoPlugin *plugin);
  GtkWidget 	*(*new_tab)			(ProponoPlugin     *plugin,
						 ProponoConnection *conn,
						 ProponoWindow     *window);

  GtkWidget 	*(*get_connect_widget)		(ProponoPlugin     *plugin,
						 ProponoConnection *initial_settings);
  void		(*parse_mdns_dialog)		(ProponoPlugin *plugin,
						 GtkWidget *connect_widget,
						 GtkWidget *dialog);


  GtkFileFilter	*(*get_file_filter)		(ProponoPlugin     *plugin);

  /* Plugins should not override this, it's handled automatically by
     the ProponoPluginClass */
  gboolean 	(*is_configurable)		(ProponoPlugin *plugin);

  /* Padding for future expansion */
  void		(*_propono_reserved1)		(void);
  void		(*_propono_reserved2)		(void);
  void		(*_propono_reserved3)		(void);
  void		(*_propono_reserved4)		(void);
};

/*
 * Public methods
 */
GType 		 propono_plugin_get_type		(void) G_GNUC_CONST;

gchar 		*propono_plugin_get_install_dir		(ProponoPlugin *plugin);
gchar 		*propono_plugin_get_data_dir		(ProponoPlugin *plugin);

void 		 propono_plugin_activate		(ProponoPlugin *plugin,
							 ProponoWindow *window);
void 		 propono_plugin_deactivate		(ProponoPlugin *plugin,
							 ProponoWindow *window);
				 
void 		 propono_plugin_update_ui		(ProponoPlugin *plugin,
							 ProponoWindow *window);

gboolean	 propono_plugin_is_configurable		(ProponoPlugin *plugin);
GtkWidget	*propono_plugin_create_configure_dialog	(ProponoPlugin *plugin);
void		 propono_plugin_parse_mdns_dialog	(ProponoPlugin *plugin,
							 GtkWidget *connect_widget,
							 GtkWidget *dialog);

GSList		*propono_plugin_get_context_groups	(ProponoPlugin *plugin);
const gchar	*propono_plugin_get_protocol		(ProponoPlugin *plugin);
gchar		**propono_plugin_get_public_description	(ProponoPlugin *plugin);
gint		 propono_plugin_get_default_port	(ProponoPlugin *plugin);
ProponoConnection *propono_plugin_new_connection	(ProponoPlugin *plugin);
ProponoConnection *propono_plugin_new_connection_from_file (ProponoPlugin *plugin,
							    const gchar   *data,
							    gboolean       use_bookmarks,
							    gchar        **error_msg);
const gchar	*propono_plugin_get_mdns_service	(ProponoPlugin *plugin);

GtkWidget 	*propono_plugin_new_tab			(ProponoPlugin     *plugin,
							 ProponoConnection *conn,
							 ProponoWindow     *window);

GtkWidget 	*propono_plugin_get_connect_widget	(ProponoPlugin     *plugin,
							 ProponoConnection *initial_settings);

GtkFileFilter 	*propono_plugin_get_file_filter		(ProponoPlugin     *plugin);

GdkPixbuf	*propono_plugin_get_icon		(ProponoPlugin *plugin,
							 gint          size);
const gchar	*propono_plugin_get_icon_name		(ProponoPlugin *plugin);
/**
 * PROPONO_PLUGIN_REGISTER_TYPE_WITH_CODE(PluginName, plugin_name, CODE):
 *
 * Utility macro used to register plugins with additional code.
 */
#define PROPONO_PLUGIN_REGISTER_TYPE_WITH_CODE(PluginName, plugin_name, CODE) 	\
	G_DEFINE_DYNAMIC_TYPE_EXTENDED (PluginName,				\
					plugin_name,				\
					PROPONO_TYPE_PLUGIN,			\
					0,					\
					GTypeModule *module G_GNUC_UNUSED = type_module; /* back compat */	\
					CODE)					\
										\
/* This is not very nice, but G_DEFINE_DYNAMIC wants it and our old macro	\
 * did not support it */							\
static void									\
plugin_name##_class_finalize (PluginName##Class *klass)				\
{										\
}										\
										\
										\
G_MODULE_EXPORT GType								\
register_propono_plugin (GTypeModule *type_module)				\
{										\
	plugin_name##_register_type (type_module);				\
										\
	return plugin_name##_get_type();					\
}

/**
 * PROPONO_PLUGIN_REGISTER_TYPE(PluginName, plugin_name):
 * 
 * Utility macro used to register plugins.
 */
#define PROPONO_PLUGIN_REGISTER_TYPE(PluginName, plugin_name)			\
	PROPONO_PLUGIN_REGISTER_TYPE_WITH_CODE(PluginName, plugin_name, ;)

/**
 * PROPONO_PLUGIN_DEFINE_TYPE_WITH_CODE(ObjectName, object_name, PARENT_TYPE, CODE):
 *
 * Utility macro used to register gobject types in plugins with additional code.
 *
 * Deprecated: use G_DEFINE_DYNAMIC_TYPE_EXTENDED instead
 */
#define PROPONO_PLUGIN_DEFINE_TYPE_WITH_CODE(ObjectName, object_name, PARENT_TYPE, CODE) \
										\
static GType g_define_type_id = 0;						\
										\
GType										\
object_name##_get_type (void)							\
{										\
	return g_define_type_id;						\
}										\
										\
static void     object_name##_init              (ObjectName        *self);	\
static void     object_name##_class_init        (ObjectName##Class *klass);	\
static gpointer object_name##_parent_class = NULL;				\
static void     object_name##_class_intern_init (gpointer klass)		\
{										\
	object_name##_parent_class = g_type_class_peek_parent (klass);		\
	object_name##_class_init ((ObjectName##Class *) klass);			\
}										\
										\
GType										\
object_name##_register_type (GTypeModule *type_module)				\
{										\
	GTypeModule *module G_GNUC_UNUSED = type_module; /* back compat */			\
	static const GTypeInfo our_info =					\
	{									\
		sizeof (ObjectName##Class),					\
		NULL, /* base_init */						\
		NULL, /* base_finalize */					\
		(GClassInitFunc) object_name##_class_intern_init,		\
		NULL,								\
		NULL, /* class_data */						\
		sizeof (ObjectName),						\
		0, /* n_preallocs */						\
		(GInstanceInitFunc) object_name##_init				\
	};									\
										\
	g_define_type_id = g_type_module_register_type (type_module,		\
					   	        PARENT_TYPE,		\
					                #ObjectName,		\
					                &our_info,		\
					                0);			\
										\
	CODE									\
										\
	return g_define_type_id;						\
}


/**
 * PROPONO_PLUGIN_DEFINE_TYPE(ObjectName, object_name, PARENT_TYPE):
 *
 * Utility macro used to register gobject types in plugins.
 *
 * Deprecated: use G_DEFINE_DYNAMIC instead
 */
#define PROPONO_PLUGIN_DEFINE_TYPE(ObjectName, object_name, PARENT_TYPE)		\
	PROPONO_PLUGIN_DEFINE_TYPE_WITH_CODE(ObjectName, object_name, PARENT_TYPE, ;)

/**
 * PROPONO_PLUGIN_IMPLEMENT_INTERFACE(TYPE_IFACE, iface_init):
 *
 * Utility macro used to register interfaces for gobject types in plugins.
 */
#define PROPONO_PLUGIN_IMPLEMENT_INTERFACE(object_name, TYPE_IFACE, iface_init)	\
	const GInterfaceInfo object_name##_interface_info = 			\
	{ 									\
		(GInterfaceInitFunc) iface_init,				\
		NULL, 								\
		NULL								\
	};									\
										\
	g_type_module_add_interface (type_module, 					\
				     g_define_type_id, 				\
				     TYPE_IFACE, 				\
				     &object_name##_interface_info);

G_END_DECLS

#endif  /* __PROPONO_PLUGIN_H__ */
/* vim: set ts=8: */
