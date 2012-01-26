/*
 * propono-notebook.c
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

#include <glib/gi18n.h>

#include "propono-notebook.h"
#include "propono-utils.h"
#include "propono-dnd.h"
#include "propono-prefs.h"
#include "propono-spinner.h"

#define PROPONO_NOTEBOOK_GET_PRIVATE(object)(G_TYPE_INSTANCE_GET_PRIVATE ((object), PROPONO_TYPE_NOTEBOOK, ProponoNotebookPrivate))

struct _ProponoNotebookPrivate
{
  ProponoWindow *window;
  GtkUIManager  *manager;
  guint         ui_merge_id;
  ProponoTab    *active_tab;
  GSList        *tabs;
};

/* Properties */
enum
{
  PROP_0,
  PROP_WINDOW
};

G_DEFINE_TYPE(ProponoNotebook, propono_notebook, GTK_TYPE_NOTEBOOK)

static void
propono_notebook_get_property (GObject    *object,
			       guint       prop_id,
			       GValue     *value,
			       GParamSpec *pspec)
{
  ProponoNotebook *nb = PROPONO_NOTEBOOK (object);

  switch (prop_id)
    {
      case PROP_WINDOW:
        g_value_set_object (value, nb->priv->window);
	break;
      default:
	G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
	break;			
    }
}

static void
propono_notebook_set_window (ProponoNotebook *nb, ProponoWindow *window)
{
  nb->priv->window = window;
  nb->priv->manager = propono_window_get_ui_manager (window);
  nb->priv->ui_merge_id = gtk_ui_manager_new_merge_id (nb->priv->manager);
}

static void
propono_notebook_set_property (GObject      *object,
			       guint         prop_id,
			       const GValue *value,
			       GParamSpec   *pspec)
{
  ProponoNotebook *nb = PROPONO_NOTEBOOK (object);

  switch (prop_id)
    {
      case PROP_WINDOW:
	propono_notebook_set_window (nb, PROPONO_WINDOW (g_value_get_object (value)));
        break;
      default:
        G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
        break;			
    }
}

static void
propono_notebook_finalize (GObject *object)
{
  ProponoNotebook *nb = PROPONO_NOTEBOOK (object);

  if (nb->priv->tabs)
    {
      g_slist_free (nb->priv->tabs);
      nb->priv->tabs = NULL;
    }

  G_OBJECT_CLASS (propono_notebook_parent_class)->finalize (object);
}

static void
propono_notebook_class_init (ProponoNotebookClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  object_class->get_property = propono_notebook_get_property;
  object_class->set_property = propono_notebook_set_property;
  object_class->finalize = propono_notebook_finalize;

  g_object_class_install_property (object_class,
				   PROP_WINDOW,
				   g_param_spec_object ("window",
							"Window",
							"The ProponoWindow",
							PROPONO_TYPE_WINDOW,
							G_PARAM_READWRITE |
							G_PARAM_CONSTRUCT_ONLY |
							G_PARAM_STATIC_NAME |
							G_PARAM_STATIC_NICK |
							G_PARAM_STATIC_BLURB));

  g_type_class_add_private (object_class, sizeof(ProponoNotebookPrivate));
}

ProponoNotebook *
propono_notebook_new (ProponoWindow *window)
{
  return PROPONO_NOTEBOOK (g_object_new (PROPONO_TYPE_NOTEBOOK, "window", window, NULL));
}

void
propono_notebook_show_hide_tabs (ProponoNotebook *nb)
{
  gboolean always, fs;
  gint     n;

  fs = propono_window_is_fullscreen (nb->priv->window);
  n = gtk_notebook_get_n_pages (GTK_NOTEBOOK (nb));

  g_object_get (propono_prefs_get_default (),
		"always-show-tabs", &always,
		NULL);

  gtk_notebook_set_show_tabs (GTK_NOTEBOOK (nb),
			      ((n > 1) || (always)) && !fs);
}

