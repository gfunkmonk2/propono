/*
 * propono-window.c
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
 
#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <errno.h>
#include <time.h>
#include <sys/types.h>
#include <string.h>

#include <glib/gi18n.h>
#include <gdk/gdkkeysyms.h>

#include "propono-window.h"
#include "propono-notebook.h"
#include "propono-fav.h"
#include "propono-prefs.h"
#include "propono-cache-prefs.h"
#include "propono-utils.h"
#include "propono-bookmarks.h"
#include "propono-ui.h"
#include "propono-window-private.h"
#include "propono-bookmarks-entry.h"
#include "propono-plugin.h"
#include "propono-plugins-engine.h"

#ifdef PROPONO_ENABLE_AVAHI
#include "propono-mdns.h"
#endif

#define PROPONO_WINDOW_GET_PRIVATE(object)(G_TYPE_INSTANCE_GET_PRIVATE ((object),\
					 PROPONO_TYPE_WINDOW,                    \
					 ProponoWindowPrivate))

G_DEFINE_TYPE(ProponoWindow, propono_window, GTK_TYPE_WINDOW)

static void
propono_window_dispose (GObject *object)
{
  ProponoWindow *window = PROPONO_WINDOW (object);

  if (!window->priv->dispose_has_run)
    {
      propono_plugins_engine_deactivate_plugins (propono_plugins_engine_get_default (),
						 window);
      window->priv->dispose_has_run = TRUE;
    }

  if (window->priv->manager)
    {
      g_object_unref (window->priv->manager);
      window->priv->manager = NULL;
    }

  if (window->priv->update_recents_menu_ui_id != 0)
    {
      GtkRecentManager *recent_manager = gtk_recent_manager_get_default ();

      g_signal_handler_disconnect (recent_manager,
				   window->priv->update_recents_menu_ui_id);
      window->priv->update_recents_menu_ui_id = 0;
    }

  G_OBJECT_CLASS (propono_window_parent_class)->dispose (object);
}

static gboolean
propono_window_delete_event (GtkWidget   *widget,
			     GdkEventAny *event)
{
  ProponoWindow *window = PROPONO_WINDOW (widget);

  propono_window_close_all_tabs (window);

  if (GTK_WIDGET_CLASS (propono_window_parent_class)->delete_event)
    return GTK_WIDGET_CLASS (propono_window_parent_class)->delete_event (widget, event);
  else
    return FALSE;
}

static void
propono_window_show_hide_controls (ProponoWindow *window)
{
  if (window->priv->fullscreen)
    {
      window->priv->fav_panel_visible = GTK_WIDGET_VISIBLE (window->priv->fav_panel);
      gtk_widget_hide (window->priv->fav_panel);

      window->priv->toolbar_visible = GTK_WIDGET_VISIBLE (window->priv->toolbar);
      gtk_widget_hide (window->priv->toolbar);

      gtk_widget_hide (window->priv->menubar);

      window->priv->statusbar_visible = GTK_WIDGET_VISIBLE (window->priv->statusbar);
      gtk_widget_hide (window->priv->statusbar);

      gtk_notebook_set_show_tabs (GTK_NOTEBOOK (window->priv->notebook), FALSE);
      gtk_notebook_set_show_border (GTK_NOTEBOOK (window->priv->notebook), FALSE);
    }
  else
    {
      if (window->priv->fav_panel_visible)
        gtk_widget_show_all (window->priv->fav_panel);

      if (window->priv->toolbar_visible)
        gtk_widget_show_all (window->priv->toolbar);

      gtk_widget_show_all (window->priv->menubar);

      if (window->priv->statusbar_visible)
        gtk_widget_show_all (window->priv->statusbar);

      propono_notebook_show_hide_tabs (PROPONO_NOTEBOOK (window->priv->notebook));
      gtk_notebook_set_show_border (GTK_NOTEBOOK (window->priv->notebook), TRUE);
    }
}

static gboolean
propono_window_state_event_cb (GtkWidget *widget,
			       GdkEventWindowState *event)
{
  ProponoWindow *window = PROPONO_WINDOW (widget);

  window->priv->window_state = event->new_window_state;
  propono_cache_prefs_set_integer ("window", "window-state", window->priv->window_state);

  if ((event->changed_mask & GDK_WINDOW_STATE_FULLSCREEN) == 0)
    return FALSE;

  if (event->new_window_state & GDK_WINDOW_STATE_FULLSCREEN)
    window->priv->fullscreen = TRUE;
  else
    window->priv->fullscreen = FALSE;

  propono_window_show_hide_controls (window);

  if (GTK_WIDGET_CLASS (propono_window_parent_class)->window_state_event)
    return GTK_WIDGET_CLASS (propono_window_parent_class)->window_state_event (widget, event);
  else
    return FALSE;
}

static gboolean 
propono_window_configure_event (GtkWidget         *widget,
			        GdkEventConfigure *event)
{
  ProponoWindow *window = PROPONO_WINDOW (widget);

  if ((propono_cache_prefs_get_integer ("window", "window-state", 0) & GDK_WINDOW_STATE_MAXIMIZED) == 0)
    {
      window->priv->width  = event->width;
      window->priv->height = event->height;

      propono_cache_prefs_set_integer ("window", "window-width", window->priv->width);
      propono_cache_prefs_set_integer ("window", "window-height", window->priv->height);
    }

  return GTK_WIDGET_CLASS (propono_window_parent_class)->configure_event (widget, event);
}

static void
propono_window_class_init (ProponoWindowClass *klass)
{
  GObjectClass *object_class    = G_OBJECT_CLASS (klass);
  GtkWidgetClass *widget_class  = GTK_WIDGET_CLASS (klass);

  object_class->dispose  = propono_window_dispose;

  widget_class->window_state_event = propono_window_state_event_cb;
  widget_class->configure_event    = propono_window_configure_event;
  widget_class->delete_event       = propono_window_delete_event;

  g_type_class_add_private (object_class, sizeof(ProponoWindowPrivate));
}

static void
menu_item_select_cb (GtkItem       *proxy,
		     ProponoWindow *window)
{
  GtkAction *action;
  char *message;

  action = g_object_get_data (G_OBJECT (proxy),  "gtk-action");
  g_return_if_fail (action != NULL);

  g_object_get (G_OBJECT (action), "tooltip", &message, NULL);
  if (message)
    {
      gtk_statusbar_push (GTK_STATUSBAR (window->priv->statusbar),
			  window->priv->tip_message_cid, message);
      g_free (message);
    }
}

static void
menu_item_deselect_cb (GtkItem       *proxy,
                       ProponoWindow *window)
{
  gtk_statusbar_pop (GTK_STATUSBAR (window->priv->statusbar),
		     window->priv->tip_message_cid);
}

static void
connect_proxy_cb (GtkUIManager  *manager,
                  GtkAction     *action,
                  GtkWidget     *proxy,
                  ProponoWindow *window)
{
  if (GTK_IS_MENU_ITEM (proxy))
    {
       g_signal_connect (proxy, "select",
			 G_CALLBACK (menu_item_select_cb), window);
       g_signal_connect (proxy, "deselect",
			 G_CALLBACK (menu_item_deselect_cb), window);
    }
}

static void
disconnect_proxy_cb (GtkUIManager  *manager,
                     GtkAction     *action,
                     GtkWidget     *proxy,
                     ProponoWindow *window)
{
  if (GTK_IS_MENU_ITEM (proxy))
    {
      g_signal_handlers_disconnect_by_func
		(proxy, G_CALLBACK (menu_item_select_cb), window);
      g_signal_handlers_disconnect_by_func
		(proxy, G_CALLBACK (menu_item_deselect_cb), window);
    }
}

static void
activate_recent_cb (GtkRecentChooser *action, ProponoWindow *window)
{
  ProponoConnection *conn;
  gchar             *error, *uri;

  uri = gtk_recent_chooser_get_current_uri (action);
  conn = propono_connection_new_from_string (uri,
					     &error,
					     TRUE);
  if (conn)
    {
      propono_cmd_open_bookmark (window, conn);
      g_object_unref (conn);
    }
  else
    propono_utils_show_error (NULL, error ? error : _("Unknown error"), GTK_WINDOW (window));

  g_free (error);
  g_free (uri);
}

static void
update_recent_connections (ProponoWindow *window)
{
  ProponoWindowPrivate *p = window->priv;

  g_return_if_fail (p->recent_action_group != NULL);

  if (p->recents_menu_ui_id != 0)
    gtk_ui_manager_remove_ui (p->manager, p->recents_menu_ui_id);

  p->recents_menu_ui_id = gtk_ui_manager_new_merge_id (p->manager);

  gtk_ui_manager_add_ui (p->manager,
			 p->recents_menu_ui_id,
			 "/MenuBar/MachineMenu/FileRecentsPlaceholder",
			 "recent_connections",
			 "recent_connections",
			 GTK_UI_MANAGER_MENUITEM,
			 FALSE);
}

static void
recent_manager_changed (GtkRecentManager *manager,
			ProponoWindow     *window)
{
  update_recent_connections (window);
}

static void
show_hide_accels (ProponoWindow *window)
{
  gboolean show_accels;

  g_object_get (propono_prefs_get_default (),
		"show-accels", &show_accels,
		NULL);
  g_object_set (gtk_settings_get_default (),
		"gtk-enable-accels", show_accels,
		"gtk-enable-mnemonics", show_accels,
		NULL);
}

static void
create_menu_bar_and_toolbar (ProponoWindow *window, 
			     GtkWidget     *main_box)
{
  GtkActionGroup   *action_group;
  GtkUIManager     *manager;
  GError           *error = NULL;
  GtkRecentManager *recent_manager;
  GtkRecentFilter  *filter;
  GtkAction        *action;

  manager = gtk_ui_manager_new ();
  window->priv->manager = manager;

  /* show tooltips in the statusbar */
  g_signal_connect (manager,
		    "connect-proxy",
		    G_CALLBACK (connect_proxy_cb),
		    window);
  g_signal_connect (manager,
		   "disconnect-proxy",
		    G_CALLBACK (disconnect_proxy_cb),
		    window);

  gtk_window_add_accel_group (GTK_WINDOW (window),
			      gtk_ui_manager_get_accel_group (manager));

  /* Normal actions */
  action_group = gtk_action_group_new ("ProponoWindowAlwaysSensitiveActions");
  gtk_action_group_set_translation_domain (action_group, NULL);
  gtk_action_group_add_actions (action_group,
				propono_always_sensitive_entries,
				G_N_ELEMENTS (propono_always_sensitive_entries),
				window);
  
  gtk_action_group_add_toggle_actions (action_group,
				       propono_always_sensitive_toggle_entries,
				       G_N_ELEMENTS (propono_always_sensitive_toggle_entries),
				       window);

  gtk_ui_manager_insert_action_group (manager, action_group, 0);
  g_object_unref (action_group);
  window->priv->always_sensitive_action_group = action_group;

  action = gtk_action_group_get_action (action_group, "MachineConnect");
  g_object_set (action, "is_important", TRUE, NULL);

  /* Machine connected actions */
  action_group = gtk_action_group_new ("ProponoWindowMachineConnectedActions");
  gtk_action_group_set_translation_domain (action_group, NULL);
  gtk_action_group_add_actions (action_group,
				propono_machine_connected_entries,
				G_N_ELEMENTS (propono_machine_connected_entries),
				window);
  gtk_action_group_set_sensitive (action_group, FALSE);
  gtk_ui_manager_insert_action_group (manager, action_group, 0);
  g_object_unref (action_group);
  window->priv->machine_connected_action_group = action_group;

  action = gtk_action_group_get_action (action_group, "MachineClose");
  g_object_set (action, "is_important", TRUE, NULL);

  /* Machine initialized actions */
  action_group = gtk_action_group_new ("ProponoWindowMachineInitializedActions");
  gtk_action_group_set_translation_domain (action_group, NULL);
  gtk_action_group_add_actions (action_group,
				propono_machine_initialized_entries,
				G_N_ELEMENTS (propono_machine_initialized_entries),
				window);
  gtk_action_group_set_sensitive (action_group, FALSE);
  gtk_ui_manager_insert_action_group (manager, action_group, 0);
  g_object_unref (action_group);
  window->priv->machine_initialized_action_group = action_group;

  action = gtk_action_group_get_action (action_group, "ViewFullScreen");
  g_object_set (action, "is_important", TRUE, NULL);
  action = gtk_action_group_get_action (action_group, "MachineTakeScreenshot");
  g_object_set (action, "is_important", TRUE, NULL);

  /* now load the UI definition */
  gtk_ui_manager_add_ui_from_file (manager, propono_utils_get_ui_xml_filename (), &error);
  if (error != NULL)
    {
      g_critical (_("Could not merge UI XML file: %s"), error->message);
      g_error_free (error);
      return;
    }

  /* Bookmarks */
  action_group = gtk_action_group_new ("BookmarksActions");
  gtk_action_group_set_translation_domain (action_group, NULL);
  window->priv->bookmarks_list_action_group = action_group;
  gtk_ui_manager_insert_action_group (manager, action_group, 0);
  g_object_unref (action_group);

  window->priv->menubar = gtk_ui_manager_get_widget (manager, "/MenuBar");
  gtk_box_pack_start (GTK_BOX (main_box), 
		      window->priv->menubar, 
		      FALSE, 
		      FALSE, 
		      0);

  window->priv->toolbar = gtk_ui_manager_get_widget (manager, "/ToolBar");
  gtk_widget_hide (window->priv->toolbar);
  gtk_box_pack_start (GTK_BOX (main_box),
		      window->priv->toolbar,
		      FALSE,
		      FALSE,
		      0);

  /* Recent connections */
  window->priv->recent_action = gtk_recent_action_new ("recent_connections",
						     _("_Recent connections"),
						       NULL, NULL);
  g_object_set (G_OBJECT (window->priv->recent_action),
		"show-not-found", TRUE,
		"local-only", FALSE,
		"show-private", TRUE,
		"show-tips", TRUE,
		"sort-type", GTK_RECENT_SORT_MRU,
		NULL);
  g_signal_connect (window->priv->recent_action,
		    "item-activated",
		    G_CALLBACK (activate_recent_cb),
		    window);

  action_group = gtk_action_group_new ("ProponoRecentConnectionsActions");
  gtk_action_group_set_translation_domain (action_group, NULL);

  gtk_ui_manager_insert_action_group (manager, action_group, 0);
  g_object_unref (action_group);
  window->priv->recent_action_group = action_group;
  gtk_action_group_add_action (window->priv->recent_action_group,
			       window->priv->recent_action);

  filter = gtk_recent_filter_new ();
  gtk_recent_filter_add_group (filter, "propono");
  gtk_recent_chooser_add_filter (GTK_RECENT_CHOOSER (window->priv->recent_action),
				 filter);

  update_recent_connections (window);

  recent_manager = gtk_recent_manager_get_default ();
  window->priv->update_recents_menu_ui_id = g_signal_connect (
		    recent_manager,
		    "changed",
		    G_CALLBACK (recent_manager_changed),
		    window);

  g_signal_connect_swapped (propono_prefs_get_default (),
			    "notify::show-accels",
			     G_CALLBACK (show_hide_accels),
			     window);
  show_hide_accels (window);
}

