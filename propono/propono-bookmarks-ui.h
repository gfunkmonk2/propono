/*
 * propono-bookmarks-ui.h
 * This file is part of propono
 *
 * Copyright (C) 2008  Jonh Wendell <wendell@bani.com.br>
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

#ifndef __PROPONO_BOOKMARKS_UI_H__
#define __PROPONO_BOOKMARKS_UI_H__

#include <glib.h>
#include <gtk/gtk.h>

#include "propono-connection.h"
#include "propono-bookmarks.h"
#include "propono-bookmarks-entry.h"
#include "propono-window.h"

G_BEGIN_DECLS

void   propono_bookmarks_add        (ProponoBookmarks  *book,
                                     ProponoConnection *conn,
                                     GtkWindow         *window);
void   propono_bookmarks_del        (ProponoBookmarks      *book,
                                     ProponoBookmarksEntry *entry,
                                     GtkWindow             *window);
void   propono_bookmarks_edit       (ProponoBookmarks      *book,
                                     ProponoBookmarksEntry *entry,
                                     GtkWindow             *window);
void   propono_bookmarks_new_folder (ProponoBookmarks      *book,
                                     GtkWindow             *window);

G_END_DECLS

#endif  /* __PROPONO_BOOKMARKS_UI_H__ */

/* vim: set ts=8: */
