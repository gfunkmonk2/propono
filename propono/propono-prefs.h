/*
 * propono-prefs.c
 * This file is part of propono
 *
 * Copyright (C) Jonh Wendell 2008 <wendell@bani.com.br>
 * 
 * propono-prefs.c is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation, either version 2 of the License, or
 * (at your option) any later version.
 * 
 * propono-prefs.c is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License along
 * with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef _PROPONO_PREFS_H_
#define _PROPONO_PREFS_H_

#include <glib-object.h>
#include "propono-window.h"

G_BEGIN_DECLS

#define PROPONO_TYPE_PREFS             (propono_prefs_get_type ())
#define PROPONO_PREFS(obj)             (G_TYPE_CHECK_INSTANCE_CAST ((obj), PROPONO_TYPE_PREFS, ProponoPrefs))
#define PROPONO_PREFS_CLASS(klass)     (G_TYPE_CHECK_CLASS_CAST ((klass), PROPONO_TYPE_PREFS, ProponoPrefsClass))
#define PROPONO_IS_PREFS(obj)          (G_TYPE_CHECK_INSTANCE_TYPE ((obj), PROPONO_TYPE_PREFS))
#define PROPONO_IS_PREFS_CLASS(klass)  (G_TYPE_CHECK_CLASS_TYPE ((klass), PROPONO_TYPE_PREFS))
#define PROPONO_PREFS_GET_CLASS(obj)   (G_TYPE_INSTANCE_GET_CLASS ((obj), PROPONO_TYPE_PREFS, ProponoPrefsClass))

typedef struct _ProponoPrefsClass ProponoPrefsClass;
typedef struct _ProponoPrefs ProponoPrefs;
typedef struct _ProponoPrefsPrivate ProponoPrefsPrivate;

struct _ProponoPrefsClass
{
  GObjectClass parent_class;
};

struct _ProponoPrefs
{
  GObject parent_instance;
  ProponoPrefsPrivate *priv;
};

GType propono_prefs_get_type (void) G_GNUC_CONST;

ProponoPrefs	*propono_prefs_get_default (void);

void		propono_prefs_dialog_show (ProponoWindow *window);
G_END_DECLS

#endif /* _PROPONO_PREFS_H_ */
/* vim: set ts=8: */
