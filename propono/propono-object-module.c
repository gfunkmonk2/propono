/*
 * propono-object-module.c
 * This file is part of propono
 *
 * Copyright (C) 2005 - Paolo Maggi
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
 
/* This is a modified version of ephy-module.c from Epiphany source code.
 * Here the original copyright assignment:
 *
 *  Copyright (C) 2003 Marco Pesenti Gritti
 *  Copyright (C) 2003, 2004 Christian Persch
 *
 */

/*
 * Modified by the propono Team, 2005. See the AUTHORS file for a 
 * list of people on the propono Team.  
 * See the ChangeLog files for a list of changes. 
 *
 * $Id: propono-module.c 6314 2008-06-05 12:57:53Z pborelli $
 */

#include "config.h"

#include "propono-object-module.h"
#include "propono-debug.h"

typedef GType (*ProponoObjectModuleRegisterFunc) (GTypeModule *);

enum {
	PROP_0,
	PROP_MODULE_NAME,
	PROP_PATH,
	PROP_TYPE_REGISTRATION,
	PROP_RESIDENT
};

struct _ProponoObjectModulePrivate
{
	GModule *library;

	GType type;
	gchar *path;
	gchar *module_name;
	gchar *type_registration;

	gboolean resident;
};

G_DEFINE_TYPE (ProponoObjectModule, propono_object_module, G_TYPE_TYPE_MODULE);

static gboolean
propono_object_module_load (GTypeModule *gmodule)
{
	ProponoObjectModule *module = PROPONO_OBJECT_MODULE (gmodule);
	ProponoObjectModuleRegisterFunc register_func;
	gchar *path;

	propono_debug_message (DEBUG_PLUGINS, "Loading %s module from %s",
			     module->priv->module_name, module->priv->path);

	path = g_module_build_path (module->priv->path, module->priv->module_name);
	g_return_val_if_fail (path != NULL, FALSE);
	propono_debug_message (DEBUG_PLUGINS, "Module filename: %s", path);

	module->priv->library = g_module_open (path, 
					       G_MODULE_BIND_LAZY);
	g_free (path);

	if (module->priv->library == NULL)
	{
		g_warning ("%s: %s", module->priv->module_name, g_module_error());

		return FALSE;
	}

	/* extract symbols from the lib */
	if (!g_module_symbol (module->priv->library, module->priv->type_registration,
			      (void *) &register_func))
	{
		g_warning ("%s: %s", module->priv->module_name, g_module_error());
		g_module_close (module->priv->library);

		return FALSE;
	}

	/* symbol can still be NULL even though g_module_symbol
	 * returned TRUE */
	if (register_func == NULL)
	{
		g_warning ("Symbol '%s' should not be NULL", module->priv->type_registration);
		g_module_close (module->priv->library);

		return FALSE;
	}

	module->priv->type = register_func (gmodule);

	if (module->priv->type == 0)
	{
		g_warning ("Invalid object contained by module %s", module->priv->module_name);
		return FALSE;
	}

	if (module->priv->resident)
	{
		g_module_make_resident (module->priv->library);
	}

	return TRUE;
}

static void
propono_object_module_unload (GTypeModule *gmodule)
{
	ProponoObjectModule *module = PROPONO_OBJECT_MODULE (gmodule);

	propono_debug_message (DEBUG_PLUGINS, "Unloading %s", module->priv->path);

	g_module_close (module->priv->library);

	module->priv->library = NULL;
	module->priv->type = 0;
}

static void
propono_object_module_init (ProponoObjectModule *module)
{
	propono_debug_message (DEBUG_PLUGINS, "ProponoObjectModule %p initialising", module);
	
	module->priv = G_TYPE_INSTANCE_GET_PRIVATE (module,
						    PROPONO_TYPE_OBJECT_MODULE,
						    ProponoObjectModulePrivate);
}

static void
propono_object_module_finalize (GObject *object)
{
	ProponoObjectModule *module = PROPONO_OBJECT_MODULE (object);

	propono_debug_message (DEBUG_PLUGINS, "ProponoObjectModule %p finalising", module);

	g_free (module->priv->path);
	g_free (module->priv->module_name);
	g_free (module->priv->type_registration);

	G_OBJECT_CLASS (propono_object_module_parent_class)->finalize (object);
}

static void
propono_object_module_get_property (GObject    *object,
			          guint       prop_id,
			          GValue     *value,
			          GParamSpec *pspec)
{
	ProponoObjectModule *module = PROPONO_OBJECT_MODULE (object);

	switch (prop_id)
	{
		case PROP_MODULE_NAME:
			g_value_set_string (value, module->priv->module_name);
			break;
		case PROP_PATH:
			g_value_set_string (value, module->priv->path);
			break;
		case PROP_TYPE_REGISTRATION:
			g_value_set_string (value, module->priv->type_registration);
			break;
		case PROP_RESIDENT:
			g_value_set_boolean (value, module->priv->resident);
			break;
		default:
			g_return_if_reached ();
	}
}

