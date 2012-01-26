/*
 * propono-utils.c
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

#include <string.h>
#include <glib/gi18n.h>
#include "propono-utils.h"

#define PROPONO_UI_FILE  "propono.ui"
#define PROPONO_UI_XML_FILE "propono-ui.xml"

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

/* For the workspace/viewport stuff */
#ifdef GDK_WINDOWING_X11
#include <gdk/gdkx.h>
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/Xatom.h>
#endif

GtkWidget *
propono_utils_create_small_close_button ()
{
  GtkRcStyle *rcstyle;
  GtkWidget *image;
  GtkWidget *close_button;

  close_button = gtk_button_new ();
  gtk_button_set_relief (GTK_BUTTON (close_button),
			 GTK_RELIEF_NONE);
  /* don't allow focus on the close button */
  gtk_button_set_focus_on_click (GTK_BUTTON (close_button), FALSE);

  /* make it as small as possible */
  rcstyle = gtk_rc_style_new ();
  rcstyle->xthickness = rcstyle->ythickness = 0;
  gtk_widget_modify_style (close_button, rcstyle);
  g_object_unref (rcstyle),

  image = gtk_image_new_from_stock (GTK_STOCK_CLOSE,
				    GTK_ICON_SIZE_MENU);
  gtk_container_add (GTK_CONTAINER (close_button), image);

  return close_button;
}

void
propono_utils_show_error (const gchar *title, const gchar *message, GtkWindow *parent)
{
  GtkWidget *d;
  gchar     *t;

  if (title)
    t = g_strdup (title);
  else
    t = g_strdup (_("An error has occurred:"));

  d = gtk_message_dialog_new (parent,
			      GTK_DIALOG_MODAL | GTK_DIALOG_DESTROY_WITH_PARENT,
			      GTK_MESSAGE_ERROR,
			      GTK_BUTTONS_CLOSE,
			      "%s",
			      t);
  g_free (t);

  if (message)
    gtk_message_dialog_format_secondary_markup (GTK_MESSAGE_DIALOG (d),
					      "%s",
					      message);

  g_signal_connect_swapped (d,
			    "response", 
			    G_CALLBACK (gtk_widget_destroy),
			    d);
  gtk_widget_show_all (GTK_WIDGET(d));
}

void
propono_utils_show_many_errors (const gchar *title, GSList *items, GtkWindow *parent)
{
  GString *msg;
  GSList  *l;

  msg = g_string_new (NULL);

  for (l = items; l; l = l->next)
    g_string_append_printf (msg, "%s\n", (gchar *)l->data);

  propono_utils_show_error (title, msg->str, parent);
  g_string_free (msg, TRUE);
}

void
propono_utils_toggle_widget_visible (GtkWidget *widget)
{
  if (GTK_WIDGET_VISIBLE (widget))
    gtk_widget_hide (widget);
  else
    gtk_widget_show_all (widget);
}

const gchar *
propono_utils_get_ui_filename (void)
{
  if (g_file_test (PROPONO_UI_FILE, G_FILE_TEST_EXISTS))
    return PROPONO_UI_FILE;
  else
    return PROPONO_DATADIR "/" PROPONO_UI_FILE;
}

const gchar *
propono_utils_get_ui_xml_filename (void)
{
  if (g_file_test (PROPONO_UI_XML_FILE, G_FILE_TEST_EXISTS))
    return PROPONO_UI_XML_FILE;
  else
    return PROPONO_DATADIR "/" PROPONO_UI_XML_FILE;
}

/**
 * propono_utils_get_builder:
 * @plugin: a #ProponoPlugin or NULL
 * @filename: the filename of the UI file for the plugin, or NULL if plugin is NULL
 *
 * This function gets a #GtkBuilder object for a UI file.
 * Supply a plugin and a filename in order to get a #GtkBuilder for the plugin.
 * Supply NULL to both arguments to get the main Propono #GtkBuilder
 *
 * Returns a #GtkBuilder on success (free with #g_object_unref) or NULL
 *  if the file cannot be found. In this case an error dialog will be shown.
 */
