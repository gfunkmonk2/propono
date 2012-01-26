/*
 * propono-ui.h
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

#ifndef __PROPONO_UI_H__
#define __PROPONO_UI_H__

#include <gtk/gtk.h>

#include "propono-commands.h"

G_BEGIN_DECLS

static const GtkActionEntry propono_always_sensitive_entries[] =
{
  /* Toplevel */
  { "Machine", NULL, N_("_Machine") },
  { "Edit", NULL, N_("_Edit") },
  { "View", NULL, N_("_View") },
  { "Bookmarks", NULL, N_("_Bookmarks") },
  { "Help", NULL, N_("_Help") },

  /* Machine menu */
  { "MachineConnect", GTK_STOCK_CONNECT, NULL, "<control>N",
    N_("Connect to a remote machine"), G_CALLBACK (propono_cmd_machine_connect) },
  { "MachineOpen", GTK_STOCK_OPEN, NULL, "<control>O",
    N_("Open a .VNC file"), G_CALLBACK (propono_cmd_machine_open) },
  { "MachineQuit", GTK_STOCK_QUIT, NULL, "<control>Q",
    N_("Quit the program"), G_CALLBACK (propono_cmd_machine_quit) },

  /* Edit menu */
  { "EditPreferences", GTK_STOCK_PREFERENCES, NULL, NULL,
    N_("Edit the application preferences"), G_CALLBACK (propono_cmd_edit_preferences) },
  { "EditPlugins", GTK_STOCK_EXECUTE, N_("_Plugins"), NULL,
    N_("Select plugins"), G_CALLBACK (propono_cmd_edit_plugins) },

  /* Help menu */
  {"HelpContents", GTK_STOCK_HELP, N_("_Contents"), "F1",
    N_("Open the propono manual"),  G_CALLBACK (propono_cmd_help_contents)},
  { "HelpAbout", GTK_STOCK_ABOUT, NULL, NULL,
    N_("About this application"), G_CALLBACK (propono_cmd_help_about) }
};

static const GtkToggleActionEntry propono_always_sensitive_toggle_entries[] =
{
  { "ViewToolbar", NULL, N_("_Toolbar"), NULL,
    N_("Show or hide the toolbar"),
    G_CALLBACK (propono_cmd_view_show_toolbar), FALSE },

  { "ViewStatusbar", NULL, N_("_Statusbar"), NULL,
    N_("Show or hide the statusbar"),
    G_CALLBACK (propono_cmd_view_show_statusbar), FALSE },

  { "ViewSidePanel", NULL, N_("Side _Panel"), "F9",
    N_("Show or hide the side panel"),
    G_CALLBACK (propono_cmd_view_show_fav_panel), FALSE }
};

static const GtkActionEntry propono_machine_connected_entries[] =
{
  /* Machine menu */
  { "MachineClose", GTK_STOCK_CLOSE, NULL, "<control>W",
    N_("Close the current connection"), G_CALLBACK (propono_cmd_machine_close) },
  { "MachineCloseAll", GTK_STOCK_CLOSE, N_("C_lose All"), "<control><shift>W",
    N_("Close all active connections"), G_CALLBACK (propono_cmd_machine_close_all) },

  /* Bookmarks menu */
  { "BookmarksAdd", GTK_STOCK_SAVE, N_("_Add to bookmarks"), "<control>D",
    N_("Add current connection to your bookmarks"), G_CALLBACK (propono_cmd_bookmarks_add) }

};

static const GtkActionEntry propono_machine_initialized_entries[] =
{
  /* Machine menu */
  { "MachineTakeScreenshot", "applets-screenshooter", N_("Take screenshot"), NULL,
    N_("Take a screenshot of active connection"), G_CALLBACK (propono_cmd_machine_take_screenshot) },

  /* View menu */
  { "ViewFullScreen", GTK_STOCK_FULLSCREEN, NULL, "F11",
    N_("View the current machine in full screen"), G_CALLBACK (propono_cmd_view_fullscreen) }
};

static const GtkToggleActionEntry propono_machine_initialized_toggle_entries[] =
{
 0,
};

G_END_DECLS

#endif  /* __PROPONO_UI_H__  */
/* vim: set ts=8: */