static void
fav_panel_size_allocate (GtkWidget     *widget,
			 GtkAllocation *allocation,
			 ProponoWindow *window)
{
  window->priv->side_panel_size = allocation->width;
  if (window->priv->side_panel_size > 0)
    propono_cache_prefs_set_integer ("window", "side-panel-size", window->priv->side_panel_size);
}

static void
propono_window_populate_bookmarks (ProponoWindow *window,
				   const gchar   *group,
				   GSList        *entries,
				   const gchar   *parent)
{
  static guint           i = 0;
  GSList                *l;
  ProponoBookmarksEntry *entry;
  gchar                 *action_name, *action_label, *path, *tooltip;
  GtkAction             *action;
  ProponoWindowPrivate  *p = window->priv;
  ProponoConnection     *conn;
  ProponoPlugin         *plugin;

  for (l = entries; l; l = l->next)
    {
      entry = PROPONO_BOOKMARKS_ENTRY (l->data);
      switch (propono_bookmarks_entry_get_node (entry))
	{
	  case PROPONO_BOOKMARKS_ENTRY_NODE_FOLDER:
	    action_label = propono_utils_escape_underscores (propono_bookmarks_entry_get_name (entry), -1);
	    action_name = g_strdup_printf ("BOOKMARK_FOLDER_ACTION_%d", ++i);
	    action = gtk_action_new (action_name,
				     action_label,
				     NULL,
				     NULL);
	    g_object_set (G_OBJECT (action), "icon-name", "folder", "hide-if-empty", FALSE, NULL);
	    gtk_action_group_add_action (p->bookmarks_list_action_group,
					 action);
	    g_object_unref (action);

	    path = g_strdup_printf ("/MenuBar/BookmarksMenu/%s%s", group, parent?parent:"");
	    gtk_ui_manager_add_ui (p->manager,
				   p->bookmarks_list_menu_ui_id,
				   path,
				   action_label, action_name,
				   GTK_UI_MANAGER_MENU,
				   FALSE);
	    g_free (path);
    	    g_free (action_name);

	    path = g_strdup_printf ("%s/%s", parent?parent:"", action_label);
	    g_free (action_label);

	    propono_window_populate_bookmarks (window, group, propono_bookmarks_entry_get_children (entry), path);
	    g_free (path);
	    break;

	  case PROPONO_BOOKMARKS_ENTRY_NODE_CONN:
	    conn = propono_bookmarks_entry_get_conn (entry);
	    plugin = propono_plugins_engine_get_plugin_by_protocol (propono_plugins_engine_get_default (),
								    propono_connection_get_protocol (conn));

	    action_name = propono_connection_get_best_name (conn);
	    action_label = propono_utils_escape_underscores (action_name, -1);
	    g_free (action_name);

	    action_name = g_strdup_printf ("BOOKMARK_ITEM_ACTION_%d", ++i);

	    /* Translators: This is server:port, a statusbar tooltip when mouse is over a bookmark item on menu */
	    tooltip = g_strdup_printf (_("Open %s:%d"),
					propono_connection_get_host (conn),
					propono_connection_get_port (conn));
	    action = gtk_action_new (action_name,
				     action_label,
				     tooltip,
				     NULL);
	    g_object_set (G_OBJECT (action),
			  "icon-name",
			  propono_plugin_get_icon_name (plugin),
			  NULL);
	    g_object_set_data (G_OBJECT (action), "conn", conn);
	    gtk_action_group_add_action (p->bookmarks_list_action_group,
					 action);
	    g_signal_connect (action,
			     "activate",
			     G_CALLBACK (propono_fav_bookmarks_open),
			     window->priv->fav_panel);

	    path = g_strdup_printf ("/MenuBar/BookmarksMenu/%s%s", group, parent?parent:"");
	    gtk_ui_manager_add_ui (p->manager,
				   p->bookmarks_list_menu_ui_id,
				   path,
				   action_label, action_name,
				   GTK_UI_MANAGER_MENUITEM,
				   FALSE);

	    g_object_unref (action);
	    g_free (action_name);
	    g_free (action_label);
	    g_free (tooltip);
	    g_free (path);
	    break;

	  default:
	    g_assert_not_reached ();
	}
    }
}

