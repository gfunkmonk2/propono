/*
 * propono-app.c
 * This file is part of propono
 *
 * Copyright (C) 2008 - Jonh Wendell <wendell@bani.com.br>
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

#include <string.h>
#include <glib/gi18n.h>

#include "propono-app.h"
#include "propono-utils.h"

#define PROPONO_APP_GET_PRIVATE(object)(G_TYPE_INSTANCE_GET_PRIVATE ((object), PROPONO_TYPE_APP, ProponoAppPrivate))


struct _ProponoAppPrivate
{
  GList *windows;
  ProponoWindow *active_window;
};

G_DEFINE_TYPE(ProponoApp, propono_app, G_TYPE_OBJECT)

static void
propono_app_finalize (GObject *object)
{
  ProponoApp *app = PROPONO_APP (object); 

  g_list_free (app->priv->windows);

  G_OBJECT_CLASS (propono_app_parent_class)->finalize (object);
}

static void 
propono_app_class_init (ProponoAppClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  object_class->finalize = propono_app_finalize;

  g_type_class_add_private (object_class, sizeof(ProponoAppPrivate));
}

static gchar *
get_accel_file (void)
{
  const gchar *home;

  home = g_get_user_config_dir ();

  if (home)
    return g_build_filename (home,
			     "propono",
			     "accels",
			     NULL);
  return NULL;
}

static void
load_accels (void)
{
  gchar *filename;

  filename = get_accel_file ();
  if (!filename)
    return;

  gtk_accel_map_load (filename);
  g_free (filename);
}

static void
save_accels (void)
{
  gchar *filename;

  filename = get_accel_file ();
  if (!filename)
    return;

  gtk_accel_map_save (filename);
  g_free (filename);
}

static void
propono_app_init (ProponoApp *app)
{
  app->priv = PROPONO_APP_GET_PRIVATE (app);
  app->priv->windows = NULL;
  app->priv->active_window = NULL;

  load_accels ();
}

static void
app_weak_notify (gpointer data,
		 GObject *where_the_app_was)
{
  gtk_main_quit ();
}

ProponoApp *
propono_app_get_default (void)
{
  static ProponoApp *app = NULL;

  if (G_LIKELY (app))
    return app;

  app = PROPONO_APP (g_object_new (PROPONO_TYPE_APP, NULL));	

  g_object_add_weak_pointer (G_OBJECT (app),
			     (gpointer) &app);
  g_object_weak_ref (G_OBJECT (app),
		     app_weak_notify,
		     NULL);

  return app;
}

static gboolean
window_focus_in_event (ProponoWindow *window, 
		       GdkEventFocus *event, 
		       ProponoApp    *app)
{
  /* updates active_view and active_child when a new toplevel receives focus */
  g_return_val_if_fail (PROPONO_IS_WINDOW (window), FALSE);

  app->priv->active_window = window;

  return FALSE;
}

static void
window_destroy (ProponoWindow *window, 
		ProponoApp    *app)
{
  app->priv->windows = g_list_remove (app->priv->windows,
				      window);

  if (window == app->priv->active_window)
    app->priv->active_window = app->priv->windows != NULL ?
			       app->priv->windows->data : NULL;

  if (app->priv->windows == NULL)
    {
      save_accels ();
      g_object_unref (app);
    }
}

ProponoWindow *
propono_app_create_window (ProponoApp *app, GdkScreen *screen)
{
  ProponoWindow *window;

  /*
   * We need to be careful here, there is a race condition:
   * when another propono is launched it checks active_window,
   * so we must do our best to ensure that active_window
   * is never NULL when at least a window exists.
   */
  if (app->priv->windows == NULL)
    app->priv->active_window = window = propono_window_new ();
  else
    window = propono_window_new ();

  app->priv->windows = g_list_prepend (app->priv->windows,
				       window);

  g_signal_connect (window, 
		    "focus_in_event",
		    G_CALLBACK (window_focus_in_event), 
		    app);
  g_signal_connect (window, 
		    "destroy",
		    G_CALLBACK (window_destroy),
		    app);

  if (screen != NULL)
    gtk_window_set_screen (GTK_WINDOW (window), screen);

  return window;
}

const GList *
propono_app_get_windows (ProponoApp *app)
{
  g_return_val_if_fail (PROPONO_IS_APP (app), NULL);

  return app->priv->windows;
}

ProponoWindow *
propono_app_get_active_window (ProponoApp *app)
{
  g_return_val_if_fail (PROPONO_IS_APP (app), NULL);

  /* make sure our active window is always realized:
   * this is needed on startup if we launch two propono fast
   * enough that the second instance comes up before the
   * first one shows its window.
   */

  if (!GTK_WIDGET_REALIZED (GTK_WIDGET (app->priv->active_window)))
    gtk_widget_realize (GTK_WIDGET (app->priv->active_window));

  return app->priv->active_window;
}

static gboolean
is_in_viewport (ProponoWindow *window,
		GdkScreen     *screen,
		gint          workspace,
		gint          viewport_x,
		gint          viewport_y)
{
  GdkScreen *s;
  GdkDisplay *display;
  const gchar *cur_name;
  const gchar *name;
  gint cur_n;
  gint n;
  gint ws;
  gint sc_width, sc_height;
  gint x, y, width, height;
  gint vp_x, vp_y;

  /* Check for screen and display match */
  display = gdk_screen_get_display (screen);
  cur_name = gdk_display_get_name (display);
  cur_n = gdk_screen_get_number (screen);

  s = gtk_window_get_screen (GTK_WINDOW (window));
  display = gdk_screen_get_display (s);
  name = gdk_display_get_name (display);
  n = gdk_screen_get_number (s);

  if (strcmp (cur_name, name) != 0 || cur_n != n)
    return FALSE;

  /* Check for workspace match */
  ws = propono_utils_get_window_workspace (GTK_WINDOW (window));
  if (ws != workspace && ws != PROPONO_ALL_WORKSPACES)
    return FALSE;

  /* Check for viewport match */
  gdk_window_get_position (GTK_WIDGET (window)->window, &x, &y);
  gdk_drawable_get_size (GTK_WIDGET (window)->window, &width, &height);
  propono_utils_get_current_viewport (screen, &vp_x, &vp_y);
  x += vp_x;
  y += vp_y;

  sc_width = gdk_screen_get_width (screen);
  sc_height = gdk_screen_get_height (screen);

  return x + width * .25 >= viewport_x &&
	 x + width * .75 < viewport_x + sc_width &&
	 y  >= viewport_y &&
	 y + height < viewport_y + sc_height;
}

ProponoWindow *
propono_app_get_window_in_viewport (ProponoApp *app,
				    GdkScreen  *screen,
				    gint        workspace,
				    gint        viewport_x,
				    gint        viewport_y)
{
  ProponoWindow *window;
  GList *l;

  g_return_val_if_fail (PROPONO_IS_APP (app), NULL);

  /* first try if the active window */
  window = app->priv->active_window;

  g_return_val_if_fail (PROPONO_IS_WINDOW (window), NULL);

  if (is_in_viewport (window, screen, workspace, viewport_x, viewport_y))
    return window;

  /* otherwise try to see if there is a window on this workspace */
  for (l = app->priv->windows; l != NULL; l = l->next)
    {
      window = l->data;

      if (is_in_viewport (window, screen, workspace, viewport_x, viewport_y))
	return window;
    }

  /* no window on this workspace... create a new one */
  return propono_app_create_window (app, screen);
}

GList *
propono_app_get_connections (ProponoApp *app)
{
  return NULL;
}

/* vim: set ts=8: */
