/*
 * propono-bookmarks.c
 * This file is part of propono
 *
 * Copyright (C) 2007,2008,2009  Jonh Wendell <wendell@bani.com.br>
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

#include <gtk/gtk.h>
#include <glib/gi18n.h>
#include <gio/gio.h>
#include <libxml/parser.h>
#include <libxml/xmlwriter.h>

#include "propono-bookmarks.h"
#include "propono-bookmarks-entry.h"
#include "propono-bookmarks-migration.h"
#include "propono-connection.h"
#include "propono-plugin.h"
#include "propono-plugins-engine.h"

struct _ProponoBookmarksPrivate
{
  gchar        *filename;
  GSList       *entries;
  GFileMonitor *monitor;
};

enum
{
  BOOKMARK_CHANGED,
  LAST_SIGNAL
};

G_DEFINE_TYPE (ProponoBookmarks, propono_bookmarks, G_TYPE_OBJECT);

static ProponoBookmarks *book_singleton = NULL;
static guint signals[LAST_SIGNAL] = { 0 };

/* Prototypes */
static void propono_bookmarks_update_from_file (ProponoBookmarks *book);
static void propono_bookmarks_file_changed     (GFileMonitor     *monitor,
					        GFile             *file,
					        GFile             *other_file,
					        GFileMonitorEvent  event_type,
					        ProponoBookmarks  *book);

static void
propono_bookmarks_init (ProponoBookmarks *book)
{
  GFile *gfile;

  book->priv = G_TYPE_INSTANCE_GET_PRIVATE (book, PROPONO_TYPE_BOOKMARKS, ProponoBookmarksPrivate);
  book->priv->entries = NULL;
  book->priv->filename = g_build_filename (g_get_user_data_dir (),
			                   "propono",
			                   PROPONO_BOOKMARKS_FILE,
			                   NULL);

  if (!g_file_test (book->priv->filename, G_FILE_TEST_EXISTS))
    propono_bookmarks_migration_migrate (book->priv->filename);

  propono_bookmarks_update_from_file (book);

  gfile = g_file_new_for_path (book->priv->filename);
  book->priv->monitor = g_file_monitor_file (gfile,
                                             G_FILE_MONITOR_NONE,
                                             NULL,
                                             NULL);
  g_object_unref (gfile);

  g_signal_connect (book->priv->monitor,
                    "changed",
                    G_CALLBACK (propono_bookmarks_file_changed),
                    book);
}

static void
propono_bookmarks_clear_entries (ProponoBookmarks *book)
{
  g_slist_foreach (book->priv->entries, (GFunc) g_object_unref, NULL);
  g_slist_free (book->priv->entries);

  book->priv->entries = NULL;
}

static void
propono_bookmarks_finalize (GObject *object)
{
  ProponoBookmarks *book = PROPONO_BOOKMARKS (object);

  g_free (book->priv->filename);
  book->priv->filename = NULL;

  G_OBJECT_CLASS (propono_bookmarks_parent_class)->finalize (object);
}

static void
propono_bookmarks_dispose (GObject *object)
{
  ProponoBookmarks *book = PROPONO_BOOKMARKS (object);

  if (book->priv->entries)
    propono_bookmarks_clear_entries (book);

  if (book->priv->monitor)
    {
      g_file_monitor_cancel (book->priv->monitor);
      g_object_unref (book->priv->monitor);
      book->priv->monitor = NULL;
    }

  G_OBJECT_CLASS (propono_bookmarks_parent_class)->dispose (object);
}

static void
propono_bookmarks_class_init (ProponoBookmarksClass *klass)
{
  GObjectClass* object_class = G_OBJECT_CLASS (klass);

  g_type_class_add_private (klass, sizeof (ProponoBookmarksPrivate));

  object_class->finalize = propono_bookmarks_finalize;
  object_class->dispose  = propono_bookmarks_dispose;

  signals[BOOKMARK_CHANGED] =
		g_signal_new ("changed",
			      G_OBJECT_CLASS_TYPE (object_class),
			      G_SIGNAL_RUN_FIRST,
			      G_STRUCT_OFFSET (ProponoBookmarksClass, changed),
			      NULL, NULL,
			      g_cclosure_marshal_VOID__VOID,
			      G_TYPE_NONE,
			      0);
}