GtkBuilder *
propono_utils_get_builder (ProponoPlugin *plugin, const gchar *filename)
{
  GtkBuilder *xml = NULL;
  GError     *error = NULL;
  gchar      *actual_filename, *plugin_datadir;

  if (plugin)
    {
      plugin_datadir = propono_plugin_get_data_dir (plugin);
      actual_filename = g_build_filename (plugin_datadir, filename, NULL);
      g_free (plugin_datadir);
    }
  else
    actual_filename = g_strdup (propono_utils_get_ui_filename ());

  xml = gtk_builder_new ();
  if (!gtk_builder_add_from_file (xml,
				  actual_filename,
				  &error))
    {
      GString *str = g_string_new (NULL);

      if (plugin)
	g_string_append (str, _("A plugin tried to open an UI file but did not succeed, with the error message:"));
      else
	g_string_append (str, _("The program tried to open an UI file but did not succeed, with the error message:"));

      g_string_append_printf (str, "\n\n%s\n\n", error->message);
      g_string_append (str, _("Please check your installation."));
      propono_utils_show_error (_("Error loading UI file"), str->str, NULL);
      g_error_free (error);
      g_string_free (str, TRUE);
      g_object_unref (xml);
      xml = NULL;
    }

  g_free (actual_filename);
  return xml;
}

/*
 * Doubles underscore to avoid spurious menu accels.
 */
gchar * 
propono_utils_escape_underscores (const gchar* text,
				  gssize       length)
{
	GString *str;
	const gchar *p;
	const gchar *end;

	g_return_val_if_fail (text != NULL, NULL);

	if (length < 0)
		length = strlen (text);

	str = g_string_sized_new (length);

	p = text;
	end = text + length;

	while (p != end)
	{
		const gchar *next;
		next = g_utf8_next_char (p);

		switch (*p)
		{
			case '_':
				g_string_append (str, "__");
				break;
			default:
				g_string_append_len (str, p, next - p);
				break;
		}

		p = next;
	}

	return g_string_free (str, FALSE);
}

static void _default_log (const gchar *log_domain G_GNUC_UNUSED,
			 GLogLevelFlags log_level G_GNUC_UNUSED,
			 const gchar *message,
			 gpointer user_data G_GNUC_UNUSED)
{
  printf ("gtk-vnc: %s\n", message);
}

void
propono_utils_handle_debug (void)
{
  static gboolean initialized = FALSE;

  if (initialized)
    return;

  g_log_set_handler ("gtk-vnc",
		     G_LOG_LEVEL_DEBUG,
		     _default_log,
		     NULL);

  initialized = TRUE;
}

/* the following two functions are courtesy of galeon */

/**
 * pluma_utils_get_current_workspace: Get the current workspace
 *
 * Get the currently visible workspace for the #GdkScreen.
 *
 * If the X11 window property isn't found, 0 (the first workspace)
 * is returned.
 */
guint
propono_utils_get_current_workspace (GdkScreen *screen)
{
#ifdef GDK_WINDOWING_X11
	GdkWindow *root_win;
	GdkDisplay *display;
	Atom type;
	gint format;
	gulong nitems;
	gulong bytes_after;
	guint *current_desktop;
	gint err, result;
	guint ret = 0;

	g_return_val_if_fail (GDK_IS_SCREEN (screen), 0);

	root_win = gdk_screen_get_root_window (screen);
	display = gdk_screen_get_display (screen);

	gdk_error_trap_push ();
	result = XGetWindowProperty (GDK_DISPLAY_XDISPLAY (display), GDK_WINDOW_XID (root_win),
				     gdk_x11_get_xatom_by_name_for_display (display, "_NET_CURRENT_DESKTOP"),
				     0, G_MAXLONG, False, XA_CARDINAL, &type, &format, &nitems,
				     &bytes_after, (gpointer) &current_desktop);
	err = gdk_error_trap_pop ();

	if (err != Success || result != Success)
		return ret;

	if (type == XA_CARDINAL && format == 32 && nitems > 0)
		ret = current_desktop[0];

	XFree (current_desktop);
	return ret;
#else
	/* FIXME: on mac etc proably there are native APIs
	 * to get the current workspace etc */
	return 0;
#endif
}

/**
 * pluma_utils_get_window_workspace: Get the workspace the window is on
 *
 * This function gets the workspace that the #GtkWindow is visible on,
 * it returns PLUMA_ALL_WORKSPACES if the window is sticky, or if
 * the window manager doesn support this function
 */
