/*
 * propono-app.h
 * This file is part of propono
 *
 * Copyright (C) 2008 - Jonh Wendell <wendell@bani.com.br>
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
 * along with this program. If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef __PROPONO_APP_H__
#define __PROPONO_APP_H__

#include <gtk/gtk.h>

#include "propono-window.h"

G_BEGIN_DECLS

#define PROPONO_TYPE_APP              (propono_app_get_type())
#define PROPONO_APP(obj)              (G_TYPE_CHECK_INSTANCE_CAST((obj), PROPONO_TYPE_APP, ProponoApp))
#define PROPONO_APP_CLASS(klass)      (G_TYPE_CHECK_CLASS_CAST((klass), PROPONO_TYPE_APP, ProponoAppClass))
#define PROPONO_IS_APP(obj)           (G_TYPE_CHECK_INSTANCE_TYPE((obj), PROPONO_TYPE_APP))
#define PROPONO_IS_APP_CLASS(klass)   (G_TYPE_CHECK_CLASS_TYPE ((klass), PROPONO_TYPE_APP))
#define PROPONO_APP_GET_CLASS(obj)    (G_TYPE_INSTANCE_GET_CLASS((obj), PROPONO_TYPE_APP, ProponoAppClass))

typedef struct _ProponoAppPrivate ProponoAppPrivate;
typedef struct _ProponoApp ProponoApp;
typedef struct _ProponoAppClass ProponoAppClass;

struct _ProponoApp 
{
  GObject object;
  ProponoAppPrivate *priv;
};


struct _ProponoAppClass 
{
  GObjectClass parent_class;
};


GType		propono_app_get_type		(void) G_GNUC_CONST;

ProponoApp 	*propono_app_get_default	(void);

ProponoWindow	*propono_app_create_window	(ProponoApp  *app,
						 GdkScreen *screen);

const GList	*propono_app_get_windows	(ProponoApp *app);
ProponoWindow	*propono_app_get_active_window	(ProponoApp *app);
GList		*propono_app_get_connections	(ProponoApp *app);

ProponoWindow	*propono_app_get_window_in_viewport (ProponoApp *app,
						     GdkScreen  *screen,
						     gint        workspace,
						     gint        viewport_x,
						     gint        viewport_y);

G_END_DECLS

#endif  /* __PROPONO_APP_H__  */
/* vim: set ts=8: */