void
propono_window_update_bookmarks_list_menu (ProponoWindow *window)
{
  ProponoWindowPrivate *p = window->priv;
  GList  *actions, *l;
  GSList *favs, *mdnss = NULL;

  g_return_if_fail (p->bookmarks_list_action_group != NULL);

  if (p->bookmarks_list_menu_ui_id != 0)
    gtk_ui_manager_remove_ui (p->manager, p->bookmarks_list_menu_ui_id);

  actions = gtk_action_group_list_actions (p->bookmarks_list_action_group);
  for (l = actions; l != NULL; l = l->next)
    {
      g_signal_handlers_disconnect_by_func (GTK_ACTION (l->data),
					    G_CALLBACK (propono_fav_bookmarks_open),
					    window->priv->fav_panel);
      gtk_action_group_remove_action (p->bookmarks_list_action_group,
				      GTK_ACTION (l->data));
    }
  g_list_free (actions);

  favs = propono_bookmarks_get_all (propono_bookmarks_get_default ());

#ifdef PROPONO_ENABLE_AVAHI
  mdnss = propono_mdns_get_all (propono_mdns_get_default ());
#endif

  p->bookmarks_list_menu_ui_id = (g_slist_length (favs) > 0||g_slist_length (mdnss) > 0) ? gtk_ui_manager_new_merge_id (p->manager) : 0;
  propono_window_populate_bookmarks (window, "BookmarksList", favs, NULL);
  propono_window_populate_bookmarks (window, "AvahiList", mdnss, NULL);
}