guint
propono_utils_get_window_workspace (GtkWindow *gtkwindow)
{
#ifdef GDK_WINDOWING_X11
	GdkWindow *window;
	GdkDisplay *display;
	Atom type;
	gint format;
	gulong nitems;
	gulong bytes_after;
	guint *workspace;
	gint err, result;
	guint ret = PROPONO_ALL_WORKSPACES;

	g_return_val_if_fail (GTK_IS_WINDOW (gtkwindow), 0);
	g_return_val_if_fail (GTK_WIDGET_REALIZED (GTK_WIDGET (gtkwindow)), 0);

	window = GTK_WIDGET (gtkwindow)->window;
	display = gdk_drawable_get_display (window);

	gdk_error_trap_push ();
	result = XGetWindowProperty (GDK_DISPLAY_XDISPLAY (display), GDK_WINDOW_XID (window),
				     gdk_x11_get_xatom_by_name_for_display (display, "_NET_WM_DESKTOP"),
				     0, G_MAXLONG, False, XA_CARDINAL, &type, &format, &nitems,
				     &bytes_after, (gpointer) &workspace);
	err = gdk_error_trap_pop ();

	if (err != Success || result != Success)
		return ret;

	if (type == XA_CARDINAL && format == 32 && nitems > 0)
		ret = workspace[0];

	XFree (workspace);
	return ret;
#else
	/* FIXME: on mac etc proably there are native APIs
	 * to get the current workspace etc */
	return 0;
#endif
}

/**
 * pluma_utils_get_current_viewport: Get the current viewport origin
 *
 * Get the currently visible viewport origin for the #GdkScreen.
 *
 * If the X11 window property isn't found, (0, 0) is returned.
 */
void
propono_utils_get_current_viewport (GdkScreen    *screen,
				  gint         *x,
				  gint         *y)
{
#ifdef GDK_WINDOWING_X11
	GdkWindow *root_win;
	GdkDisplay *display;
	Atom type;
	gint format;
	gulong nitems;
	gulong bytes_after;
	gulong *coordinates;
	gint err, result;

	g_return_if_fail (GDK_IS_SCREEN (screen));
	g_return_if_fail (x != NULL && y != NULL);

	/* Default values for the viewport origin */
	*x = 0;
	*y = 0;

	root_win = gdk_screen_get_root_window (screen);
	display = gdk_screen_get_display (screen);

	gdk_error_trap_push ();
	result = XGetWindowProperty (GDK_DISPLAY_XDISPLAY (display), GDK_WINDOW_XID (root_win),
				     gdk_x11_get_xatom_by_name_for_display (display, "_NET_DESKTOP_VIEWPORT"),
				     0, G_MAXLONG, False, XA_CARDINAL, &type, &format, &nitems,
				     &bytes_after, (void*) &coordinates);
	err = gdk_error_trap_pop ();

	if (err != Success || result != Success)
		return;

	if (type != XA_CARDINAL || format != 32 || nitems < 2)
	{
		XFree (coordinates);
		return;
	}

	*x = coordinates[0];
	*y = coordinates[1];
	XFree (coordinates);
#else
	/* FIXME: on mac etc proably there are native APIs
	 * to get the current workspace etc */
	*x = 0;
	*y = 0;
#endif
}

/* Make url in about dialog clickable */
static void
propono_about_dialog_handle_url (GtkAboutDialog *about,
				 const char     *link,
				 gpointer        data)
{
  GError    *error = NULL;
  gchar     *address;
  GdkScreen *screen;

  if (g_strstr_len (link, strlen (link), "@"))
    address = g_strdup_printf ("mailto:%s", link);
  else
    address = g_strdup (link);

  screen = GTK_IS_WINDOW (data) ? gtk_window_get_screen (GTK_WINDOW (data)) : NULL;

  gtk_show_uri (screen,
		address,
		GDK_CURRENT_TIME,
		&error);

  if (error != NULL) 
    {
      propono_utils_show_error (NULL, error->message, GTK_IS_WINDOW (data) ? GTK_WINDOW (data) : NULL);
      g_error_free (error);
    }

  g_free (address);
}

void
propono_utils_help_contents (GtkWindow *window, const gchar *section)
{
  GError    *error;
  GdkScreen *screen;
  gchar     *uri;

  screen = GTK_IS_WINDOW (window) ? gtk_window_get_screen (GTK_WINDOW (window)) : NULL;
  error = NULL;
  if (section)
    uri = g_strdup_printf ("ghelp:propono?%s", section);
  else
    uri = g_strdup ("ghelp:propono");

  gtk_show_uri (screen,
		uri,
		GDK_CURRENT_TIME,
		&error);

  g_free (uri);
  if (error != NULL) 
    {
      propono_utils_show_error (NULL, error->message, GTK_IS_WINDOW (window) ? window : NULL);
      g_error_free (error);
    }
}

