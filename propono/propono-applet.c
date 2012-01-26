/*
 * propono-applet.c
 * This file is part of propono
 *
 * Copyright (C) 2008,2009 - Jonh Wendell <wendell@bani.com.br>
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

#include <string.h>
#include <glib.h>
#include <gtk/gtk.h>
#include <locale.h>
#include <glib/gi18n.h>
#include <mate-panel-applet.h>

#include "propono-bookmarks.h"
#include "propono-bookmarks-entry.h"
#include "propono-utils.h"
#include "propono-connection.h"
#include "propono-commands.h"
#include "propono-plugin.h"
#include "propono-plugins-engine.h"

#include <config.h>

#ifdef PROPONO_ENABLE_AVAHI
#include "propono-mdns.h"
#endif

#define PROPONO_TYPE_APPLET		(propono_applet_get_type ())
#define PROPONO_APPLET(o)		(G_TYPE_CHECK_INSTANCE_CAST ((o), PROPONO_TYPE_APPLET, ProponoApplet))
#define PROPONO_APPLET_CLASS(k)		(G_TYPE_CHECK_CLASS_CAST((k), PROPONO_TYPE_APPLET, ProponoAppletClass))
#define PROPONO_IS_APPLET(o)		(G_TYPE_CHECK_INSTANCE_TYPE ((o), PROPONO_TYPE_APPLET))
#define PROPONO_IS_APPLET_CLASS(k)	(G_TYPE_CHECK_CLASS_TYPE ((k), PROPONO_TYPE_APPLET))
#define PROPONO_APPLET_GET_CLASS(o)	(G_TYPE_INSTANCE_GET_CLASS ((o), PROPONO_TYPE_APPLET, ProponoAppletClass))

#define PROPONO_APPLET_OAFID		"OAFIID:MATE_ProponoApplet"
#define PROPONO_APPLET_FACTORY_OAFID	"OAFIID:MATE_ProponoApplet_Factory"
#define MATE_PANEL_APPLET_VERTICAL(p)	 (((p) == MATE_PANEL_APPLET_ORIENT_LEFT) || ((p) == MATE_PANEL_APPLET_ORIENT_RIGHT))

typedef struct{
  MatePanelApplet parent;
  GdkPixbuf *icon;
  gint icon_width, icon_height, size;
} ProponoApplet;

typedef struct{
  MatePanelAppletClass parent_class;
} ProponoAppletClass;

GType        propono_applet_get_type   (void);
static void  propono_applet_class_init (ProponoAppletClass *klass);
static void  propono_applet_init       (ProponoApplet *applet);

static void	propono_applet_get_icon		(ProponoApplet *applet);
static void	propono_applet_check_size	(ProponoApplet *applet);
static gboolean	propono_applet_draw_cb		(ProponoApplet *applet);
static void	propono_applet_update_tooltip	(ProponoApplet *applet);
static void	propono_applet_dialog_about_cb	(MateComponentUIComponent *uic, gpointer data, const gchar *verbname);
static gboolean	propono_applet_matecomponent_cb	(MatePanelApplet *_applet, const gchar *iid, gpointer data);
static void	propono_applet_destroy_cb	(GtkObject *object);

G_DEFINE_TYPE (ProponoApplet, propono_applet, PANEL_TYPE_APPLET)

static void
propono_applet_get_icon (ProponoApplet *applet)
{
  if (applet->icon != NULL)
    {
      g_object_unref (applet->icon);
      applet->icon = NULL;
    }

  if (applet->size <= 2)
    return;

  applet->icon = gtk_icon_theme_load_icon (gtk_icon_theme_get_default (),
					   "propono",
					   applet->size - 2,
					   0,
					   NULL);

  applet->icon_height = gdk_pixbuf_get_height (applet->icon);
  applet->icon_width  = gdk_pixbuf_get_width (applet->icon);
}

static void
propono_applet_check_size (ProponoApplet *applet)
{
  /* we don't use the size function here, but the yet allocated size because the
     size value is false (kind of rounded) */
  if (MATE_PANEL_APPLET_VERTICAL(mate_panel_applet_get_orient (MATE_PANEL_APPLET (applet))))
    {
      if (applet->size != GTK_WIDGET(applet)->allocation.width)
	{
	  applet->size = GTK_WIDGET(applet)->allocation.width;
	  propono_applet_get_icon (applet);
	  gtk_widget_set_size_request (GTK_WIDGET(applet), applet->size, applet->icon_height + 2);
	}

      /* Adjusting in case the icon size has changed */
      if (GTK_WIDGET(applet)->allocation.height < applet->icon_height + 2)
	{
	  gtk_widget_set_size_request (GTK_WIDGET(applet), applet->size, applet->icon_height + 2);
	}
    }
  else
    {
      if (applet->size != GTK_WIDGET(applet)->allocation.height)
	{
	  applet->size = GTK_WIDGET(applet)->allocation.height;
	  propono_applet_get_icon (applet);
	  gtk_widget_set_size_request (GTK_WIDGET(applet), applet->icon_width + 2, applet->size);
	}

      /* Adjusting in case the icon size has changed */
      if (GTK_WIDGET(applet)->allocation.width < applet->icon_width + 2)
	{
	  gtk_widget_set_size_request (GTK_WIDGET(applet), applet->icon_width + 2, applet->size);
	}
    }
}