static void
drag_data_get_handl (GtkWidget *widget,
		     GdkDragContext *context,
		     GtkSelectionData *selection_data,
		     guint target_type,
		     guint time,
		     ProponoTab *tab)
{
  gchar *uri, *name, *data;
  ProponoConnection *conn;

  g_assert (selection_data != NULL);

  switch (target_type)
    {
      case TARGET_PROPONO:
	conn = propono_tab_get_conn (tab);
	uri = propono_connection_get_string_rep (conn, TRUE);
	name = propono_connection_get_best_name (conn);
	data = g_strdup_printf ("%s||%s", name, uri);

	/*FIXME: Set other properties of the connection*/
	gtk_selection_data_set (selection_data,
				selection_data->target,
				8,
				(guchar*) data,
				strlen (data));

	g_free (data);
	g_free (name);
	g_free (uri);
	break;

      case TARGET_STRING:
	/*FIXME: Create a .VNC file*/
	break;

      default:
	g_assert_not_reached ();
    }
}

static void
propono_notebook_update_ui_sentitivity (ProponoNotebook *nb)
{
  gboolean       active;
  GtkActionGroup *action_group;

  active = gtk_notebook_get_n_pages (GTK_NOTEBOOK (nb)) > 0;
  action_group = propono_window_get_connected_action (nb->priv->window);
  gtk_action_group_set_sensitive (action_group, active);

  action_group = propono_window_get_initialized_action (nb->priv->window);
  active = (nb->priv->active_tab) &&
	   (propono_tab_get_state (PROPONO_TAB (nb->priv->active_tab)) == PROPONO_TAB_STATE_CONNECTED);
  gtk_action_group_set_sensitive (action_group, active);

  if (nb->priv->active_tab)
    {
      GtkWidget *spinner, *icon;

      spinner = g_object_get_data (G_OBJECT (nb->priv->active_tab), "spinner");
      icon = g_object_get_data (G_OBJECT (nb->priv->active_tab), "icon");

      if (propono_tab_get_state (PROPONO_TAB (nb->priv->active_tab)) == PROPONO_TAB_STATE_CONNECTED)
	{
	  gtk_widget_hide (spinner);
	  propono_spinner_stop (PROPONO_SPINNER (spinner));
	  gtk_widget_show (icon);
	}
      else
	{
	  gtk_widget_hide (icon);
	  gtk_widget_show (spinner);
	  propono_spinner_start (PROPONO_SPINNER (spinner));
	}
    }
}

static void
propono_notebook_update_window_title (ProponoNotebook *nb)
{
  gchar *title, *name, *extra;

  if (nb->priv->active_tab == NULL)
    {
      gtk_window_set_title (GTK_WINDOW (nb->priv->window), g_get_application_name ());
      return;
    }

  extra = propono_tab_get_extra_title (nb->priv->active_tab);
  name = propono_connection_get_best_name (propono_tab_get_conn (nb->priv->active_tab));
  if (extra)
    title = g_strdup_printf ("%s %s - %s",
			     name,
			     extra,
			     g_get_application_name ());
  else
    title = g_strdup_printf ("%s - %s",
			     name,
			     g_get_application_name ());

  gtk_window_set_title (GTK_WINDOW (nb->priv->window), title);
  g_free (title);
  g_free (extra);
  g_free (name);
}

static void
insert_actions_ui (ProponoNotebook *nb, GtkActionGroup *action_group, const GSList *actions)
{
  ProponoTabUiAction *action;
  const gchar        *name;
  gint               i;

  while (actions)
    {
      action = (ProponoTabUiAction *) actions->data;
      name = gtk_action_get_name (action->action);

      gtk_action_group_add_action (action_group, action->action);

      for (i = 0; i < g_strv_length (action->paths); i++)
	gtk_ui_manager_add_ui (nb->priv->manager,
			       nb->priv->ui_merge_id,
			       action->paths[i],
			       name,
			       name,
			       GTK_UI_MANAGER_AUTO,
			       FALSE);

      actions = actions->next;
    }
}