static void
create_side_panel (ProponoWindow *window)
{
  window->priv->fav_panel = propono_fav_new (window);

  gtk_paned_pack1 (GTK_PANED (window->priv->hpaned), 
		   window->priv->fav_panel, 
		   FALSE, 
		   FALSE);

  window->priv->side_panel_size = propono_cache_prefs_get_integer ("window", "side-panel-size", 200);
  gtk_paned_set_position (GTK_PANED (window->priv->hpaned), window->priv->side_panel_size);

  g_signal_connect (window->priv->fav_panel,
		    "size-allocate",
		    G_CALLBACK (fav_panel_size_allocate),
		    window);
}

static void
init_widgets_visibility (ProponoWindow *window)
{
  GtkAction *action;
  gboolean visible;

  /* side panel visibility */
  action = gtk_action_group_get_action (window->priv->always_sensitive_action_group,
					"ViewSidePanel");
  visible = propono_cache_prefs_get_boolean ("window", "side-panel-visible", TRUE);
  if (gtk_toggle_action_get_active (GTK_TOGGLE_ACTION (action)) != visible)
    gtk_toggle_action_set_active (GTK_TOGGLE_ACTION (action), visible);

  /* toolbar visibility */
  action = gtk_action_group_get_action (window->priv->always_sensitive_action_group,
					"ViewToolbar");
  visible = propono_cache_prefs_get_boolean ("window", "toolbar-visible", TRUE);
  if (gtk_toggle_action_get_active (GTK_TOGGLE_ACTION (action)) != visible)
    gtk_toggle_action_set_active (GTK_TOGGLE_ACTION (action), visible);

  /* statusbar visibility */
  action = gtk_action_group_get_action (window->priv->always_sensitive_action_group,
					"ViewStatusbar");
  visible = propono_cache_prefs_get_boolean ("window", "statusbar-visible", TRUE);
  if (gtk_toggle_action_get_active (GTK_TOGGLE_ACTION (action)) != visible)
    gtk_toggle_action_set_active (GTK_TOGGLE_ACTION (action), visible);

  gtk_window_set_default_size (GTK_WINDOW (window),
			       propono_cache_prefs_get_integer ("window", "window-width", 650),
			       propono_cache_prefs_get_integer ("window", "window-height", 500));

  if ((propono_cache_prefs_get_integer ("window", "window-state", 0) & GDK_WINDOW_STATE_MAXIMIZED) != 0)
    gtk_window_maximize (GTK_WINDOW (window));
  else
    gtk_window_unmaximize (GTK_WINDOW (window));
}