static gboolean
propono_applet_draw_cb (ProponoApplet *applet)
{
  gint w, h, bg_type;
  GdkColor color;
  GdkGC *gc;
  GdkPixmap *background;

  if (GTK_WIDGET (applet)->window == NULL)
    return FALSE;

  /* Clear the window so we can draw on it later */
  gdk_window_clear (GTK_WIDGET(applet)->window);

  /* retrieve applet size */
  propono_applet_get_icon (applet);
  propono_applet_check_size (applet);
  if (applet->size <= 2)
    return FALSE;

  /* if no icon, then don't try to display */
  if (applet->icon == NULL)
    return FALSE;

  w = GTK_WIDGET(applet)->allocation.width;
  h = GTK_WIDGET(applet)->allocation.height;

  gc = gdk_gc_new (GTK_WIDGET(applet)->window);

  /* draw pixmap background */
  bg_type = mate_panel_applet_get_background (MATE_PANEL_APPLET (applet), &color, &background);
  if (bg_type == PANEL_PIXMAP_BACKGROUND)
    {
      /* fill with given background pixmap */
      gdk_draw_drawable (GTK_WIDGET(applet)->window, gc, background, 0, 0, 0, 0, w, h);
    }
	
  /* draw color background */
  if (bg_type == PANEL_COLOR_BACKGROUND)
    {
      gdk_gc_set_rgb_fg_color (gc,&color);
      gdk_gc_set_fill (gc,GDK_SOLID);
      gdk_draw_rectangle (GTK_WIDGET(applet)->window, gc, TRUE, 0, 0, w, h);
    }

  /* draw icon at center */
  gdk_draw_pixbuf (GTK_WIDGET(applet)->window, gc, applet->icon,
		   0, 0, (w - applet->icon_width)/2, (h - applet->icon_height)/2,
		   applet->icon_width, applet->icon_height,
		   GDK_RGB_DITHER_NONE, 0, 0);

  return TRUE;
}

static void
propono_applet_change_background_cb (ProponoApplet *applet,
				     MatePanelAppletBackgroundType arg1,
				     GdkColor *arg2, GdkPixmap *arg3, gpointer data)
{
  gtk_widget_queue_draw (GTK_WIDGET (applet));
}

static void
propono_applet_destroy_cb (GtkObject *object)
{
  ProponoApplet *applet = PROPONO_APPLET (object);

  if (applet->icon != NULL)
    g_object_unref (applet->icon);
}

static void
propono_applet_class_init (ProponoAppletClass *klass)
{
  /* nothing to do here */
}

static void
menu_position (GtkMenu    *menu,
	       gint       *x,
	       gint       *y,
	       gboolean   *push_in,
	       GtkWidget  *applet)
{
  int applet_height, applet_width;
  GtkRequisition requisition;

  gdk_window_get_origin (applet->window, x, y);
  gdk_drawable_get_size (applet->window, &applet_width, &applet_height);
  gtk_widget_size_request (GTK_WIDGET (menu), &requisition);

  switch (mate_panel_applet_get_orient (MATE_PANEL_APPLET (applet)))
    {
      case MATE_PANEL_APPLET_ORIENT_DOWN:
	*y += applet_height + 1;
	break;

      case MATE_PANEL_APPLET_ORIENT_UP:
	*y = MAX (*y - requisition.height - 2, 2*applet_height);
	break;

      case MATE_PANEL_APPLET_ORIENT_LEFT:
	*x = MAX (*x - requisition.width - 2, 2*applet_width);
	break;

      case MATE_PANEL_APPLET_ORIENT_RIGHT:
	*x += applet_height + 1;
	break;

      default:
	g_assert_not_reached ();
    }

  *push_in = TRUE;
}