static void
merge_tab_ui (ProponoNotebook *nb)
{
  const GSList   *actions;
  GtkActionGroup *action_group;

  if (!nb->priv->active_tab)
    return;

  /* Always sensitive actions */
  action_group = propono_window_get_always_sensitive_action (nb->priv->window);
  actions = propono_tab_get_always_sensitive_actions (nb->priv->active_tab);
  insert_actions_ui (nb, action_group, actions);

  /* Connected actions */
  action_group = propono_window_get_connected_action (nb->priv->window);
  actions = propono_tab_get_connected_actions (nb->priv->active_tab);
  insert_actions_ui (nb, action_group, actions);

  /* Initialized actions */
  action_group = propono_window_get_initialized_action (nb->priv->window);
  actions = propono_tab_get_initialized_actions (nb->priv->active_tab);
  insert_actions_ui (nb, action_group, actions);
}

static void
remove_actions_ui (ProponoNotebook *nb, GtkActionGroup *action_group, const GSList *actions)
{
  ProponoTabUiAction *action;

  while (actions)
    {
      action = (ProponoTabUiAction *) actions->data;

      gtk_action_group_remove_action (action_group, action->action);
      actions = actions->next;
    }
}

static void
unmerge_tab_ui (ProponoNotebook *nb)
{
  const GSList   *actions;
  GtkActionGroup *action_group;

  if (!nb->priv->active_tab)
    return;

  gtk_ui_manager_remove_ui (nb->priv->manager,
			    nb->priv->ui_merge_id);

  /* Always sensitive actions */
  action_group = propono_window_get_always_sensitive_action (nb->priv->window);
  actions = propono_tab_get_always_sensitive_actions (nb->priv->active_tab);
  remove_actions_ui (nb, action_group, actions);

  /* Connected actions */
  action_group = propono_window_get_connected_action (nb->priv->window);
  actions = propono_tab_get_connected_actions (nb->priv->active_tab);
  remove_actions_ui (nb, action_group, actions);

  /* Initialized actions */
  action_group = propono_window_get_initialized_action (nb->priv->window);
  actions = propono_tab_get_initialized_actions (nb->priv->active_tab);
  remove_actions_ui (nb, action_group, actions);
}

static void 
propono_notebook_page_switched (GtkNotebook     *notebook,
				GtkNotebookPage *pg,
				gint            page_num, 
				gpointer        data)
{
  ProponoNotebook *nb = PROPONO_NOTEBOOK (notebook);
  ProponoTab *tab;

  tab = PROPONO_TAB (gtk_notebook_get_nth_page (notebook, page_num));
  if (tab == nb->priv->active_tab)
    return;

  unmerge_tab_ui (nb);
  nb->priv->active_tab = tab;
  merge_tab_ui (nb);

  propono_notebook_update_window_title (nb);
  propono_notebook_update_ui_sentitivity (nb);
}

static void
propono_notebook_page_removed_cb (ProponoNotebook *nb)
{
  gint n;

  if ((gtk_notebook_get_n_pages (GTK_NOTEBOOK (nb)) == 0) &&
      (propono_window_is_fullscreen (nb->priv->window)))
    propono_window_toggle_fullscreen (nb->priv->window);

propono_notebook_show_hide_tabs (nb);
}

static void
propono_notebook_init (ProponoNotebook *nb)
{
  nb->priv = PROPONO_NOTEBOOK_GET_PRIVATE (nb);
  nb->priv->active_tab = NULL;
  nb->priv->tabs = NULL;

  gtk_notebook_set_scrollable (GTK_NOTEBOOK (nb), TRUE);

  g_signal_connect (nb,
		    "page-added",
		    G_CALLBACK (propono_notebook_show_hide_tabs),
		    NULL);
  g_signal_connect (nb,
		    "page-removed",
		    G_CALLBACK (propono_notebook_page_removed_cb),
		    NULL);
  g_signal_connect (nb,
		    "switch-page",
		    G_CALLBACK (propono_notebook_page_switched),
		    NULL);
  g_signal_connect_swapped (propono_prefs_get_default (),
			    "notify::always-show-tabs",
			     G_CALLBACK (propono_notebook_show_hide_tabs),
			     nb);
}