static ProponoConnection *
find_conn_by_host (GSList      *entries,
		   const gchar *protocol,
		   const gchar *host,
		   gint         port)
{
  GSList *l;
  ProponoConnection *conn;

  for (l = entries; l; l = l->next)
    {
      ProponoBookmarksEntry *entry = PROPONO_BOOKMARKS_ENTRY (l->data);

      switch (propono_bookmarks_entry_get_node (entry))
	{
	  case PROPONO_BOOKMARKS_ENTRY_NODE_FOLDER:
	    if ((conn = find_conn_by_host (propono_bookmarks_entry_get_children (entry),
					   protocol,
					   host,
					   port)))
	      return conn;
	    break;

	  case PROPONO_BOOKMARKS_ENTRY_NODE_CONN:
	    conn = propono_bookmarks_entry_get_conn (entry);
	    if ( (g_str_equal (host, propono_connection_get_host (conn))) &&
		 (port == propono_connection_get_port (conn)) &&
		 (g_str_equal (protocol, propono_connection_get_protocol (conn))) )
	      return g_object_ref (conn);
	    break;

	  default:
	    g_assert_not_reached ();
	}
    }
  return NULL;
}

ProponoConnection *
propono_bookmarks_exists (ProponoBookmarks *book,
                          const gchar      *protocol,
                          const gchar      *host,
                          gint              port)
{
  ProponoConnection *conn = NULL;
  GSList *l, *next;

  g_return_val_if_fail (PROPONO_IS_BOOKMARKS (book), NULL);
  g_return_val_if_fail (host != NULL, NULL);

  return find_conn_by_host (book->priv->entries, protocol, host, port);
}

static void
propono_bookmarks_save_fill_xml (GSList *list, xmlTextWriter *writer)
{
  GSList                *l;
  ProponoBookmarksEntry *entry;
  ProponoConnection     *conn;

  for (l = list; l; l = l->next)
    {
      entry = (ProponoBookmarksEntry *) l->data;
      switch (propono_bookmarks_entry_get_node (entry))
	{
	  case PROPONO_BOOKMARKS_ENTRY_NODE_FOLDER:
	    xmlTextWriterStartElement (writer, "folder");
	    xmlTextWriterWriteAttribute (writer, "name", propono_bookmarks_entry_get_name (entry));

	    propono_bookmarks_save_fill_xml (propono_bookmarks_entry_get_children (entry), writer);
	    xmlTextWriterEndElement (writer);
	    break;

	  case PROPONO_BOOKMARKS_ENTRY_NODE_CONN:
	    conn = propono_bookmarks_entry_get_conn (entry);

	    xmlTextWriterStartElement (writer, "item");
	    propono_connection_fill_writer (conn, writer);
	    xmlTextWriterEndElement (writer);
	    break;

	  default:
	    g_assert_not_reached ();
	}
    }
}

static ProponoBookmarksEntry *
propono_bookmarks_parse_item (xmlNode *root)
{
  ProponoBookmarksEntry *entry = NULL;
  ProponoConnection     *conn;
  xmlNode               *curr;
  xmlChar               *s_value;
  gchar                 *protocol = NULL;
  ProponoPlugin         *plugin;

  /* Loop to discover the protocol */
  for (curr = root->children; curr; curr = curr->next)
    {
      if (xmlStrcmp(curr->name, (const xmlChar *)"protocol"))
        continue;

      s_value = xmlNodeGetContent (curr);
      protocol = g_strdup ((const gchar *)s_value);
      xmlFree (s_value);
      break;
    }

  if (!protocol)
    protocol = g_strdup ("vnc");

  plugin = g_hash_table_lookup (propono_plugin_engine_get_plugins_by_protocol (propono_plugins_engine_get_default ()),
				protocol);
  if (!plugin)
    {
      g_warning (_("The protocol %s is not supported."), protocol);
      goto out;
    }

  conn = propono_plugin_new_connection (plugin);
  propono_connection_parse_item (conn, root);

  if (propono_connection_get_host (conn))
    entry = propono_bookmarks_entry_new_conn (conn);

  g_object_unref (conn);

out:
  g_free (protocol);
  return entry;
}