void
propono_utils_help_about (GtkWindow *window)
{
  static const gchar * const authors[] = {
	"Jonh Wendell <jwendell@mate.org>",
	NULL
  };

  static const gchar * const artists[] = {
	"Vinicius Depizzol <vdepizzol@gmail.com>",
	NULL
  };

  static const gchar copyright[] = \
	"Copyright \xc2\xa9 2007-2009 Jonh Wendell";

  static const gchar comments[] = \
	N_("Propono is a remote desktop viewer for the MATE Desktop");

  static const char *license[] = {
	N_("Propono is free software; you can redistribute it and/or modify "
	   "it under the terms of the GNU General Public License as published by "
	   "the Free Software Foundation; either version 2 of the License, or "
	   "(at your option) any later version."),
	N_("Propono is distributed in the hope that it will be useful, "
	   "but WITHOUT ANY WARRANTY; without even the implied warranty of "
	   "MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the "
	   "GNU General Public License for more details."),
	N_("You should have received a copy of the GNU General Public License "
	   "along with this program. If not, see <http://www.gnu.org/licenses/>.")
  };

  gchar *license_trans;

  license_trans = g_strjoin ("\n\n", _(license[0]), _(license[1]),
				     _(license[2]), NULL);

  /* Make URLs and email clickable in about dialog */
  gtk_about_dialog_set_url_hook (propono_about_dialog_handle_url, window, NULL);
  gtk_about_dialog_set_email_hook (propono_about_dialog_handle_url, window, NULL);


  gtk_show_about_dialog (GTK_IS_WINDOW (window)?window:NULL,
			 "authors", authors,
			 "artists", artists,
			 "comments", _(comments),
			 "copyright", copyright,
			 "license", license_trans,
			 "wrap-license", TRUE,
			 "logo-icon-name", "propono",
			 "translator-credits", _("translator-credits"),
			 "version", VERSION,
			 "website", "http://projects.mate.org/propono/",
			 "website-label", _("Propono Website"),
			 NULL);
  g_free (license_trans);
}

gboolean
propono_utils_parse_boolean (const gchar* value)
{
  if (g_ascii_strcasecmp (value, "true") == 0 || strcmp (value, "1") == 0)
    return TRUE;

  return FALSE;
}

GtkWidget *
propono_gtk_button_new_with_stock_icon (const gchar *label,
                                        const gchar *stock_id)
{
  GtkWidget *button;

  button = gtk_button_new_with_mnemonic (label);
  gtk_button_set_image (GTK_BUTTON (button),
            gtk_image_new_from_stock (stock_id,
              GTK_ICON_SIZE_BUTTON));

        return button;
}

/**
 * propono_utils_ask_question:
 * @parent: transient parent, or NULL for none
 * @message: The message to be displayed, if it contains multiple lines,
 *  the first one is considered as the title.
 * @choices: NULL-terminated array of button's labels of the dialog
 * @choice: Place to store the selected button. Zero is the first.
 *
 * Displays a dialog with a message and some options to the user.
 *
 * Returns TRUE if the user has selected any option, FALSE if the dialog
 *  was canceled.
 */
gboolean
propono_utils_ask_question (GtkWindow  *parent,
			    const char *message,
			    char       **choices,
			    int        *choice)
{
  gchar **messages;
  GtkWidget *d;
  int i, n_choices, result;

  g_return_val_if_fail (message && choices && choice, FALSE);

  messages = g_strsplit (message, "\n", 2);

  d = gtk_message_dialog_new (parent,
			      GTK_DIALOG_MODAL | GTK_DIALOG_DESTROY_WITH_PARENT,
			      GTK_MESSAGE_QUESTION,
			      GTK_BUTTONS_NONE,
			      "%s",
			      messages[0]);

  if (g_strv_length (messages) > 1)
    gtk_message_dialog_format_secondary_markup (GTK_MESSAGE_DIALOG (d),
						"%s",
						messages[1]);
  g_strfreev (messages);

  n_choices = g_strv_length (choices);
  for (i = 0; i < n_choices; i++)
    gtk_dialog_add_button (GTK_DIALOG (d), choices[i], i);

  result = gtk_dialog_run (GTK_DIALOG (d));
  gtk_widget_destroy (d);

  if (result == GTK_RESPONSE_NONE || result == GTK_RESPONSE_DELETE_EVENT)
    return FALSE;

  *choice = result;
  return TRUE;
}

typedef struct {
  GtkWidget *uname, *pw, *button;
} ControlOKButton;

static void
control_ok_button (GtkEditable *entry, ControlOKButton *data)
{
  gboolean enabled = TRUE;

  if (GTK_WIDGET_VISIBLE (data->uname))
    enabled = enabled && gtk_entry_get_text_length (GTK_ENTRY (data->uname)) > 0;

  if (GTK_WIDGET_VISIBLE (data->pw))
    enabled = enabled && gtk_entry_get_text_length (GTK_ENTRY (data->pw)) > 0;

  gtk_widget_set_sensitive (data->button, enabled);
}

