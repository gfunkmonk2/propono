/*
 * propono-main.c
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

#include <glib.h>
#include <glib/gi18n.h>
#include <gtk/gtk.h>
#include <locale.h>

#include "propono-connection.h"
#include "propono-commands.h"
#include "propono-bookmarks.h"
#include "propono-window.h"
#include "propono-app.h"
#include "propono-utils.h"
#include "propono-prefs.h"
#include "propono-cache-prefs.h"
#include "propono-bacon.h"
#include "propono-plugins-engine.h"
#include "propono-plugin-info.h"
#include "propono-plugin-info-priv.h"
#include "propono-debug.h"
#include "propono-ssh.h"

#ifdef HAVE_TELEPATHY
#include "propono-tubes-manager.h"
#endif

#ifdef PROPONO_ENABLE_AVAHI
#include "propono-mdns.h"
#endif

/* command line */
static gchar **files = NULL;
static gchar **remaining_args = NULL;
static GSList *servers = NULL;
static gboolean new_window = FALSE;
static gboolean fullscreen = FALSE;

static const GOptionEntry options [] =
{
  { "fullscreen", 'f', 0, G_OPTION_ARG_NONE, &fullscreen,
  /* Translators: this is a command line option (run propono --help) */
    N_("Open propono in fullscreen mode"), NULL },

  { "new-window", 'n', 0, G_OPTION_ARG_NONE, &new_window,
  /* Translators: this is a command line option (run propono --help) */
    N_("Create a new toplevel window in an existing instance of propono"), NULL },

  { "file", 'F', 0, G_OPTION_ARG_FILENAME_ARRAY, &files,
  /* Translators: this is a command line option (run propono --help) */
    N_("Open a file recognized by propono"), N_("filename")},

  { 
    G_OPTION_REMAINING, '\0', 0, G_OPTION_ARG_STRING_ARRAY, &remaining_args,
  /* Translators: this is a command line option (run propono --help) */
    NULL, N_("[server:port]") },

  { NULL }
};

static void
propono_main_process_command_line (ProponoWindow *window)
{
  gint               i;
  ProponoConnection *conn;
  gchar             *error;
  GSList            *errors = NULL;

  if (files)
    {
      for (i = 0; files[i]; i++) 
	{
	  conn = propono_connection_new_from_file (files[i], &error, FALSE);
	  if (conn)
	    servers = g_slist_prepend (servers, conn);
	  else
	    {
	      errors = g_slist_prepend (errors,
					g_strdup_printf ("<i>%s</i>: %s",
							files[i],
							error ? error : _("Unknown error")));
	      if (error)
	        g_free (error);
	    }
	}
      g_strfreev (files);
    }

  if (remaining_args)
    {
      for (i = 0; remaining_args[i]; i++) 
	{
	  conn = propono_connection_new_from_string (remaining_args[i], &error, TRUE);
	  if (conn)
	    servers = g_slist_prepend (servers, conn);
	  else
	    errors = g_slist_prepend (errors,
				      g_strdup_printf ("<i>%s</i>: %s",
						       remaining_args[i],
						       error ? error : _("Unknown error")));

	  if (error)
	    g_free (error);
	}

      g_strfreev (remaining_args);
    }

  if (errors)
    {
      propono_utils_show_many_errors (ngettext ("The following error has occurred:",
						"The following errors have occurred:",
						g_slist_length (errors)),
				      errors,
				      window?GTK_WINDOW (window):NULL);
      g_slist_free (errors);
    }
}

int main (int argc, char **argv) {
  GOptionContext       *context;
  GError               *error = NULL;
  GSList               *l, *plugins;
  ProponoWindow        *window;
  ProponoApp           *app;
  ProponoPluginsEngine *engine;
#ifdef HAVE_TELEPATHY
  ProponoTubesManager *propono_tubes_manager;
#endif

  if (!g_thread_supported ())
    g_thread_init (NULL);
  g_type_init();

  /* Setup debugging */
  propono_debug_init ();
  propono_debug_message (DEBUG_APP, "Startup");

  setlocale (LC_ALL, "");
  bindtextdomain (GETTEXT_PACKAGE, PACKAGE_LOCALE_DIR);
  bind_textdomain_codeset (GETTEXT_PACKAGE, "UTF-8");
  textdomain (GETTEXT_PACKAGE);

  /* Init plugins engine */
  propono_debug_message (DEBUG_APP, "Init plugins");
  engine = propono_plugins_engine_get_default ();
  plugins = (GSList *) propono_plugins_engine_get_plugin_list (engine);

  /* Setup command line options */
  context = g_option_context_new (_("- Remote Desktop Viewer"));
  g_option_context_add_main_entries (context, options, GETTEXT_PACKAGE);
  g_option_context_add_group (context, gtk_get_option_group (TRUE));

  for (l = plugins; l; l = l->next)
    {
      GOptionGroup      *group;
      GSList            *groups, *l2;
      ProponoPluginInfo *info = PROPONO_PLUGIN_INFO (l->data);

      if (!propono_plugin_info_is_active (info))
	continue;

      groups = propono_plugin_get_context_groups (info->plugin);
      for (l2 = groups; l2; l2 = l2->next)
	g_option_context_add_group (context, (GOptionGroup *)l2->data);
      g_slist_free (groups);
    }

  g_option_context_parse (context, &argc, &argv, &error);
  g_option_context_free (context);
  if (error)
    {
      g_print ("%s\n%s\n",
	       error->message,
	       _("Run 'propono --help' to see a full list of available command line options"));
      g_error_free (error);
      return 1;
    }

  g_set_application_name (_("Remote Desktop Viewer"));
  propono_main_process_command_line (NULL);

  propono_bacon_start (servers, new_window);

  propono_cache_prefs_init ();
  app = propono_app_get_default ();
  window = propono_app_create_window (app, NULL);
  gtk_widget_show (GTK_WIDGET(window));

  propono_utils_handle_debug ();

  for (l = servers; l; l = l->next)
    {
      ProponoConnection *conn = l->data;

      propono_connection_set_fullscreen (conn, fullscreen);
      propono_cmd_direct_connect (conn, window);
      g_object_unref (conn);
    }
  g_slist_free (servers);

#ifdef HAVE_TELEPATHY
   propono_tubes_manager = propono_tubes_manager_new (window);
#endif

  /* fake call, just to ensure this symbol will be present at propono.so */
  propono_ssh_connect (NULL, NULL, -1, NULL, NULL, NULL, NULL, NULL);

  gtk_main ();

#ifdef HAVE_TELEPATHY
  g_object_unref (propono_tubes_manager);
#endif
  g_object_unref (propono_bookmarks_get_default ());
  g_object_unref (propono_prefs_get_default ());
  propono_cache_prefs_finalize ();
#ifdef PROPONO_ENABLE_AVAHI
  g_object_unref (propono_mdns_get_default ());
#endif

  return 0;
}
/* vim: set ts=8: */