static void
propono_bookmarks_parse_xml (ProponoBookmarks *book, xmlNode *root, ProponoBookmarksEntry *parent_entry)
{
  xmlNode *curr;
  xmlChar *folder_name;
  ProponoBookmarksEntry *entry;

  for (curr = root; curr; curr = curr->next)
    {
      if (curr->type == XML_ELEMENT_NODE)
	{
	  if (!xmlStrcmp(curr->name, (const xmlChar *)"folder"))
	    {
	      folder_name = xmlGetProp (curr, (const xmlChar *)"name");
	      if (folder_name && *folder_name)
		{
		  entry = propono_bookmarks_entry_new_folder ((const gchar *) folder_name);
		  if (parent_entry)
		    propono_bookmarks_entry_add_child (parent_entry, entry);
		  else
		    book->priv->entries = g_slist_insert_sorted (book->priv->entries,
							         entry,
							        (GCompareFunc)propono_bookmarks_entry_compare);

		  propono_bookmarks_parse_xml (book, curr->children, entry);
		}
	      xmlFree (folder_name);
	    }
	  else if (!xmlStrcmp(curr->name, (const xmlChar *)"item"))
	    {
	      entry = propono_bookmarks_parse_item (curr);
	      if (entry)
		{
		  if (parent_entry)
		    propono_bookmarks_entry_add_child (parent_entry, entry);
		  else
		    book->priv->entries = g_slist_insert_sorted (book->priv->entries,
								 entry,
								 (GCompareFunc)propono_bookmarks_entry_compare);
		}
	    }
	}
    }

}

static void
propono_bookmarks_update_from_file (ProponoBookmarks *book)
{
  xmlErrorPtr error;
  xmlNodePtr  root;
  xmlDocPtr   doc;

  if (!g_file_test (book->priv->filename, G_FILE_TEST_EXISTS))
    return;

  doc = xmlReadFile (book->priv->filename, NULL, XML_PARSE_NOERROR);

  if (!doc)
    {
      error = xmlGetLastError ();
      g_warning (_("Error while initializing bookmarks: %s"), error?error->message: _("Unknown error"));
      return;
    }

  root = xmlDocGetRootElement (doc);
  if (!root)
    {
      g_warning (_("Error while initializing bookmarks: The file seems to be empty"));
      xmlFreeDoc (doc);
      return;
    }

  if (xmlStrcmp (root->name, (const xmlChar *) "propono-bookmarks"))
    {
      g_warning (_("Error while initializing bookmarks: The file is not a propono bookmarks file"));
      xmlFreeDoc (doc);
      return;
    }

  propono_bookmarks_clear_entries (book);
  propono_bookmarks_parse_xml (book, root->xmlChildrenNode, NULL);
  xmlFreeDoc (doc);
}


static void
propono_bookmarks_file_changed (GFileMonitor      *monitor,
		                GFile             *file,
		                GFile             *other_file,
		                GFileMonitorEvent  event_type,
		                ProponoBookmarks  *book)
{
  if (event_type != G_FILE_MONITOR_EVENT_CHANGED &&
      event_type != G_FILE_MONITOR_EVENT_CREATED &&
      event_type != G_FILE_MONITOR_EVENT_DELETED)
    return;

  propono_bookmarks_update_from_file (book);

  g_signal_emit (book, signals[BOOKMARK_CHANGED], 0);
}

/* Public API */

ProponoBookmarks *
propono_bookmarks_get_default (void)
{
  if (G_UNLIKELY (!book_singleton))
    book_singleton = PROPONO_BOOKMARKS (g_object_new (PROPONO_TYPE_BOOKMARKS,
                                                      NULL));
  return book_singleton;
}

GSList *
propono_bookmarks_get_all (ProponoBookmarks *book)
{
  g_return_val_if_fail (PROPONO_IS_BOOKMARKS (book), NULL);

  return book->priv->entries;
}