gboolean
propono_utils_ask_credential (GtkWindow *parent,
			      gchar *kind,
			      gchar *host,
			      gboolean need_username,
			      gboolean need_password,
			      gint password_limit,
			      gchar **username,
			      gchar **password,
			      gboolean *save_in_keyring)
{
  GtkBuilder      *xml;
  GtkWidget       *password_dialog, *save_credential_check;
  GtkWidget       *password_label, *username_label, *image;
  int             result;
  ControlOKButton control;

  xml = propono_utils_get_builder (NULL, NULL);

  password_dialog = GTK_WIDGET (gtk_builder_get_object (xml, "auth_required_dialog"));
  if (parent)
    gtk_window_set_transient_for (GTK_WINDOW (password_dialog), parent);

  if (kind)
    {
       /* Translators: %s is a protocol, like VNC or SSH */
       gchar *str = g_strdup_printf (_("%s authentication is required"), kind);
       GtkWidget *auth_label = GTK_WIDGET (gtk_builder_get_object (xml, "auth_required_label"));
       gtk_label_set_label (GTK_LABEL (auth_label), str);
       g_free (str);
    }

  if (host)
    {
       GtkWidget *host_label = GTK_WIDGET (gtk_builder_get_object (xml, "host_label"));
       gtk_label_set_label (GTK_LABEL (host_label), host);
    }

  control.uname  = GTK_WIDGET (gtk_builder_get_object (xml, "username_entry"));
  control.pw     = GTK_WIDGET (gtk_builder_get_object (xml, "password_entry"));
  control.button = GTK_WIDGET (gtk_builder_get_object (xml, "ok_button"));
  password_label = GTK_WIDGET (gtk_builder_get_object (xml, "password_label"));
  username_label = GTK_WIDGET (gtk_builder_get_object (xml, "username_label"));
  save_credential_check = GTK_WIDGET (gtk_builder_get_object (xml, "save_credential_check"));

  image = gtk_image_new_from_stock (GTK_STOCK_DIALOG_AUTHENTICATION, GTK_ICON_SIZE_BUTTON);
  gtk_button_set_image (GTK_BUTTON (control.button), image);

  g_signal_connect (control.uname, "changed", G_CALLBACK (control_ok_button), &control);
  g_signal_connect (control.pw, "changed", G_CALLBACK (control_ok_button), &control);

  if (need_username)
    {
      if (username && *username)
        gtk_entry_set_text (GTK_ENTRY (control.uname), *username);
    }
  else
    {
      gtk_widget_hide (username_label);
      gtk_widget_hide (control.uname);
    }

  if (need_password)
    {
      gtk_entry_set_max_length (GTK_ENTRY (control.pw), password_limit);
      if (password && *password)
        gtk_entry_set_text (GTK_ENTRY (control.pw), *password);
    }
  else
    {
      gtk_widget_hide (password_label);
      gtk_widget_hide (control.pw);
    }

  result = gtk_dialog_run (GTK_DIALOG (password_dialog));
  if (result == -5)
    {
      if (username)
        {
          g_free (*username);
          if (gtk_entry_get_text_length (GTK_ENTRY (control.uname)) > 0)
	    *username = g_strdup (gtk_entry_get_text (GTK_ENTRY (control.uname)));
          else
	    *username = NULL;
	}

      if (password)
        {
          g_free (*password);
          if (gtk_entry_get_text_length (GTK_ENTRY (control.pw)) > 0)
	    *password = g_strdup (gtk_entry_get_text (GTK_ENTRY (control.pw)));
          else
	    *password = NULL;
	}

      if (save_in_keyring)
        *save_in_keyring = gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (save_credential_check));
    }

  gtk_widget_destroy (GTK_WIDGET (password_dialog));
  g_object_unref (xml);

  return result == -5;
}

gboolean
propono_utils_create_dir (const gchar *filename, GError **error)
{
  GFile    *file, *parent;
  gboolean result;
  gchar    *path;

  file   = g_file_new_for_path (filename);
  parent = g_file_get_parent (file);
  path   = g_file_get_path (parent);
  result = TRUE;

  if (!g_file_test (path, G_FILE_TEST_EXISTS))
    result = g_file_make_directory_with_parents (parent, NULL, error);

  g_object_unref (file);
  g_object_unref (parent);
  g_free (path);
  return result;
}

/* vim: set ts=8: */
