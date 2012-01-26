/*
 * propono-object-module.h
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
 
/* This is a modified version of propono-module.h from Epiphany source code.
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
 * $Id: propono-module.h 6263 2008-05-05 10:52:10Z sfre $
 */
 
#ifndef __PROPONO_OBJECT_MODULE_H__
#define __PROPONO_OBJECT_MODULE_H__

#include <glib-object.h>
#include <gmodule.h>
#include <stdarg.h>

G_BEGIN_DECLS

#define PROPONO_TYPE_OBJECT_MODULE		(propono_object_module_get_type ())
#define PROPONO_OBJECT_MODULE(obj)		(G_TYPE_CHECK_INSTANCE_CAST ((obj), PROPONO_TYPE_OBJECT_MODULE, ProponoObjectModule))
#define PROPONO_OBJECT_MODULE_CLASS(klass)	(G_TYPE_CHECK_CLASS_CAST ((klass), PROPONO_TYPE_OBJECT_MODULE, ProponoObjectModuleClass))
#define PROPONO_IS_OBJECT_MODULE(obj)		(G_TYPE_CHECK_INSTANCE_TYPE ((obj), PROPONO_TYPE_OBJECT_MODULE))
#define PROPONO_IS_OBJECT_MODULE_CLASS(klass)	(G_TYPE_CHECK_CLASS_TYPE ((klass), PROPONO_TYPE_OBJECT_MODULE))
#define PROPONO_OBJECT_MODULE_GET_CLASS(obj)	(G_TYPE_INSTANCE_GET_CLASS((obj), PROPONO_TYPE_OBJECT_MODULE, ProponoObjectModuleClass))

typedef struct _ProponoObjectModule 		ProponoObjectModule;
typedef struct _ProponoObjectModulePrivate	ProponoObjectModulePrivate;

struct _ProponoObjectModule
{
	GTypeModule parent;

	ProponoObjectModulePrivate *priv;
};

typedef struct _ProponoObjectModuleClass ProponoObjectModuleClass;

struct _ProponoObjectModuleClass
{
	GTypeModuleClass parent_class;

	/* Virtual class methods */
	void		 (* garbage_collect)	();
};

GType		 propono_object_module_get_type			(void) G_GNUC_CONST;

ProponoObjectModule *propono_object_module_new			(const gchar *module_name,
								 const gchar *path,
								 const gchar *type_registration,
								 gboolean     resident);

GObject		*propono_object_module_new_object			(ProponoObjectModule *module, 
								 const gchar	   *first_property_name,
								 ...);

GType		 propono_object_module_get_object_type		(ProponoObjectModule *module);
const gchar	*propono_object_module_get_path			(ProponoObjectModule *module);
const gchar	*propono_object_module_get_module_name		(ProponoObjectModule *module);
const gchar 	*propono_object_module_get_type_registration 	(ProponoObjectModule *module);

G_END_DECLS

#endif
