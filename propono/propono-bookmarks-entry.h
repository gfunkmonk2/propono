/*
 * propono-bookmarks-entry.h
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

#ifndef __PROPONO_BOOKMARKS_ENTRY_H__
#define __PROPONO_BOOKMARKS_ENTRY_H__

#include <glib-object.h>
#include "propono-connection.h"

G_BEGIN_DECLS

#define PROPONO_TYPE_BOOKMARKS_ENTRY             (propono_bookmarks_entry_get_type ())
#define PROPONO_BOOKMARKS_ENTRY(obj)             (G_TYPE_CHECK_INSTANCE_CAST ((obj), PROPONO_TYPE_BOOKMARKS_ENTRY, ProponoBookmarksEntry))
#define PROPONO_BOOKMARKS_ENTRY_CLASS(klass)     (G_TYPE_CHECK_CLASS_CAST ((klass), PROPONO_TYPE_BOOKMARKS_ENTRY, ProponoBookmarksEntryClass))
#define PROPONO_IS_BOOKMARKS_ENTRY(obj)          (G_TYPE_CHECK_INSTANCE_TYPE ((obj), PROPONO_TYPE_BOOKMARKS_ENTRY))
#define PROPONO_IS_BOOKMARKS_ENTRY_CLASS(klass)  (G_TYPE_CHECK_CLASS_TYPE ((klass), PROPONO_TYPE_BOOKMARKS_ENTRY))
#define PROPONO_BOOKMARKS_ENTRY_GET_CLASS(obj)   (G_TYPE_INSTANCE_GET_CLASS ((obj), PROPONO_TYPE_BOOKMARKS_ENTRY, ProponoBookmarksEntryClass))

typedef struct _ProponoBookmarksEntryClass   ProponoBookmarksEntryClass;
typedef struct _ProponoBookmarksEntry        ProponoBookmarksEntry;
typedef struct _ProponoBookmarksEntryPrivate ProponoBookmarksEntryPrivate;

typedef enum {
  PROPONO_BOOKMARKS_ENTRY_NODE_INVALID = 0,
  PROPONO_BOOKMARKS_ENTRY_NODE_FOLDER,
  PROPONO_BOOKMARKS_ENTRY_NODE_CONN
} ProponoBookmarksEntryNode;

struct _ProponoBookmarksEntryClass
{
  GObjectClass parent_class;
};

struct _ProponoBookmarksEntry
{
  GObject parent_instance;
  ProponoBookmarksEntryPrivate *priv;
};


GType propono_bookmarks_entry_get_type (void) G_GNUC_CONST;

ProponoBookmarksEntry *		propono_bookmarks_entry_new_folder  (const gchar *name);
ProponoBookmarksEntry *		propono_bookmarks_entry_new_conn    (ProponoConnection *conn);

void				propono_bookmarks_entry_add_child   (ProponoBookmarksEntry *entry, ProponoBookmarksEntry *child);
gboolean			propono_bookmarks_entry_remove_child(ProponoBookmarksEntry *entry,
								     ProponoBookmarksEntry *child);

GSList *			propono_bookmarks_entry_get_children(ProponoBookmarksEntry *entry);

void				propono_bookmarks_entry_set_node    (ProponoBookmarksEntry *entry, ProponoBookmarksEntryNode node);
ProponoBookmarksEntryNode	propono_bookmarks_entry_get_node    (ProponoBookmarksEntry *entry);

void				propono_bookmarks_entry_set_conn    (ProponoBookmarksEntry *entry, ProponoConnection *conn);
ProponoConnection *		propono_bookmarks_entry_get_conn    (ProponoBookmarksEntry *entry);

void				propono_bookmarks_entry_set_name    (ProponoBookmarksEntry *entry, const gchar *name);
const gchar *			propono_bookmarks_entry_get_name    (ProponoBookmarksEntry *entry);

//void				propono_bookmarks_entry_set_parent  (ProponoBookmarksEntry *entry, ProponoBookmarksEntry *parent);
ProponoBookmarksEntry *		propono_bookmarks_entry_get_parent  (ProponoBookmarksEntry *entry);

gint				propono_bookmarks_entry_compare     (ProponoBookmarksEntry *a, ProponoBookmarksEntry *b);
G_END_DECLS

#endif  /* __PROPONO_BOOKMARKS_ENTRY_H__ */

/* vim: set ts=8: */
