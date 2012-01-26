/*
 * propono-commands.h
 * This file is part of propono
 *
 * Copyright (C) 2007,2008,2009 - Jonh Wendell <wendell@bani.com.br>
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
 
#ifndef __PROPONO_COMMANDS_H__
#define __PROPONO_COMMANDS_H__

#include <gtk/gtk.h>
#include "propono-window.h"
#include "propono-connection.h"

G_BEGIN_DECLS

void		propono_cmd_direct_connect	(ProponoConnection *conn,
						 ProponoWindow     *window);

void		propono_cmd_machine_connect	(GtkAction     *action,
						 ProponoWindow *window);
void		propono_cmd_machine_open	(GtkAction     *action,
						 ProponoWindow *window);
void		propono_cmd_machine_close	(GtkAction     *action,
						 ProponoWindow *window);
void		propono_cmd_machine_take_screenshot (GtkAction     *action,
						     ProponoWindow *window);

void		propono_cmd_machine_close_all	(GtkAction     *action,
						 ProponoWindow *window);
void		propono_cmd_machine_quit	(GtkAction     *action,
						 ProponoWindow *window);

void		propono_cmd_edit_preferences	(GtkAction     *action,
						 ProponoWindow *window);

void		propono_cmd_edit_plugins	(GtkAction     *action,
						 ProponoWindow *window);

void		propono_cmd_view_show_toolbar	(GtkAction     *action,
						 ProponoWindow *window);
void		propono_cmd_view_show_statusbar	(GtkAction     *action,
						 ProponoWindow *window);
void		propono_cmd_view_show_fav_panel	(GtkAction     *action,
						 ProponoWindow *window);
void		propono_cmd_view_fullscreen	(GtkAction     *action,
						 ProponoWindow *window);

void		propono_cmd_open_bookmark	(ProponoWindow     *window,
						 ProponoConnection *conn);
void		propono_cmd_bookmarks_add	(GtkAction     *action,
						 ProponoWindow *window);

void		propono_cmd_help_contents	(GtkAction     *action,
						 ProponoWindow *window);
void		propono_cmd_help_about		(GtkAction     *action,
						 ProponoWindow *window);



G_END_DECLS

#endif /* __PROPONO_COMMANDS_H__ */ 
/* vim: set ts=8: */