static void
create_statusbar (ProponoWindow *window, 
		  GtkWidget   *main_box)
{
  window->priv->statusbar = gtk_statusbar_new ();

  window->priv->generic_message_cid = gtk_statusbar_get_context_id
	(GTK_STATUSBAR (window->priv->statusbar), "generic_message");
  window->priv->tip_message_cid = gtk_statusbar_get_context_id
	(GTK_STATUSBAR (window->priv->statusbar), "tip_message");

  gtk_box_pack_end (GTK_BOX (main_box),
		    window->priv->statusbar,
		    FALSE, 
		    TRUE, 
		    0);
}

static void
create_notebook (ProponoWindow *window)
{
  window->priv->notebook = propono_notebook_new (window);

  gtk_paned_pack2 (GTK_PANED (window->priv->hpaned), 
  		   GTK_WIDGET (window->priv->notebook),
  		   TRUE, 
  		   FALSE);

  gtk_widget_show (GTK_WIDGET (window->priv->notebook));
}

static gboolean
propono_window_check_first_run (ProponoWindow *window)
{
  gchar *filename;

  filename = g_build_filename (g_get_user_data_dir (),
			       "propono",
			       "first_run",
			       NULL);

  if (!g_file_test (filename, G_FILE_TEST_EXISTS))
    {
      GError *error = NULL;
      GtkWidget *dialog = gtk_message_dialog_new (GTK_WINDOW (window),
						  GTK_DIALOG_DESTROY_WITH_PARENT | GTK_DIALOG_MODAL,
						  GTK_MESSAGE_INFO,
						  GTK_BUTTONS_CLOSE,
						  _("About menu accelerators and keyboard shortcuts"));
      gtk_message_dialog_format_secondary_text (GTK_MESSAGE_DIALOG (dialog),
						_("Propono comes with menu accelerators and keyboard shortcuts disabled by default. The reason is to avoid the keys to be intercepted by the program, and allow them to be sent to the remote machine.\n\nYou can change this behavior through the preferences dialog. For more information, check the documentation.\n\nThis message will appear only once."));     
      gtk_dialog_run (GTK_DIALOG (dialog));
      gtk_widget_destroy (dialog);

      if (propono_utils_create_dir (filename, &error))
        {
          int fd = g_creat (filename, 0644);
          if (fd < 0)
            g_warning (_("Error while creating the file %s: %s"), filename, strerror (errno));
          else
            close (fd);
        }
      else
        {
          g_warning (_("Error while creating the file %s: %s"), filename, error ? error->message: _("Unknown error"));
          g_clear_error (&error);
        }
    }

  g_free (filename);
  return FALSE;
}

