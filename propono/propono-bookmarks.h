/*
 * propono-bookmarks.h
 * This file is part of propono
 *
 * Copyright (C) 2007,2008  Jonh Wendell <wendell@bani.com.br>
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

#ifndef __PROPONO_BOOKMARKS_H__
#define __PROPONO_BOOKMARKS_H__

#include <glib.h>
#include <glib-object.h>

#include "propono-connection.h"
#include "propono-bookmarks-entry.h"

G_BEGIN_DECLS

#define PROPONO_BOOKMARKS_FILE      "propono-bookmarks.xml"
#define PROPONO_BOOKMARKS_FILE_OLD  "propono.bookmarks"

#define PROPONO_TYPE_BOOKMARKS             (propono_bookmarks_get_type ())
#define PROPONO_BOOKMARKS(obj)             (G_TYPE_CHECK_INSTANCE_CAST ((obj), PROPONO_TYPE_BOOKMARKS, ProponoBookmarks))
#define PROPONO_BOOKMARKS_CLASS(klass)     (G_TYPE_CHECK_CLASS_CAST ((klass), PROPONO_TYPE_BOOKMARKS, ProponoBookmarksClass))
#define PROPONO_IS_BOOKMARKS(obj)          (G_TYPE_CHECK_INSTANCE_TYPE ((obj), PROPONO_TYPE_BOOKMARKS))
#define PROPONO_IS_BOOKMARKS_CLASS(klass)  (G_TYPE_CHECK_CLASS_TYPE ((klass), PROPONO_TYPE_BOOKMARKS))
#define PROPONO_BOOKMARKS_GET_CLASS(obj)   (G_TYPE_INSTANCE_GET_CLASS ((obj), PROPONO_TYPE_BOOKMARKS, ProponoBookmarksClass))

typedef struct _ProponoBookmarksClass   ProponoBookmarksClass;
typedef struct _ProponoBookmarks        ProponoBookmarks;
typedef struct _ProponoBookmarksPrivate ProponoBookmarksPrivate;

struct _ProponoBookmarksClass
{
  GObjectClass parent_class;

  /* Signals */
  void (* changed) (ProponoBookmarks *book);
};

struct _ProponoBookmarks
{
  GObject parent_instance;
  ProponoBookmarksPrivate *priv;
};

GType propono_bookmarks_get_type (void) G_GNUC_CONST;

ProponoBookmarks   *propono_bookmarks_get_default  (void);
GSList             *propono_bookmarks_get_all      (ProponoBookmarks *book);
void                propono_bookmarks_save_to_file (ProponoBookmarks *book);
void                propono_bookmarks_add_entry    (ProponoBookmarks      *book,
                                                    ProponoBookmarksEntry *entry,
                                                    ProponoBookmarksEntry *parent);
gboolean           propono_bookmarks_remove_entry  (ProponoBookmarks      *book,
                                                    ProponoBookmarksEntry *entry);

ProponoConnection  *propono_bookmarks_exists       (ProponoBookmarks *book,
                                                    const gchar      *protocol,
                                                    const gchar      *host,
                                                    gint              port);

ProponoBookmarksEntry *propono_bookmarks_name_exists (ProponoBookmarks      *book,
                                                      ProponoBookmarksEntry *parent,
                                                      const gchar           *name);

G_END_DECLS
#endif  /* __PROPONO_BOOKMARKS_H__ */
/* vim: set ts=8: */