static void
open_connection_cb (GtkMenuItem *item,
		    gpointer    *user_data)
{
  GError *err = NULL;
  gchar **argv;
  ProponoConnection *conn;

  conn = PROPONO_CONNECTION (g_object_get_data (G_OBJECT (item), "conn"));

  argv = g_new0 (gchar *, 3);
  argv[0] = g_strdup ("propono");
  argv[1] = g_strdup (propono_connection_get_string_rep (conn, TRUE));
  argv[2] = NULL;

  if (!gdk_spawn_on_screen (gtk_widget_get_screen (GTK_WIDGET (item)),
			    NULL,
			    argv,
			    NULL,
			    G_SPAWN_SEARCH_PATH,
			    NULL,
			    NULL,
			    NULL,
			    &err))
    {
      propono_utils_show_error (_("Could not run propono:"), err->message, NULL);
      g_error_free (err);
    }

  g_strfreev (argv);
}

static void
fill_recursive_menu (GSList *entries, GtkWidget *menu)
{
  GSList    *l;
  GtkWidget *item, *image, *child;

  for (l = entries; l; l = l->next)
    {
      ProponoBookmarksEntry *entry = PROPONO_BOOKMARKS_ENTRY (l->data);
      ProponoConnection     *conn;
      ProponoPlugin         *plugin;

      switch (propono_bookmarks_entry_get_node (entry))
	{
	  case PROPONO_BOOKMARKS_ENTRY_NODE_FOLDER:
	    image = gtk_image_new_from_icon_name ("folder", GTK_ICON_SIZE_MENU);
	    item = gtk_image_menu_item_new_with_label (propono_bookmarks_entry_get_name (entry));
	    gtk_image_menu_item_set_image (GTK_IMAGE_MENU_ITEM (item),
					   image);
	    gtk_menu_shell_append (GTK_MENU_SHELL (menu),
				   item);

	    child = gtk_menu_new ();
	    gtk_menu_item_set_submenu (GTK_MENU_ITEM (item), child);

	    fill_recursive_menu (propono_bookmarks_entry_get_children (entry),
				 child);
	    break;

	  case PROPONO_BOOKMARKS_ENTRY_NODE_CONN:
	    conn = propono_bookmarks_entry_get_conn (entry);
	    plugin = propono_plugins_engine_get_plugin_by_protocol (propono_plugins_engine_get_default (),
								    propono_connection_get_protocol (conn));

	    image = gtk_image_new_from_icon_name (propono_plugin_get_icon_name (plugin),
						  GTK_ICON_SIZE_MENU);
	    item = gtk_image_menu_item_new_with_label (propono_connection_get_name (conn));
	    gtk_image_menu_item_set_image (GTK_IMAGE_MENU_ITEM (item),
					   image);

	    gtk_menu_shell_append (GTK_MENU_SHELL (menu),
				   item);
	    g_object_set_data (G_OBJECT (item), "conn", conn);
	    g_signal_connect (item, "activate", G_CALLBACK (open_connection_cb), NULL);
	    break;

	  default:
	    g_assert_not_reached ();
	}
    }

}

static void
fill_menu (GSList *all, GtkWidget *menu)
{
  GtkWidget *item, *image;

  if (g_slist_length (all) == 0)
    return;

  /* Separator */
  item = gtk_separator_menu_item_new ();
  gtk_menu_shell_append (GTK_MENU_SHELL (menu), item);

  fill_recursive_menu (all, menu);
}

static void
open_propono_cb (GtkMenuItem *item,
		 gpointer    *user_data)
{
  GError *err = NULL;

  if (!g_spawn_command_line_async ("propono", &err))
    {
      propono_utils_show_error (_("Could not run propono:"), err->message, NULL);
      g_error_free (err);
    }
}