static void
propono_window_init (ProponoWindow *window)
{
  GtkWidget *main_box;

  gtk_window_set_default_icon_name ("propono");

  window->priv = PROPONO_WINDOW_GET_PRIVATE (window);
  window->priv->fullscreen = FALSE;
  window->priv->dispose_has_run = FALSE;

  main_box = gtk_vbox_new (FALSE, 0);
  gtk_container_add (GTK_CONTAINER (window), main_box);
  gtk_widget_show (main_box);

  /* Add menu bar and toolbar bar  */
  create_menu_bar_and_toolbar (window, main_box);

  /* Add status bar */
  create_statusbar (window, main_box);

  /* Add the main area */
  window->priv->hpaned = gtk_hpaned_new ();
  gtk_box_pack_start (GTK_BOX (main_box), 
  		      window->priv->hpaned, 
  		      TRUE, 
  		      TRUE, 
  		      0);

  /* setup notebook area */
  create_notebook (window);

  /* side panel */
  create_side_panel (window);

  gtk_widget_show (window->priv->hpaned);

  gtk_widget_grab_focus (window->priv->hpaned);

  init_widgets_visibility (window);
//  propono_window_merge_tab_ui (window);

  propono_window_update_bookmarks_list_menu (window);
  g_signal_connect_swapped (propono_bookmarks_get_default (),
                            "changed",
                            G_CALLBACK (propono_window_update_bookmarks_list_menu),
                            window);
#ifdef PROPONO_ENABLE_AVAHI
  g_signal_connect_swapped (propono_mdns_get_default (),
                            "changed",
                            G_CALLBACK (propono_window_update_bookmarks_list_menu),
                            window);
#endif

  propono_plugins_engine_activate_plugins (propono_plugins_engine_get_default (),
					   window);


  g_idle_add ((GSourceFunc) propono_window_check_first_run, window);
}