static void
close_button_clicked_cb (GtkWidget *widget, 
			 GtkWidget *tab)
{
  ProponoNotebook *notebook;

  notebook = PROPONO_NOTEBOOK (gtk_widget_get_parent (tab));
  propono_notebook_close_tab (notebook, PROPONO_TAB (tab));
}

static void
propono_notebook_update_tab_tooltip (ProponoNotebook *nb, ProponoTab *tab)
{
  char       *str;
  GtkWidget  *label;

  label = GTK_WIDGET (g_object_get_data (G_OBJECT (tab), "label-ebox"));
  g_return_if_fail (label != NULL);

  str = propono_tab_get_tooltip (tab);
  gtk_widget_set_tooltip_markup (label, str);

  g_free (str);
}

static void
tab_tooltip_changed_cb (GObject *object, GParamSpec *pspec, ProponoNotebook *nb)
{
  propono_notebook_update_tab_tooltip (nb, PROPONO_TAB (object));
}

static void
tab_disconnected_cb (ProponoTab *tab, ProponoNotebook *nb)
{
  gchar *message, *name, *emphasis;

  name = propono_connection_get_best_name (propono_tab_get_conn (tab));
  emphasis = g_strdup_printf ("<i>%s</i>", name);
  /* Translators: %s is a host name or IP address. */
  message = g_strdup_printf (_("Connection to host %s was closed."),
			     emphasis);
  propono_utils_show_error (_("Connection closed"), message, GTK_WINDOW (nb->priv->window));
  g_free (message);
  g_free (name);
  g_free (emphasis);

  propono_notebook_close_tab (nb, tab);
}

static void
tab_auth_failed_cb (ProponoTab *tab, const gchar *msg, ProponoNotebook *nb)
{
  GString *message;
  gchar   *name, *emphasis;

  message = g_string_new (NULL);
  name = propono_connection_get_best_name (propono_tab_get_conn (tab));

  emphasis = g_strdup_printf ("<i>%s</i>", name);
  /* Translators: %s is a host name or IP address. */
  g_string_printf (message, _("Authentication to host %s has failed"),
		   emphasis);
  if (msg)
  	g_string_append_printf (message, " (%s)", msg);
  g_string_append_c (message, '.');

  propono_utils_show_error (_("Authentication failed"), message->str, GTK_WINDOW (nb->priv->window));
  g_string_free (message, TRUE);
  g_free (name);
  g_free (emphasis);

  propono_notebook_close_tab (nb, tab);
}

static void
tab_initialized_cb (ProponoTab *tab, ProponoNotebook *nb)
{
  ProponoConnection *conn = propono_tab_get_conn (tab);

  propono_notebook_update_ui_sentitivity (nb);
  propono_notebook_update_tab_tooltip (nb, tab);
  propono_notebook_update_window_title (nb);

  if (propono_connection_get_fullscreen (conn) && (!propono_window_is_fullscreen (nb->priv->window)))
    propono_window_toggle_fullscreen (nb->priv->window);
}