static void
propono_object_module_set_property (GObject      *object,
				  guint         prop_id,
				  const GValue *value,
				  GParamSpec   *pspec)
{
	ProponoObjectModule *module = PROPONO_OBJECT_MODULE (object);

	switch (prop_id)
	{
		case PROP_MODULE_NAME:
			module->priv->module_name = g_value_dup_string (value);
			g_type_module_set_name (G_TYPE_MODULE (object),
						module->priv->module_name);
			break;
		case PROP_PATH:
			module->priv->path = g_value_dup_string (value);
			break;
		case PROP_TYPE_REGISTRATION:
			module->priv->type_registration = g_value_dup_string (value);
			break;
		case PROP_RESIDENT:
			module->priv->resident = g_value_get_boolean (value);
			break;
		default:
			g_return_if_reached ();
	}
}

static void
propono_object_module_class_init (ProponoObjectModuleClass *klass)
{
	GObjectClass *object_class = G_OBJECT_CLASS (klass);
	GTypeModuleClass *module_class = G_TYPE_MODULE_CLASS (klass);

	object_class->set_property = propono_object_module_set_property;
	object_class->get_property = propono_object_module_get_property;
	object_class->finalize = propono_object_module_finalize;

	module_class->load = propono_object_module_load;
	module_class->unload = propono_object_module_unload;

	g_object_class_install_property (object_class,
					 PROP_MODULE_NAME,
					 g_param_spec_string ("module-name",
							      "Module Name",
							      "The module to load for this object",
							      NULL,
							      G_PARAM_READWRITE |
							      G_PARAM_CONSTRUCT_ONLY));

	g_object_class_install_property (object_class,
					 PROP_PATH,
					 g_param_spec_string ("path",
							      "Path",
							      "The path to use when loading this module",
							      NULL,
							      G_PARAM_READWRITE |
							      G_PARAM_CONSTRUCT_ONLY));
							      
	g_object_class_install_property (object_class,
					 PROP_TYPE_REGISTRATION,
					 g_param_spec_string ("type-registration",
							      "Type Registration",
							      "The name of the type registration function",
							      NULL,
							      G_PARAM_READWRITE |
							      G_PARAM_CONSTRUCT_ONLY));

	g_object_class_install_property (object_class,
					 PROP_RESIDENT,
					 g_param_spec_boolean ("resident",
							       "Resident",
							       "Whether the module is resident",
							       FALSE,
							       G_PARAM_READWRITE |
							       G_PARAM_CONSTRUCT_ONLY));

	g_type_class_add_private (klass, sizeof (ProponoObjectModulePrivate));
}

ProponoObjectModule *
propono_object_module_new (const gchar *module_name,
			 const gchar *path,
			 const gchar *type_registration,
			 gboolean     resident)
{
	return (ProponoObjectModule *)g_object_new (PROPONO_TYPE_OBJECT_MODULE,
						  "module-name",
						  module_name,
						  "path",
						  path,
						  "type-registration",
						  type_registration,
						  "resident",
						  resident,
						  NULL);
}

GObject *
propono_object_module_new_object (ProponoObjectModule *module,
				const gchar       *first_property_name,
				...)
{
	va_list var_args;
	GObject *result;
	
	g_return_val_if_fail (module->priv->type != 0, NULL);

	propono_debug_message (DEBUG_PLUGINS, "Creating object of type %s",
			     g_type_name (module->priv->type));

	va_start (var_args, first_property_name);
	result = g_object_new_valist (module->priv->type, first_property_name, var_args);
	va_end (var_args);
	
	return result;
}

const gchar *
propono_object_module_get_path (ProponoObjectModule *module)
{
	g_return_val_if_fail (PROPONO_IS_OBJECT_MODULE (module), NULL);

	return module->priv->path;
}

const gchar *
propono_object_module_get_module_name (ProponoObjectModule *module)
{
	g_return_val_if_fail (PROPONO_IS_OBJECT_MODULE (module), NULL);

	return module->priv->module_name;
}

const gchar *
propono_object_module_get_type_registration (ProponoObjectModule *module)
{
	g_return_val_if_fail (PROPONO_IS_OBJECT_MODULE (module), NULL);

	return module->priv->type_registration;
}

GType
propono_object_module_get_object_type (ProponoObjectModule *module)
{
	g_return_val_if_fail (PROPONO_IS_OBJECT_MODULE (module), 0);
	
	return module->priv->type;
}
