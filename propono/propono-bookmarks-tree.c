/*
 * propono-bookmarks-tree.c
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

#include "propono-bookmarks-tree.h"
#include "propono-bookmarks-entry.h"
#include "propono-bookmarks.h"

#include <gtk/gtk.h>
#include <glib/gi18n.h>

struct _ProponoBookmarksTreePrivate
{
  GtkWidget *tree;
};

/* TreeModel */
enum {
  IMAGE_COL = 0,
  NAME_COL,
  ENTRY_COL,
  NUM_COLS
};

G_DEFINE_TYPE (ProponoBookmarksTree, propono_bookmarks_tree, GTK_TYPE_VBOX);

static void
propono_bookmarks_tree_row_activated_cb (GtkTreeView          *treeview,
					 GtkTreePath          *path,
					 GtkTreeViewColumn    *column,
					 ProponoBookmarksTree *tree)
{
  GtkTreeIter   iter;
  GtkTreeModel *model;
  GtkWidget    *toplevel;

  model = gtk_tree_view_get_model (treeview);

  gtk_tree_model_get_iter (model, &iter, path);
  if (gtk_tree_model_iter_has_child (model, &iter))
    {
      if (gtk_tree_view_row_expanded (treeview, path))
        gtk_tree_view_collapse_row (treeview, path);
      else
        gtk_tree_view_expand_row (treeview, path, FALSE);
      return;
    }
  else
    {
      toplevel = gtk_widget_get_toplevel (GTK_WIDGET (tree));
      if (GTK_IS_WINDOW (toplevel))
	gtk_window_activate_default (GTK_WINDOW (toplevel));
    }
}

static void
propono_bookmarks_fill_tree (GSList *entries, GtkTreeStore *store, GtkTreeIter *parent, GdkPixbuf *pixbuf)
{
  GSList                *l;
  GtkTreeIter            iter;
  ProponoBookmarksEntry *entry;

  for (l = entries; l; l = l->next)
    {
      entry = PROPONO_BOOKMARKS_ENTRY (l->data);
      if (propono_bookmarks_entry_get_node (entry) != PROPONO_BOOKMARKS_ENTRY_NODE_FOLDER)
	continue;

      gtk_tree_store_append (store, &iter, parent);
      gtk_tree_store_set (store, &iter,
                          IMAGE_COL, pixbuf,
                          NAME_COL, propono_bookmarks_entry_get_name (entry),
                          ENTRY_COL, entry,
		          -1);
      propono_bookmarks_fill_tree (propono_bookmarks_entry_get_children (entry),
				   store,
				   &iter,
				   pixbuf);
    }
}

static gboolean
propono_bookmarks_tree_update_list (ProponoBookmarksTree *tree)
{
  GtkTreeStore     *model;
  GtkTreeIter       iter;
  GdkPixbuf        *pixbuf;
  GtkIconTheme     *icon_theme;
  GtkTreeSelection *selection;
  GtkTreePath      *path;

  icon_theme = gtk_icon_theme_get_default ();
  pixbuf = gtk_icon_theme_load_icon  (icon_theme,
                                      "folder",
                                      16,
                                      0,
                                      NULL);

  model = GTK_TREE_STORE (gtk_tree_view_get_model (GTK_TREE_VIEW (tree->priv->tree)));

  gtk_tree_store_append (model, &iter, NULL);
  gtk_tree_store_set (model, &iter,
                      IMAGE_COL, pixbuf,
                      NAME_COL, _("Root Folder"),
		      -1);

  propono_bookmarks_fill_tree (propono_bookmarks_get_all (propono_bookmarks_get_default ()),
			       model,
			       NULL,
			       pixbuf);

  selection = gtk_tree_view_get_selection (GTK_TREE_VIEW (tree->priv->tree));
  gtk_tree_selection_select_iter (selection, &iter);

  g_object_unref (pixbuf);
  return FALSE;
}

