/*
 * propono-bookmarks-entry.c
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

#include "propono-bookmarks-entry.h"

struct _ProponoBookmarksEntryPrivate
{
  ProponoBookmarksEntryNode node;
  ProponoConnection        *conn;
  gchar                    *name;
  GSList                   *children; // array of ProponoBookmarksEntry
  ProponoBookmarksEntry    *parent;
};

G_DEFINE_TYPE (ProponoBookmarksEntry, propono_bookmarks_entry, G_TYPE_OBJECT);

static void
propono_bookmarks_entry_init (ProponoBookmarksEntry *entry)
{
  entry->priv = G_TYPE_INSTANCE_GET_PRIVATE (entry, PROPONO_TYPE_BOOKMARKS_ENTRY, ProponoBookmarksEntryPrivate);

  entry->priv->node = PROPONO_BOOKMARKS_ENTRY_NODE_INVALID;
  entry->priv->conn = NULL;
  entry->priv->name = NULL;
  entry->priv->children = NULL;
  entry->priv->parent = NULL;
}

static void
propono_bookmarks_entry_finalize (GObject *object)
{
  ProponoBookmarksEntry *entry = PROPONO_BOOKMARKS_ENTRY (object);

  if (entry->priv->node == PROPONO_BOOKMARKS_ENTRY_NODE_FOLDER)
    g_free (entry->priv->name);

  G_OBJECT_CLASS (propono_bookmarks_entry_parent_class)->finalize (object);
}

static void
propono_bookmarks_entry_dispose (GObject *object)
{
  ProponoBookmarksEntry *entry = PROPONO_BOOKMARKS_ENTRY (object);

  switch (entry->priv->node)
    {
      case PROPONO_BOOKMARKS_ENTRY_NODE_CONN:
	if (entry->priv->conn)
	  {
	    g_object_unref (entry->priv->conn);
	    entry->priv->conn = NULL;
	  }
	break;

      case PROPONO_BOOKMARKS_ENTRY_NODE_FOLDER:
	if (entry->priv->children)
	  {
	    g_slist_foreach (entry->priv->children, (GFunc) g_object_unref, NULL);
	    g_slist_free (entry->priv->children);
	    entry->priv->children = NULL;
	  }
	break;

      default:
	g_assert_not_reached ();
    }

  G_OBJECT_CLASS (propono_bookmarks_entry_parent_class)->dispose (object);
}

static void
propono_bookmarks_entry_class_init (ProponoBookmarksEntryClass *klass)
{
  GObjectClass* object_class = G_OBJECT_CLASS (klass);

  g_type_class_add_private (klass, sizeof (ProponoBookmarksEntryPrivate));

  object_class->finalize = propono_bookmarks_entry_finalize;
  object_class->dispose  = propono_bookmarks_entry_dispose;
}

ProponoBookmarksEntry *
propono_bookmarks_entry_new_folder (const gchar *name)
{
  ProponoBookmarksEntry *entry;

  g_return_val_if_fail (name != NULL, NULL);

  entry = PROPONO_BOOKMARKS_ENTRY (g_object_new (PROPONO_TYPE_BOOKMARKS_ENTRY, NULL));
  propono_bookmarks_entry_set_node (entry, PROPONO_BOOKMARKS_ENTRY_NODE_FOLDER);
  propono_bookmarks_entry_set_name (entry, name);

  return entry;
}

ProponoBookmarksEntry *
propono_bookmarks_entry_new_conn (ProponoConnection *conn)
{
  ProponoBookmarksEntry *entry;

  g_return_val_if_fail (PROPONO_IS_CONNECTION (conn), NULL);

  entry = PROPONO_BOOKMARKS_ENTRY (g_object_new (PROPONO_TYPE_BOOKMARKS_ENTRY, NULL));
  propono_bookmarks_entry_set_node (entry, PROPONO_BOOKMARKS_ENTRY_NODE_CONN);
  propono_bookmarks_entry_set_conn (entry, conn);

  return entry;

}

void
propono_bookmarks_entry_set_node (ProponoBookmarksEntry *entry, ProponoBookmarksEntryNode node)
{
  g_return_if_fail (PROPONO_IS_BOOKMARKS_ENTRY (entry));

  if (entry->priv->node == node)
    return;

  entry->priv->node = node;

  switch (entry->priv->node)
    {
      case PROPONO_BOOKMARKS_ENTRY_NODE_CONN:
	g_free (entry->priv->name);
	entry->priv->name = NULL;
	g_slist_foreach (entry->priv->children, (GFunc) g_object_unref, NULL);
	g_slist_free (entry->priv->children);
	entry->priv->children = NULL;
	break;

      case PROPONO_BOOKMARKS_ENTRY_NODE_FOLDER:
	if (entry->priv->conn)
	  {
	    g_object_unref (entry->priv->conn);
	    entry->priv->conn = NULL;
	  }
	break;

      default:
	g_assert_not_reached ();
    }
}

ProponoBookmarksEntryNode
propono_bookmarks_entry_get_node (ProponoBookmarksEntry *entry)
{
  g_return_val_if_fail (PROPONO_IS_BOOKMARKS_ENTRY (entry), PROPONO_BOOKMARKS_ENTRY_NODE_INVALID);

  return entry->priv->node;
}

void
propono_bookmarks_entry_set_conn (ProponoBookmarksEntry *entry, ProponoConnection *conn)
{
  g_return_if_fail (PROPONO_IS_BOOKMARKS_ENTRY (entry));
  g_return_if_fail (PROPONO_IS_CONNECTION (conn));
  g_return_if_fail (entry->priv->node == PROPONO_BOOKMARKS_ENTRY_NODE_CONN);

  if (entry->priv->conn != NULL)
    g_object_unref (entry->priv->conn);

  entry->priv->conn = g_object_ref (conn);
}

ProponoConnection *
propono_bookmarks_entry_get_conn (ProponoBookmarksEntry *entry)
{
  g_return_val_if_fail (PROPONO_IS_BOOKMARKS_ENTRY (entry), NULL);

  return entry->priv->conn;
}

void
propono_bookmarks_entry_set_name (ProponoBookmarksEntry *entry, const gchar *name)
{
  g_return_if_fail (PROPONO_IS_BOOKMARKS_ENTRY (entry));
  g_return_if_fail (entry->priv->node == PROPONO_BOOKMARKS_ENTRY_NODE_FOLDER);
  g_return_if_fail (name != NULL);

  g_free (entry->priv->name);
  entry->priv->name = g_strdup (name);
}

const gchar *
propono_bookmarks_entry_get_name (ProponoBookmarksEntry *entry)
{
  g_return_val_if_fail (PROPONO_IS_BOOKMARKS_ENTRY (entry), NULL);

  return entry->priv->name;
}

void
propono_bookmarks_entry_add_child (ProponoBookmarksEntry *entry, ProponoBookmarksEntry *child)
{
  g_return_if_fail (PROPONO_IS_BOOKMARKS_ENTRY (entry));
  g_return_if_fail (entry->priv->node == PROPONO_BOOKMARKS_ENTRY_NODE_FOLDER);
  g_return_if_fail (PROPONO_IS_BOOKMARKS_ENTRY (child));

  entry->priv->children = g_slist_insert_sorted (entry->priv->children,
						 child,
						 (GCompareFunc)propono_bookmarks_entry_compare);
  child->priv->parent = entry;
}

gboolean
propono_bookmarks_entry_remove_child (ProponoBookmarksEntry *entry,
				      ProponoBookmarksEntry *child)
{
  GSList *l;

  g_return_val_if_fail (PROPONO_IS_BOOKMARKS_ENTRY (entry), FALSE);
  g_return_val_if_fail (PROPONO_IS_BOOKMARKS_ENTRY (child), FALSE);

  if (g_slist_index (entry->priv->children, child) > -1)
    {
      entry->priv->children = g_slist_remove (entry->priv->children, child);
      return TRUE;
    }

  for (l = entry->priv->children; l; l = l->next)
    {
      ProponoBookmarksEntry *e = (ProponoBookmarksEntry *) l->data;

      if (propono_bookmarks_entry_get_node (e) != PROPONO_BOOKMARKS_ENTRY_NODE_FOLDER)
	continue;

      if (propono_bookmarks_entry_remove_child (e, child))
	return TRUE;
    }

  return FALSE;
}

GSList *
propono_bookmarks_entry_get_children (ProponoBookmarksEntry *entry)
{
  g_return_val_if_fail (PROPONO_IS_BOOKMARKS_ENTRY (entry), NULL);

  return entry->priv->children;
}

ProponoBookmarksEntry *
propono_bookmarks_entry_get_parent (ProponoBookmarksEntry *entry)
{
  g_return_val_if_fail (PROPONO_IS_BOOKMARKS_ENTRY (entry), NULL);

  return entry->priv->parent;
}

gint
propono_bookmarks_entry_compare (ProponoBookmarksEntry *a, ProponoBookmarksEntry *b)
{
  gchar *name_a, *name_b;
  gint   result;

  if (a->priv->node != b->priv->node)
    return a->priv->node - b->priv->node;

  if (a->priv->node == PROPONO_BOOKMARKS_ENTRY_NODE_FOLDER)
    return g_ascii_strcasecmp (a->priv->name, b->priv->name);

  name_a = propono_connection_get_best_name (a->priv->conn);
  name_b = propono_connection_get_best_name (b->priv->conn);
  result = g_ascii_strcasecmp (name_a, name_b);
  g_free (name_a);
  g_free (name_b);

  return result;
}

/* vim: set ts=8: */