static gboolean
propono_applet_click_cb (GtkWidget      *applet,
			 GdkEventButton *event,
			 gpointer        user_data)
{
  GtkWidget *menu, *item, *image;
  GSList *all;

  if ((event->type != GDK_BUTTON_PRESS) || (event->button != 1))
    return FALSE;

  menu = gtk_menu_new ();

  /* Open, first item */
  image = gtk_image_new_from_icon_name ("propono", GTK_ICON_SIZE_MENU);
  item = gtk_image_menu_item_new_with_label (_("Open Remote Desktop Viewer"));
  gtk_image_menu_item_set_image (GTK_IMAGE_MENU_ITEM (item),
				 image);
  gtk_menu_shell_append (GTK_MENU_SHELL (menu), item);
  g_signal_connect (item, "activate", G_CALLBACK (open_propono_cb), NULL);

  all = propono_bookmarks_get_all (propono_bookmarks_get_default ());
  fill_menu (all, menu);

#ifdef PROPONO_ENABLE_AVAHI
  all = propono_mdns_get_all (propono_mdns_get_default ());
  fill_menu (all, menu);
#endif

  gtk_widget_show_all (menu);
  gtk_menu_popup (GTK_MENU (menu), NULL, NULL, (GtkMenuPositionFunc) menu_position, applet, 
		  event->button, event->time);
  return TRUE;
}

static void
propono_applet_help_cb (MateComponentUIComponent *ui_container,
			gpointer           data,
			const gchar       *cname)
{
  propono_utils_help_contents (NULL, NULL);
}

static void
propono_applet_about_cb (MateComponentUIComponent *ui_container,
			 gpointer           data,
			 const gchar       *cname)
{
  propono_utils_help_about (NULL);
}


static void
propono_applet_init (ProponoApplet *applet)
{
  gchar *tmp;
#ifdef PROPONO_ENABLE_AVAHI
  ProponoMdns *mdns;
#endif

  setlocale (LC_ALL, "");
  bindtextdomain (GETTEXT_PACKAGE, PACKAGE_LOCALE_DIR);
  bind_textdomain_codeset (GETTEXT_PACKAGE, "UTF-8");
  textdomain (GETTEXT_PACKAGE);

  applet->size = 0;
  applet->icon = NULL;

  tmp = g_strdup_printf ("%s\n%s",
			_("Remote Desktop Viewer"),
			_("Access your bookmarks"));
  gtk_widget_set_tooltip_text (GTK_WIDGET (applet), tmp);
  g_free (tmp);

  mate_panel_applet_set_flags (MATE_PANEL_APPLET (applet), MATE_PANEL_APPLET_EXPAND_MINOR);
  gtk_widget_show_all (GTK_WIDGET(applet));
  propono_applet_draw_cb (applet);

  g_signal_connect (G_OBJECT (applet), "button-press-event",
		    G_CALLBACK (propono_applet_click_cb), NULL);

  g_signal_connect (G_OBJECT (applet), "expose-event",
		    G_CALLBACK (propono_applet_draw_cb), NULL);

  /* We use g_signal_connect_after because letting the panel draw
   * the background is the only way to have the correct
   * background when a theme defines a background picture. */
  g_signal_connect_after (G_OBJECT (applet), "expose-event",
			  G_CALLBACK (propono_applet_draw_cb), NULL);

  g_signal_connect (G_OBJECT (applet), "change-background",
		    G_CALLBACK (propono_applet_change_background_cb), NULL);

  g_signal_connect (G_OBJECT (applet), "change-orient",
		    G_CALLBACK (propono_applet_draw_cb), NULL);

  g_signal_connect (G_OBJECT (applet), "destroy",
		    G_CALLBACK (propono_applet_destroy_cb), NULL);

#ifdef PROPONO_ENABLE_AVAHI
  mdns = propono_mdns_get_default ();
#endif
}

static gboolean
propono_applet_fill (MatePanelApplet *_applet,
		     const gchar *iid,
		     gpointer     data)
{
  ProponoApplet *applet = PROPONO_APPLET (_applet);

  static const MateComponentUIVerb menu_verbs[] = {
    MATECOMPONENT_UI_VERB ("ProponoHelp", propono_applet_help_cb),
    MATECOMPONENT_UI_VERB ("ProponoAbout", propono_applet_about_cb),
    MATECOMPONENT_UI_VERB_END
  };

  if (strcmp (iid, PROPONO_APPLET_OAFID) != 0)
    return FALSE;

  gtk_window_set_default_icon_name ("propono");
  g_set_application_name (_("Remote Desktop Viewer"));

  mate_panel_applet_setup_menu_from_file (_applet,
				     NULL,
				     DATADIR "/propono/MATE_ProponoApplet.xml",
				     NULL, menu_verbs, applet);
  propono_applet_draw_cb (applet);
  return TRUE;
}

MATE_PANEL_APPLET_MATECOMPONENT_FACTORY (PROPONO_APPLET_FACTORY_OAFID,
                             PROPONO_TYPE_APPLET,
                             "ProponoApplet",
                             VERSION,
                             propono_applet_fill,
                             NULL);

/* vim: set ts=8: */
