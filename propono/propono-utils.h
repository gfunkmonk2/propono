/*
 * propono-utils.h
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

#ifndef __PROPONO_UTILS_H__
#define __PROPONO_UTILS_H__

#include <gtk/gtk.h>
#include <glib.h>
#include "propono-plugin.h"

/* useful macro */
#define GBOOLEAN_TO_POINTER(i) ((gpointer) ((i) ? 2 : 1))
#define GPOINTER_TO_BOOLEAN(i) ((gboolean) ((((gint)(i)) == 2) ? TRUE : FALSE))
#define IS_VALID_BOOLEAN(v) (((v == TRUE) || (v == FALSE)) ? TRUE : FALSE)

enum { PROPONO_ALL_WORKSPACES = 0xffffffff };

GtkWidget	*propono_utils_create_small_close_button (void);

void		propono_utils_show_error		(const gchar *title,
							 const gchar *message,
							 GtkWindow *parent);

void		propono_utils_show_many_errors		(const gchar *title,
							 GSList *items,
							 GtkWindow *parent);

void		propono_utils_toggle_widget_visible	(GtkWidget *widget);

const gchar	*propono_utils_get_ui_filename		(void);
const gchar	*propono_utils_get_ui_xml_filename	(void);
GtkBuilder	*propono_utils_get_builder		(ProponoPlugin *plugin, const gchar *filename);

gchar		*propono_utils_escape_underscores	(const gchar *text,
							 gssize      length);

void		propono_utils_handle_debug		(void);

guint		propono_utils_get_current_workspace	(GdkScreen *screen);
guint		propono_utils_get_window_workspace	(GtkWindow *gtkwindow);
void		propono_utils_get_current_viewport	(GdkScreen    *screen,
							 gint         *x,
							 gint         *y);

void		propono_utils_help_contents		(GtkWindow *window, const gchar *section);
void		propono_utils_help_about		(GtkWindow *window);

gboolean	propono_utils_parse_boolean		(const gchar* value);

GtkWidget      *propono_gtk_button_new_with_stock_icon   (const gchar *label,
                                                         const gchar *stock_id);

gboolean	propono_utils_ask_question		(GtkWindow  *parent,
							 const char *message,
							 char       **choices,
							 int        *choice);

gboolean	propono_utils_ask_credential		(GtkWindow *parent,
							 gchar *kind,
							 gchar *host,
							 gboolean need_username,
							 gboolean need_password,
							 gint password_limit,
							 gchar **username,
							 gchar **password,
							 gboolean *save_in_keyring);

gboolean	propono_utils_create_dir		(const gchar *filename, GError **error);

#endif  /* __PROPONO_UTILS_H__  */
/* vim: set ts=8: */