ProponoNotebook *
propono_window_get_notebook (ProponoWindow *window)
{
  g_return_val_if_fail (PROPONO_IS_WINDOW (window), NULL);

  return window->priv->notebook;
}

void
propono_window_close_all_tabs (ProponoWindow *window)
{
  g_return_if_fail (PROPONO_IS_WINDOW (window));

  propono_notebook_close_all_tabs (PROPONO_NOTEBOOK (window->priv->notebook));
}

void
propono_window_set_active_tab (ProponoWindow *window,
			       ProponoTab    *tab)
{
  gint page_num;
	
  g_return_if_fail (PROPONO_IS_WINDOW (window));
  g_return_if_fail (PROPONO_IS_TAB (tab));
	
  page_num = gtk_notebook_page_num (GTK_NOTEBOOK (window->priv->notebook),
				    GTK_WIDGET (tab));
  g_return_if_fail (page_num != -1);
	
  gtk_notebook_set_current_page (GTK_NOTEBOOK (window->priv->notebook),
				 page_num);
}

ProponoWindow *
propono_window_new ()
{
  return g_object_new (PROPONO_TYPE_WINDOW,
		       "type",      GTK_WINDOW_TOPLEVEL,
		       "title",     g_get_application_name (),
		       NULL); 
}

gboolean
propono_window_is_fullscreen (ProponoWindow *window)
{
  return window->priv->fullscreen;
}