static GtkWidget *
build_tab_label (ProponoNotebook *nb, 
		 ProponoTab      *tab)
{
  GtkWidget *hbox, *label_hbox, *label_ebox;
  GtkWidget *label, *dummy_label;
  GtkWidget *close_button, *spinner;
  GtkRcStyle *rcstyle;
  GtkWidget *image;
  GtkWidget *icon;
  gchar     *name;

  hbox = gtk_hbox_new (FALSE, 4);

  label_ebox = gtk_event_box_new ();
  gtk_event_box_set_visible_window (GTK_EVENT_BOX (label_ebox), FALSE);
  gtk_box_pack_start (GTK_BOX (hbox), label_ebox, TRUE, TRUE, 0);
  gtk_widget_set_tooltip_text (label_ebox, _("Connecting..."));

  label_hbox = gtk_hbox_new (FALSE, 4);
  gtk_container_add (GTK_CONTAINER (label_ebox), label_hbox);

  /* setup close button */
  close_button = gtk_button_new ();
  gtk_button_set_relief (GTK_BUTTON (close_button),
			 GTK_RELIEF_NONE);

  /* don't allow focus on the close button */
  gtk_button_set_focus_on_click (GTK_BUTTON (close_button), FALSE);

  /* make it as small as possible */
  rcstyle = gtk_rc_style_new ();
  rcstyle->xthickness = rcstyle->ythickness = 0;
  gtk_widget_modify_style (close_button, rcstyle);
  g_object_unref (rcstyle);
  gint w, h;
  gtk_icon_size_lookup (GTK_ICON_SIZE_MENU, &w, &h);
  gtk_widget_set_size_request (GTK_WIDGET (close_button), w+2, h+2);

  image = gtk_image_new_from_stock (GTK_STOCK_CLOSE,
					  GTK_ICON_SIZE_MENU);
  gtk_container_add (GTK_CONTAINER (close_button), image);
  gtk_box_pack_start (GTK_BOX (hbox), close_button, FALSE, FALSE, 0);

  gtk_widget_set_tooltip_text (close_button, _("Close connection"));

  g_signal_connect (close_button,
		    "clicked",
		    G_CALLBACK (close_button_clicked_cb),
		    tab);

  /* setup spinner */
  spinner = propono_spinner_new ();
  propono_spinner_set_size (PROPONO_SPINNER (spinner), GTK_ICON_SIZE_MENU);
  gtk_box_pack_start (GTK_BOX (label_hbox), spinner, FALSE, FALSE, 0);

  /* setup site icon */
  icon = gtk_image_new_from_icon_name (propono_tab_get_icon_name (tab),
				       GTK_ICON_SIZE_MENU);
  gtk_box_pack_start (GTK_BOX (label_hbox), icon, FALSE, FALSE, 0);
	
  /* setup label */
  name = propono_connection_get_best_name (propono_tab_get_conn (tab));
  label = gtk_label_new (name);
  g_free (name);
  gtk_misc_set_alignment (GTK_MISC (label), 0.0, 0.5);
  gtk_misc_set_padding (GTK_MISC (label), 0, 0);
  gtk_box_pack_start (GTK_BOX (label_hbox), label, FALSE, FALSE, 0);

  dummy_label = gtk_label_new ("");
  gtk_box_pack_start (GTK_BOX (label_hbox), dummy_label, TRUE, TRUE, 0);
	
  gtk_widget_show (hbox);
  gtk_widget_show (label_ebox);
  gtk_widget_show (label_hbox);
  gtk_widget_show (label);
  gtk_widget_show (dummy_label);	
  gtk_widget_show (image);
  gtk_widget_show (close_button);
  
  g_object_set_data (G_OBJECT (hbox), "label", label);
  g_object_set_data (G_OBJECT (tab),  "label", label);
  g_object_set_data (G_OBJECT (hbox), "label-ebox", label_ebox);
  g_object_set_data (G_OBJECT (tab),  "label-ebox", label_ebox);
  g_object_set_data (G_OBJECT (tab), "spinner", spinner);
  g_object_set_data (G_OBJECT (tab), "icon", icon);
  g_object_set_data (G_OBJECT (hbox), "close-button", close_button);
  g_object_set_data (G_OBJECT (tab),  "close-button", close_button);

  gtk_drag_source_set ( GTK_WIDGET (hbox),
			GDK_BUTTON1_MASK,
			propono_target_list,
			G_N_ELEMENTS (propono_target_list),
			GDK_ACTION_COPY );
  g_signal_connect (hbox,
		    "drag-data-get",
		    G_CALLBACK (drag_data_get_handl),
		    tab);

  return hbox;
}

