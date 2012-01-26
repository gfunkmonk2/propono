/*
 * propono-bookmarks-migration.h
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

/*
 * This is a temporary hack to migrate the bookmarks from an .ini format to
 * a XML one. The new format is used in Propono 2.25.
 */

#ifndef __PROPONO_BOOKMARKS_MIGRATION_H__
#define __PROPONO_BOOKMARKS_MIGRATION_H__

G_BEGIN_DECLS

void propono_bookmarks_migration_migrate (const gchar *filename);

G_END_DECLS

#endif  /* __PROPONO_BOOKMARKS_MIGRATION_H__ */

/* vim: set ts=8: */