void
propono_window_toggle_fullscreen (ProponoWindow *window)
{
  if (window->priv->fullscreen)
    gtk_window_unfullscreen (GTK_WINDOW (window));
  else
    gtk_window_fullscreen (GTK_WINDOW (window));
}

GtkWidget *
propono_window_get_statusbar (ProponoWindow *window)
{
  g_return_val_if_fail (PROPONO_IS_WINDOW (window), NULL);

  return window->priv->statusbar;
}

GtkWidget *
propono_window_get_toolbar (ProponoWindow *window)
{
  g_return_val_if_fail (PROPONO_IS_WINDOW (window), NULL);

  return window->priv->toolbar;
}

GtkWidget *
propono_window_get_menubar (ProponoWindow *window)
{
  g_return_val_if_fail (PROPONO_IS_WINDOW (window), NULL);

  return window->priv->menubar;
}

GtkActionGroup *
propono_window_get_initialized_action (ProponoWindow *window)
{
  g_return_val_if_fail (PROPONO_IS_WINDOW (window), NULL);

  return window->priv->machine_initialized_action_group;
}

GtkActionGroup *
propono_window_get_always_sensitive_action (ProponoWindow *window)
{
  g_return_val_if_fail (PROPONO_IS_WINDOW (window), NULL);

  return window->priv->always_sensitive_action_group;
}

GtkActionGroup *
propono_window_get_connected_action (ProponoWindow *window)
{
  g_return_val_if_fail (PROPONO_IS_WINDOW (window), NULL);

  return window->priv->machine_connected_action_group;
}

GtkWidget *
propono_window_get_fav_panel (ProponoWindow *window)
{
  g_return_val_if_fail (PROPONO_IS_WINDOW (window), NULL);

  return window->priv->fav_panel;
}

void
propono_window_close_active_tab	(ProponoWindow *window)
{
  g_return_if_fail (PROPONO_IS_WINDOW (window));

  propono_notebook_close_active_tab (window->priv->notebook);
}

ProponoTab *
propono_window_get_active_tab (ProponoWindow *window)
{
  g_return_val_if_fail (PROPONO_IS_WINDOW (window), NULL);

  return propono_notebook_get_active_tab (window->priv->notebook);
}

GtkUIManager *
propono_window_get_ui_manager (ProponoWindow *window)
{
  g_return_val_if_fail (PROPONO_IS_WINDOW (window), NULL);

  return window->priv->manager;
}

static void
add_connection (ProponoTab *tab, GList **res)
{
  ProponoConnection *conn;
	
  conn = propono_tab_get_conn (tab);
	
  *res = g_list_prepend (*res, conn);
}

/* Returns a newly allocated list with all the connections in the window */
GList *
propono_window_get_connections (ProponoWindow *window)
{
  GList *res = NULL;

  g_return_val_if_fail (PROPONO_IS_WINDOW (window), NULL);
	
  gtk_container_foreach (GTK_CONTAINER (window->priv->notebook),
			 (GtkCallback)add_connection,
			  &res);
			       
  res = g_list_reverse (res);
	
  return res;
}

ProponoTab *
propono_window_conn_exists (ProponoWindow *window, ProponoConnection *conn)
{
  ProponoConnection *c;
  ProponoTab *tab;
  const gchar *host, *protocol;
  gint port;
  GList *conns, *l;

  g_return_val_if_fail (PROPONO_IS_WINDOW (window), NULL);
  g_return_val_if_fail (PROPONO_IS_CONNECTION (conn), NULL);

  host = propono_connection_get_host (conn);
  protocol = propono_connection_get_protocol (conn);
  port = propono_connection_get_port (conn);

  if (!host || !protocol)
    return NULL;

  l = conns = propono_window_get_connections (window);
  tab = NULL;

  while (l != NULL)
    {
      c = PROPONO_CONNECTION (l->data);

      if (!strcmp (host, propono_connection_get_host (c)) &&
	  !strcmp (protocol, propono_connection_get_protocol (c)) &&
	  port == propono_connection_get_port (c))
	{
	  tab = g_object_get_data (G_OBJECT (c), PROPONO_TAB_KEY);
	  break;
	}
      l = l->next;
    }
  g_list_free (conns);

  return tab;
}

/* vim: set ts=8: */
