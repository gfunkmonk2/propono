/*
 * propono-bookmarks-tree.h
 * This file is part of propono
 *
 * Copyright (C) Jonh Wendell 2008 <wendell@bani.com.br>
 * 
 * propono is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 * 
 * propono is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License along
 * with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef _PROPONO_BOOKMARKS_TREE_H_
#define _PROPONO_BOOKMARKS_TREE_H_

#include <glib-object.h>
#include <gtk/gtk.h>
#include "propono-bookmarks-entry.h"

G_BEGIN_DECLS

#define PROPONO_TYPE_BOOKMARKS_TREE             (propono_bookmarks_tree_get_type ())
#define PROPONO_BOOKMARKS_TREE(obj)             (G_TYPE_CHECK_INSTANCE_CAST ((obj), PROPONO_TYPE_BOOKMARKS_TREE, ProponoBookmarksTree))
#define PROPONO_BOOKMARKS_TREE_CLASS(klass)     (G_TYPE_CHECK_CLASS_CAST ((klass), PROPONO_TYPE_BOOKMARKS_TREE, ProponoBookmarksTreeClass))
#define PROPONO_IS_BOOKMARKS_TREE(obj)          (G_TYPE_CHECK_INSTANCE_TYPE ((obj), PROPONO_TYPE_BOOKMARKS_TREE))
#define PROPONO_IS_BOOKMARKS_TREE_CLASS(klass)  (G_TYPE_CHECK_CLASS_TYPE ((klass), PROPONO_TYPE_BOOKMARKS_TREE))
#define PROPONO_BOOKMARKS_TREE_GET_CLASS(obj)   (G_TYPE_INSTANCE_GET_CLASS ((obj), PROPONO_TYPE_BOOKMARKS_TREE, ProponoBookmarksTreeClass))

typedef struct _ProponoBookmarksTreeClass   ProponoBookmarksTreeClass;
typedef struct _ProponoBookmarksTree        ProponoBookmarksTree;
typedef struct _ProponoBookmarksTreePrivate ProponoBookmarksTreePrivate;

struct _ProponoBookmarksTreeClass
{
  GtkVBoxClass parent_class;
};

struct _ProponoBookmarksTree
{
  GtkVBox parent_instance;
  ProponoBookmarksTreePrivate *priv;
};

GType			propono_bookmarks_tree_get_type (void) G_GNUC_CONST;

GtkWidget*		propono_bookmarks_tree_new (void);
gboolean		propono_bookmarks_tree_select_entry       (ProponoBookmarksTree *tree,
								   ProponoBookmarksEntry *entry);
ProponoBookmarksEntry*	propono_bookmarks_tree_get_selected_entry (ProponoBookmarksTree *tree);

G_END_DECLS

#endif /* _PROPONO_BOOKMARKS_TREE_H_ */

/* vim: set ts=8: */