void
propono_bookmarks_save_to_file (ProponoBookmarks *book)
{
  xmlTextWriter *writer;
  xmlBuffer     *buf;
  int            rc;
  GError        *error;

  writer = NULL;
  buf    = NULL;
  error  = NULL;

  buf = xmlBufferCreate ();
  if (!buf)
    {
      g_warning (_("Error while saving bookmarks: Failed to create the XML structure"));
      return;
    }

  writer = xmlNewTextWriterMemory(buf, 0);
  if (!writer)
    {
      g_warning (_("Error while saving bookmarks: Failed to create the XML structure"));
      goto finalize;
    }

  rc = xmlTextWriterStartDocument (writer, NULL, "utf-8", NULL);
  if (rc < 0)
    {
      g_warning (_("Error while saving bookmarks: Failed to initialize the XML structure"));
      goto finalize;
    }

  rc = xmlTextWriterStartElement (writer, "propono-bookmarks");
  if (rc < 0)
    {
      g_warning (_("Error while saving bookmarks: Failed to initialize the XML structure"));
      goto finalize;
    }

  propono_bookmarks_save_fill_xml (book->priv->entries, writer);

  rc = xmlTextWriterEndDocument (writer);
  if (rc < 0)
    {
      g_warning (_("Error while saving bookmarks: Failed to finalize the XML structure"));
      goto finalize;
    }

  if (!g_file_set_contents (book->priv->filename,
			    (const char *) buf->content,
			    -1,
			    &error))
    {
      g_warning (_("Error while saving bookmarks: %s"), error?error->message:_("Unknown error"));
      if (error)
	g_error_free (error);
      goto finalize;
    }

finalize:
  if (writer)
    xmlFreeTextWriter (writer);
  if (buf)
    xmlBufferFree (buf);
}

void
propono_bookmarks_add_entry (ProponoBookmarks      *book,
                             ProponoBookmarksEntry *entry,
                             ProponoBookmarksEntry *parent)
{
  /* I do not ref entry */
  if (parent)
    propono_bookmarks_entry_add_child (parent, entry);
  else
    book->priv->entries = g_slist_insert_sorted (book->priv->entries,
						 entry,
						 (GCompareFunc)propono_bookmarks_entry_compare);
  propono_bookmarks_save_to_file (book);
}

gboolean
propono_bookmarks_remove_entry (ProponoBookmarks      *book,
				ProponoBookmarksEntry *entry)
{
  GSList *l;

  g_return_val_if_fail (PROPONO_IS_BOOKMARKS (book), FALSE);

  /* I do unref entry */
  if (g_slist_index (book->priv->entries, entry) > -1)
    {
      book->priv->entries = g_slist_remove (book->priv->entries, entry);
      g_object_unref (entry);
      propono_bookmarks_save_to_file (book);
      return TRUE;
    }

  for (l = book->priv->entries; l; l = l->next)
    {
      ProponoBookmarksEntry *e = (ProponoBookmarksEntry *) l->data;

      if (propono_bookmarks_entry_get_node (e) != PROPONO_BOOKMARKS_ENTRY_NODE_FOLDER)
	continue;

      if (propono_bookmarks_entry_remove_child (e, entry))
	{
	  g_object_unref (entry);
	  propono_bookmarks_save_to_file (book);
	  return TRUE;
	}
    }

  return FALSE;
}


ProponoBookmarksEntry *
propono_bookmarks_name_exists (ProponoBookmarks      *book,
                               ProponoBookmarksEntry *parent,
                               const gchar           *name)
{
  GSList *entries, *l;

  g_return_val_if_fail (PROPONO_IS_BOOKMARKS (book), FALSE);

  if (parent)
    entries = propono_bookmarks_entry_get_children (parent);
  else
    entries = book->priv->entries;

  for (l = entries; l; l = l->next)
    {
      ProponoBookmarksEntry *e = (ProponoBookmarksEntry *) l->data;

      if (propono_bookmarks_entry_get_node (e) == PROPONO_BOOKMARKS_ENTRY_NODE_FOLDER)
	{
	  if (g_strcmp0 (propono_bookmarks_entry_get_name (e), name) == 0)
	    return e;
	}
      else
	{
	  ProponoConnection *conn = propono_bookmarks_entry_get_conn (e);
	  if (g_strcmp0 (propono_connection_get_name (conn), name) == 0)
	    return e;
	}
    }

  return NULL;
}

/* vim: set ts=8: */