void
propono_notebook_add_tab (ProponoNotebook *nb,
			  ProponoTab      *tab,
			  gint           position)
{
  GtkWidget      *label;
  GtkActionGroup *action_group;
  int            pos;

  g_return_if_fail (PROPONO_IS_NOTEBOOK (nb));
  g_return_if_fail (PROPONO_IS_TAB (tab));

  /* Unmerge the UI for the current tab */
  unmerge_tab_ui (nb);

  nb->priv->active_tab = tab;
  nb->priv->tabs = g_slist_append (nb->priv->tabs, tab);
  
  /* Merge the UI for the new tab */
  merge_tab_ui (nb);

  /* Actually add the new tab */
  label = build_tab_label (nb, tab);
  pos = gtk_notebook_insert_page (GTK_NOTEBOOK (nb), 
				  GTK_WIDGET (tab),
				  label, 
				  position);

  gtk_notebook_set_current_page (GTK_NOTEBOOK (nb), pos);
  propono_tab_set_notebook (tab, nb);

  g_signal_connect (tab,
		    "notify::tooltip",
		    G_CALLBACK (tab_tooltip_changed_cb),
		    nb);

  g_signal_connect (tab,
		    "tab-disconnected",
		    G_CALLBACK (tab_disconnected_cb),
		    nb);

  g_signal_connect (tab,
		    "tab-auth-failed",
		    G_CALLBACK (tab_auth_failed_cb),
		    nb);

  g_signal_connect (tab,
		    "tab-initialized",
		    G_CALLBACK (tab_initialized_cb),
		    nb);

  propono_notebook_update_window_title (nb);
  propono_notebook_update_ui_sentitivity (nb);
}

void
propono_notebook_close_tab (ProponoNotebook *nb,
			    ProponoTab      *tab)
{
  gint           position;
  GtkActionGroup *action_group;
  GtkNotebook    *notebook;
  ProponoTab     *previous_active_tab;

  g_return_if_fail (PROPONO_IS_NOTEBOOK (nb));
  g_return_if_fail (PROPONO_IS_TAB (tab));

  notebook = GTK_NOTEBOOK (nb);
  previous_active_tab = nb->priv->active_tab;

  g_signal_handlers_disconnect_by_func (tab,
					G_CALLBACK (tab_disconnected_cb),
					nb);
  g_signal_handlers_disconnect_by_func (tab,
					G_CALLBACK (tab_auth_failed_cb),
					nb);

  /* If it's closing the current tab, unmerge the UI */
  if (nb->priv->active_tab == tab)
    unmerge_tab_ui (nb);

  position = gtk_notebook_page_num (notebook, GTK_WIDGET (tab));
  g_signal_handlers_block_by_func (notebook, propono_notebook_page_switched, NULL);
  gtk_notebook_remove_page (notebook, position);
  g_signal_handlers_unblock_by_func (notebook, propono_notebook_page_switched, NULL);
  
  position = gtk_notebook_get_current_page (notebook);
  nb->priv->active_tab = PROPONO_TAB (gtk_notebook_get_nth_page (notebook,
								 position));
  nb->priv->tabs = g_slist_remove (nb->priv->tabs, tab);

  /* Merge the UI for the new tab (if one exists) */
  if (nb->priv->active_tab != previous_active_tab)
    {
      merge_tab_ui (nb);
      propono_notebook_update_window_title (nb);
      propono_notebook_update_ui_sentitivity (nb);
    }
}

static void
close_tab (ProponoTab *tab,
	    ProponoNotebook *nb)
{
  propono_notebook_close_tab (nb, tab);
}

void
propono_notebook_close_all_tabs (ProponoNotebook *nb)
{	
  g_return_if_fail (PROPONO_IS_NOTEBOOK (nb));
	
  gtk_container_foreach (GTK_CONTAINER (nb),
			(GtkCallback) close_tab,
			 nb);
}

void
propono_notebook_close_active_tab (ProponoNotebook *nb)
{
  g_return_if_fail (PROPONO_IS_NOTEBOOK (nb));
  g_return_if_fail (nb->priv->active_tab != NULL);

  propono_notebook_close_tab (nb, nb->priv->active_tab);
}

ProponoTab *
propono_notebook_get_active_tab (ProponoNotebook *nb)
{
  g_return_val_if_fail (PROPONO_IS_NOTEBOOK (nb), NULL);

  return nb->priv->active_tab;
}

GSList *
propono_notebook_get_tabs (ProponoNotebook *nb)
{
  g_return_val_if_fail (PROPONO_IS_NOTEBOOK (nb), NULL);

  return nb->priv->tabs;
}

/* vim: set ts=8: */