static void
propono_bookmarks_tree_init (ProponoBookmarksTree *tree)
{
  GtkCellRenderer   *cell;
  GtkWidget         *scroll;
  GtkTreeSelection  *selection;
  GtkTreeStore      *model;
  GtkTreeViewColumn *main_column;

  tree->priv = G_TYPE_INSTANCE_GET_PRIVATE (tree, PROPONO_TYPE_BOOKMARKS_TREE, ProponoBookmarksTreePrivate);

  /* Create the scrolled window */
  scroll = gtk_scrolled_window_new (NULL, NULL);
  gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (scroll),
                                  GTK_POLICY_AUTOMATIC,
                                  GTK_POLICY_AUTOMATIC);
  gtk_scrolled_window_set_shadow_type (GTK_SCROLLED_WINDOW (scroll),
                                       GTK_SHADOW_ETCHED_OUT);
  gtk_widget_set_size_request (scroll, 200, 180);
  gtk_box_pack_start (GTK_BOX (tree), scroll, TRUE, TRUE, 0);

  /* Create the model */
  model = gtk_tree_store_new (NUM_COLS,
                              GDK_TYPE_PIXBUF,
                              G_TYPE_STRING,
                              PROPONO_TYPE_BOOKMARKS_ENTRY);

  tree->priv->tree = gtk_tree_view_new_with_model (GTK_TREE_MODEL (model));
  gtk_tree_view_set_headers_visible (GTK_TREE_VIEW (tree->priv->tree), FALSE);
  selection = gtk_tree_view_get_selection (GTK_TREE_VIEW (tree->priv->tree));
  gtk_tree_selection_set_mode (selection, GTK_SELECTION_SINGLE);
  g_object_unref (model);

  main_column = gtk_tree_view_column_new ();
  gtk_tree_view_column_set_clickable (main_column, FALSE);
  gtk_tree_view_append_column (GTK_TREE_VIEW (tree->priv->tree), main_column);

  /* Set up the pixbuf column */
  cell = gtk_cell_renderer_pixbuf_new ();
  gtk_tree_view_column_pack_start (main_column, cell, FALSE);
  gtk_tree_view_column_add_attribute (main_column, cell, "pixbuf", IMAGE_COL);

  /* Set up the name column */
  cell = gtk_cell_renderer_text_new ();
  gtk_tree_view_column_pack_start (main_column, cell, TRUE);
  gtk_tree_view_column_add_attribute (main_column, cell, "text", NAME_COL);

  g_signal_connect (tree->priv->tree,
		    "row-activated",
		    G_CALLBACK (propono_bookmarks_tree_row_activated_cb),
		    tree);

  propono_bookmarks_tree_update_list (tree);

  gtk_container_add (GTK_CONTAINER(scroll), tree->priv->tree);
  gtk_widget_show (tree->priv->tree);
}

static void
propono_bookmarks_tree_class_init (ProponoBookmarksTreeClass *klass)
{
  GObjectClass* object_class = G_OBJECT_CLASS (klass);

  g_type_class_add_private (object_class, sizeof (ProponoBookmarksTreePrivate));
}

GtkWidget *
propono_bookmarks_tree_new (void)
{
  return GTK_WIDGET (g_object_new (PROPONO_TYPE_BOOKMARKS_TREE, NULL));
}

ProponoBookmarksEntry *
propono_bookmarks_tree_get_selected_entry (ProponoBookmarksTree *tree)
{
  GtkTreeSelection      *selection;
  GtkTreeIter            iter;
  GtkTreeModel          *model;
  ProponoBookmarksEntry *entry;

  g_return_val_if_fail (PROPONO_IS_BOOKMARKS_TREE (tree), NULL);

  selection = gtk_tree_view_get_selection (GTK_TREE_VIEW (tree->priv->tree));
  if (gtk_tree_selection_get_selected (selection, NULL, &iter))
    {
      model = gtk_tree_view_get_model (GTK_TREE_VIEW (tree->priv->tree));
      gtk_tree_model_get (model, 
			  &iter,
			  ENTRY_COL, &entry,
			  -1);
      if (entry)
	g_object_unref (entry);
      return entry;
    }
  else
    return NULL;
}

struct _find_entry
{
  ProponoBookmarksEntry *entry;
  gboolean               found;
  GtkTreePath           *path;
};

static gboolean
find_entry (GtkTreeModel *model,
	    GtkTreePath  *path,
	    GtkTreeIter  *iter,
	    gpointer      data)
{
  ProponoBookmarksEntry *entry;
  struct _find_entry    *f = data;
  gboolean              result = FALSE;

  gtk_tree_model_get (model, 
		      iter,
		      ENTRY_COL, &entry,
			  -1);
  if (entry == f->entry)
    {
      f->found = TRUE;
      f->path  = gtk_tree_path_copy (path);
      result = TRUE;
    }

  if (entry)
    g_object_unref (entry);

  return result;
}

gboolean
propono_bookmarks_tree_select_entry (ProponoBookmarksTree *tree,
				     ProponoBookmarksEntry *entry)
{
  GtkTreeModel          *model;
  GtkTreeIter            iter;
  GtkTreePath           *path;
  gboolean               valid;
  struct _find_entry     f;

  if (!entry)
    return FALSE;
  g_return_val_if_fail (PROPONO_IS_BOOKMARKS_TREE (tree), FALSE);
  g_return_val_if_fail (PROPONO_IS_BOOKMARKS_ENTRY (entry), FALSE);

  model = gtk_tree_view_get_model (GTK_TREE_VIEW (tree->priv->tree));
  f.entry = entry;
  f.found = FALSE;

  gtk_tree_model_foreach (model, find_entry, &f);
  if (f.found)
    {
      gtk_tree_view_expand_to_path (GTK_TREE_VIEW (tree->priv->tree), f.path);
      gtk_tree_view_set_cursor (GTK_TREE_VIEW (tree->priv->tree), f.path, NULL, FALSE);
      gtk_tree_path_free (f.path);
      return TRUE;
    }
  else
    return FALSE;
}

/* vim: set ts=8: */
