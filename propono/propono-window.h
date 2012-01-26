/*
 * propono-window.h
 * This file is part of propono
 *
 * Copyright (C) 2007,2009 - Jonh Wendell <wendell@bani.com.br>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANWINDOWILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program. If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef __PROPONO_WINDOW_H__
#define __PROPONO_WINDOW_H__

#include <gtk/gtk.h>

G_BEGIN_DECLS

#define PROPONO_TYPE_WINDOW              (propono_window_get_type())
#define PROPONO_WINDOW(obj)              (G_TYPE_CHECK_INSTANCE_CAST((obj), PROPONO_TYPE_WINDOW, ProponoWindow))
#define PROPONO_WINDOW_CONST(obj)        (G_TYPE_CHECK_INSTANCE_CAST((obj), PROPONO_TYPE_WINDOW, ProponoWindow const))
#define PROPONO_WINDOW_CLASS(klass)      (G_TYPE_CHECK_CLASS_CAST((klass), PROPONO_TYPE_WINDOW, ProponoWindowClass))
#define PROPONO_IS_WINDOW(obj)           (G_TYPE_CHECK_INSTANCE_TYPE((obj), PROPONO_TYPE_WINDOW))
#define PROPONO_IS_WINDOW_CLASS(klass)   (G_TYPE_CHECK_CLASS_TYPE ((klass), PROPONO_TYPE_WINDOW))
#define PROPONO_WINDOW_GET_CLASS(obj)    (G_TYPE_INSTANCE_GET_CLASS((obj), PROPONO_TYPE_WINDOW, ProponoWindowClass))

typedef struct _ProponoWindowPrivate ProponoWindowPrivate;
typedef struct _ProponoWindow        ProponoWindow;
typedef struct _ProponoWindowClass   ProponoWindowClass;

#include "propono-tab.h"
#include "propono-bookmarks.h"

struct _ProponoWindow 
{
  GtkWindow window;
  ProponoWindowPrivate *priv;
};

struct _ProponoWindowClass 
{
  GtkWindowClass parent_class;
};

GType 		 propono_window_get_type 		(void) G_GNUC_CONST;

ProponoWindow   *propono_window_new			(void);

void		 propono_window_close_tab		(ProponoWindow         *window,
							 ProponoTab            *tab);
void		 propono_window_close_active_tab	(ProponoWindow         *window);

void		 propono_window_close_all_tabs		(ProponoWindow         *window);

void		 propono_window_close_tabs		(ProponoWindow         *window,
							 const GList           *tabs);
							 
ProponoTab	*propono_window_get_active_tab		(ProponoWindow         *window);

void		 propono_window_set_active_tab		(ProponoWindow         *window,
							 ProponoTab            *tab);

GtkWidget	*propono_window_get_statusbar		(ProponoWindow         *window);
GtkWidget	*propono_window_get_toolbar		(ProponoWindow         *window);
GtkWidget	*propono_window_get_menubar		(ProponoWindow         *window);
GtkWidget	*propono_window_get_fav_panel		(ProponoWindow         *window);
ProponoNotebook	*propono_window_get_notebook		(ProponoWindow	       *window);
GtkActionGroup	*propono_window_get_initialized_action	(ProponoWindow         *window);
GtkActionGroup	*propono_window_get_always_sensitive_action (ProponoWindow     *window);
GtkActionGroup	*propono_window_get_connected_action	(ProponoWindow         *window);

void		propono_window_update_bookmarks_list_menu (ProponoWindow       *window);

GtkUIManager	*propono_window_get_ui_manager		(ProponoWindow         *window);

gboolean	propono_window_is_fullscreen		(ProponoWindow         *window);

void		propono_window_toggle_fullscreen	(ProponoWindow *window);

void		propono_window_merge_tab_ui (ProponoWindow	*window);

GList 		*propono_window_get_connections		(ProponoWindow *window);
ProponoTab	*propono_window_conn_exists		(ProponoWindow *window,
							 ProponoConnection *conn);

G_END_DECLS

#endif  /* __PROPONO_WINDOW_H__  */
/* vim: set ts=8: */
