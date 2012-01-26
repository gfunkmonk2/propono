/*
 * propono-vnc-tab.c
 * VNC Implementation for ProponoVncTab widget
 * This file is part of propono
 *
 * Copyright (C) 2009 - Jonh Wendell <wendell@bani.com.br>
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

#include <glib/gi18n.h>
#include <vncdisplay.h>
#include <gdk/gdkkeysyms.h>

#include <propono/propono-utils.h>
#include <propono/propono-prefs.h>

#include "propono-vnc-tab.h"
#include "propono-vnc-connection.h"
#include "propono-vnc-tunnel.h"

#define PROPONO_VNC_TAB_GET_PRIVATE(object)(G_TYPE_INSTANCE_GET_PRIVATE ((object), PROPONO_TYPE_VNC_TAB, ProponoVncTabPrivate))

struct _ProponoVncTabPrivate
{
  GtkWidget  *vnc, *align;
  gboolean   pointer_grab;
  gchar      *clipboard_str;
  GSList     *connected_actions, *initialized_actions;
  GtkWidget  *viewonly_button, *scaling_button;
  GtkAction  *scaling_action, *viewonly_action, *original_size_action, *keep_ratio_action;
  gulong     signal_clipboard, signal_align;
};

G_DEFINE_TYPE (ProponoVncTab, propono_vnc_tab, PROPONO_TYPE_TAB)

/* Properties */
enum
{
  PROP_0,
  PROP_ORIGINAL_WIDTH,
  PROP_ORIGINAL_HEIGHT
};

static void open_vnc (ProponoVncTab *vnc_tab);
static void setup_toolbar (ProponoVncTab *vnc_tab);

static void
propono_vnc_tab_get_property (GObject    *object,
			  guint       prop_id,
			  GValue     *value,
			  GParamSpec *pspec)
{
  ProponoVncTab *tab = PROPONO_VNC_TAB (object);

  switch (prop_id)
    {
      case PROP_ORIGINAL_WIDTH:
        g_value_set_int (value, propono_vnc_tab_get_original_width (tab));
	break;
      case PROP_ORIGINAL_HEIGHT:
        g_value_set_int (value, propono_vnc_tab_get_original_height (tab));
	break;
      default:
	G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
	break;			
    }
}

static void
view_scaling_cb (GtkAction *action, ProponoVncTab *vnc_tab)
{
  propono_vnc_tab_set_scaling (vnc_tab,
			       gtk_toggle_action_get_active (GTK_TOGGLE_ACTION (action)));
}

static void
view_keep_ratio_cb (GtkAction *action, ProponoVncTab *vnc_tab)
{
  propono_vnc_tab_set_keep_ratio (vnc_tab,
				  gtk_toggle_action_get_active (GTK_TOGGLE_ACTION (action)));
}

static void
view_viewonly_cb (GtkAction *action, ProponoVncTab *vnc_tab)
{
  propono_vnc_tab_set_viewonly (vnc_tab,
				gtk_toggle_action_get_active (GTK_TOGGLE_ACTION (action)));
}

static void
send_ctrlaltdel_cb (GtkAction *action, ProponoVncTab *vnc_tab)
{
  propono_vnc_tab_send_ctrlaltdel (vnc_tab);
}

static void
view_original_size_cb (GtkAction *action, ProponoVncTab *vnc_tab)
{
  propono_vnc_tab_original_size (vnc_tab);
}

static void
view_refresh_cb (GtkAction *action, ProponoVncTab *vnc_tab)
{
  vnc_display_request_update (VNC_DISPLAY (vnc_tab->priv->vnc));
}

const static GSList *
vnc_get_connected_actions (ProponoTab *tab)
{
  ProponoVncTab *vnc_tab = PROPONO_VNC_TAB (tab);

  return vnc_tab->priv->connected_actions;
}

const static GSList *
vnc_get_initialized_actions (ProponoTab *tab)
{
  ProponoVncTab *vnc_tab = PROPONO_VNC_TAB (tab);

  return vnc_tab->priv->initialized_actions;
}

static gchar *
vnc_tab_get_tooltip (ProponoTab *tab)
{
  ProponoVncTab *vnc_tab = PROPONO_VNC_TAB (tab);
  ProponoConnection *conn = propono_tab_get_conn (tab);

  return  g_markup_printf_escaped (
				  "<b>%s</b> %s\n\n"
				  "<b>%s</b> %s\n"
				  "<b>%s</b> %d\n"
				  "<b>%s</b> %dx%d",
				  _("Desktop Name:"), vnc_display_get_name (VNC_DISPLAY (vnc_tab->priv->vnc)),
				  _("Host:"), propono_connection_get_host (conn),
				  _("Port:"), propono_connection_get_port (conn),
				  _("Dimensions:"), vnc_display_get_width (VNC_DISPLAY (vnc_tab->priv->vnc)), vnc_display_get_height (VNC_DISPLAY (vnc_tab->priv->vnc)));
}

static GdkPixbuf *
vnc_tab_get_screenshot (ProponoTab *tab)
{
  ProponoVncTab *vnc_tab = PROPONO_VNC_TAB (tab);

  return vnc_display_get_pixbuf (VNC_DISPLAY (vnc_tab->priv->vnc));
}

static void
propono_vnc_tab_finalize (GObject *object)
{
  ProponoVncTab *vnc_tab = PROPONO_VNC_TAB (object);

  g_free (vnc_tab->priv->clipboard_str);

  G_OBJECT_CLASS (propono_vnc_tab_parent_class)->finalize (object);
}

static void
propono_vnc_tab_dispose (GObject *object)
{
  ProponoVncTab *vnc_tab = PROPONO_VNC_TAB (object);

  if (vnc_tab->priv->connected_actions)
    {
      propono_tab_free_actions (vnc_tab->priv->connected_actions);
      vnc_tab->priv->connected_actions = NULL;
    }

  if (vnc_tab->priv->initialized_actions)
    {
      propono_tab_free_actions (vnc_tab->priv->initialized_actions);
      vnc_tab->priv->initialized_actions = NULL;
    }

  if (vnc_tab->priv->signal_clipboard != 0)
    {
      GtkClipboard  *cb = gtk_clipboard_get (GDK_SELECTION_CLIPBOARD);

      g_signal_handler_disconnect (cb,
				   vnc_tab->priv->signal_clipboard);
      vnc_tab->priv->signal_clipboard = 0;
    }

  G_OBJECT_CLASS (propono_vnc_tab_parent_class)->dispose (object);
}

static void
propono_vnc_tab_constructed (GObject *object)
{
  ProponoVncTab *vnc_tab = PROPONO_VNC_TAB (object);

  if (G_OBJECT_CLASS (propono_vnc_tab_parent_class)->constructed)
    G_OBJECT_CLASS (propono_vnc_tab_parent_class)->constructed (object);

  setup_toolbar (vnc_tab);
  open_vnc (vnc_tab);
}

static void 
propono_vnc_tab_class_init (ProponoVncTabClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);
  ProponoTabClass* tab_class = PROPONO_TAB_CLASS (klass);

  object_class->finalize = propono_vnc_tab_finalize;
  object_class->dispose = propono_vnc_tab_dispose;
  object_class->get_property = propono_vnc_tab_get_property;
  object_class->constructed = propono_vnc_tab_constructed;

  tab_class->impl_get_tooltip = vnc_tab_get_tooltip;
  tab_class->impl_get_connected_actions = vnc_get_connected_actions;
  tab_class->impl_get_initialized_actions = vnc_get_initialized_actions;
  tab_class->impl_get_screenshot = vnc_tab_get_screenshot;

  g_object_class_install_property (object_class,
				   PROP_ORIGINAL_WIDTH,
				   g_param_spec_int ("original-width",
						     "Original width",
						     "The original width of the remote screen",
						     -1, G_MAXINT, 0,
						      G_PARAM_READABLE |
						      G_PARAM_STATIC_NAME |
						      G_PARAM_STATIC_NICK |
						      G_PARAM_STATIC_BLURB));

  g_object_class_install_property (object_class,
				   PROP_ORIGINAL_HEIGHT,
				   g_param_spec_int ("original-height",
						     "Original height",
						     "The original height of the remote screen",
						     -1, G_MAXINT, 0,
						      G_PARAM_READABLE |
						      G_PARAM_STATIC_NAME |
						      G_PARAM_STATIC_NICK |
						      G_PARAM_STATIC_BLURB));

  g_type_class_add_private (object_class, sizeof (ProponoVncTabPrivate));
}

static gboolean
idle_close (ProponoTab *tab)
{
  propono_notebook_close_tab (propono_tab_get_notebook (tab), tab);
  return FALSE;
}

static void
open_vnc (ProponoVncTab *vnc_tab)
{
  gchar      *host, *port_str, *ssh_tunnel_host;
  gint       port, shared, fd, depth_profile;
  gboolean   scaling, success, lossy_encoding;
  GError     *error;
  VncDisplay *vnc = VNC_DISPLAY (vnc_tab->priv->vnc);
  ProponoTab *tab = PROPONO_TAB (vnc_tab);
  GtkWindow  *window = GTK_WINDOW (propono_tab_get_window (tab));

  success = TRUE;
  error = NULL;

  g_object_get (propono_tab_get_conn (tab),
		"port", &port,
		"host", &host,
		"scaling", &scaling,
		"shared", &shared,
		"fd", &fd,
		"depth-profile", &depth_profile,
		"lossy-encoding", &lossy_encoding,
		"ssh-tunnel-host", &ssh_tunnel_host,
		NULL);

  port_str = g_strdup_printf ("%d", port);
  if (shared == -1)
    g_object_get (propono_prefs_get_default (),
		  "shared-flag", &shared,
		  NULL);

  vnc_display_set_shared_flag (vnc, shared);
  vnc_display_set_force_size (vnc, !scaling);
  vnc_display_set_depth (vnc, depth_profile);
  vnc_display_set_lossy_encoding (vnc, lossy_encoding);

  if (fd > 0)
    success = vnc_display_open_fd (vnc, fd);
  else
    {
      if (ssh_tunnel_host && *ssh_tunnel_host)
	if (!propono_vnc_tunnel_create (window, &host, &port_str, ssh_tunnel_host, &error))
	  {
	    success = FALSE;
	    propono_utils_show_error (_("Error creating the SSH tunnel"),
				      error ? error->message : _("Unknown reason"),
				      window);
	    goto out;
	  }
      success = vnc_display_open_host (vnc, host, port_str);
    }

  if (success)
    gtk_widget_grab_focus (GTK_WIDGET (vnc));
  else
    propono_utils_show_error (_("Error connecting to host."),
			      error ? error->message : _("Unknown reason"),
			      window);

out:
  g_free (port_str);
  g_free (host);
  g_free (ssh_tunnel_host);
  g_clear_error (&error);

  if (!success)
    g_idle_add ((GSourceFunc)idle_close, vnc_tab);
}

static void
vnc_connected_cb (VncDisplay *vnc, ProponoVncTab *tab)
{
  g_signal_emit_by_name (G_OBJECT (tab), "tab-connected");
}

static void
vnc_disconnected_cb (VncDisplay *vnc, ProponoVncTab *tab)
{
  g_signal_emit_by_name (G_OBJECT (tab), "tab-disconnected");
}

static void
vnc_auth_failed_cb (VncDisplay *vnc, const gchar *msg, ProponoVncTab *vnc_tab)
{
  propono_tab_remove_credentials_from_keyring (PROPONO_TAB (vnc_tab));
  g_signal_emit_by_name (vnc_tab, "tab-auth-failed", msg);
}

static void
vnc_auth_unsupported_cb (VncDisplay *vnc, guint auth_type, ProponoVncTab *vnc_tab)
{
  GString *message;
  gchar   *name, *emphasis;
  ProponoTab *tab = PROPONO_TAB (vnc_tab);

  message = g_string_new (NULL);
  name = propono_connection_get_best_name (propono_tab_get_conn (tab));
  emphasis = g_strdup_printf ("<i>%s</i>", name);

  /* Translators: %s is a host name or IP address; %u is a code error (number). Also, the right sentence would be ... method for host... */
  g_string_printf (message, _("Authentication method to host %s is unsupported. (%u)"),
		   emphasis,
		   auth_type);

  propono_utils_show_error (_("Authentication unsupported"),
			    message->str,
			    GTK_WINDOW (propono_tab_get_window (tab)));
  g_string_free (message, TRUE);
  g_free (name);
  g_free (emphasis);

  propono_tab_remove_from_notebook (tab);
}

/* text was actually requested */
static void
copy_cb (GtkClipboard     *clipboard,
         GtkSelectionData *data,
	 guint             info,
	 ProponoVncTab    *vnc_tab)
{
  gtk_selection_data_set_text (data, vnc_tab->priv->clipboard_str, -1);
}

static void
vnc_server_cut_text_cb (VncDisplay *vnc, const gchar *text, ProponoVncTab *vnc_tab)
{
  GtkClipboard *cb;
  gsize a, b;
  GtkTargetEntry targets[] = {
				{"UTF8_STRING", 0, 0},
				{"COMPOUND_TEXT", 0, 0},
				{"TEXT", 0, 0},
				{"STRING", 0, 0},
			     };

  if (!text)
    return;

  g_free (vnc_tab->priv->clipboard_str);
  vnc_tab->priv->clipboard_str = g_convert (text, -1, "utf-8", "iso8859-1", &a, &b, NULL);

  if (vnc_tab->priv->clipboard_str)
    {
      cb = gtk_clipboard_get (GDK_SELECTION_CLIPBOARD);

      gtk_clipboard_set_with_owner (cb,
				    targets,
				    G_N_ELEMENTS(targets),
				    (GtkClipboardGetFunc) copy_cb,
				    NULL,
				    G_OBJECT (vnc_tab));
    }
}

static void
vnc_initialized_cb (VncDisplay *vnc, ProponoVncTab *vnc_tab)
{
  GtkLabel *label;
  gchar    *name;
  gboolean scaling, view_only, fullscreen, keep_ratio;
  ProponoTab *tab = PROPONO_TAB (vnc_tab);
  ProponoConnection *conn = propono_tab_get_conn (tab);

  g_object_get (conn,
		"view-only", &view_only,
		"scaling", &scaling,
		"keep_ratio", &keep_ratio,
		"fullscreen", &fullscreen,
		NULL);

  gtk_toggle_action_set_active (GTK_TOGGLE_ACTION (vnc_tab->priv->scaling_action), scaling);
  gtk_toggle_action_set_active (GTK_TOGGLE_ACTION (vnc_tab->priv->keep_ratio_action), keep_ratio);
  gtk_toggle_action_set_active (GTK_TOGGLE_ACTION (vnc_tab->priv->viewonly_action), view_only);
  vnc_display_set_pointer_local (vnc, TRUE);
  vnc_display_set_keyboard_grab (vnc, TRUE);
  vnc_display_set_pointer_grab (vnc, TRUE);

  propono_vnc_connection_set_desktop_name (PROPONO_VNC_CONNECTION (conn),
					   vnc_display_get_name (vnc));

  name = propono_connection_get_best_name (conn);
  label = g_object_get_data (G_OBJECT (tab), "label");
  g_return_if_fail (label != NULL);
  gtk_label_set_label (label, name);
  g_free (name);

  propono_tab_save_credentials_in_keyring (tab);
  propono_tab_add_recent_used (tab);
  propono_tab_set_state (tab, PROPONO_TAB_STATE_CONNECTED);

  g_signal_emit_by_name (G_OBJECT (tab), "tab-initialized");
}

static void
vnc_authentication_cb (VncDisplay *vnc, GValueArray *credList, ProponoVncTab *vnc_tab)
{
  gchar *username, *password, *host;
  gboolean need_password, need_username, save_in_keyring;
  int i;
  ProponoTab *tab = PROPONO_TAB (vnc_tab);
  ProponoConnection *conn = propono_tab_get_conn (tab);
  GtkWindow *window = GTK_WINDOW (propono_tab_get_window (tab));

  if (credList == NULL)
    return;

  need_password = FALSE;
  need_username = FALSE;
  save_in_keyring = FALSE;
  username = NULL;
  password = NULL;
  host = NULL;

  for (i = 0; i < credList->n_values; i++) {
    switch (g_value_get_enum (&credList->values[i]))
      {
	case VNC_DISPLAY_CREDENTIAL_USERNAME:
	  if (propono_connection_get_username (conn))
	    {
	      vnc_display_set_credential (vnc, VNC_DISPLAY_CREDENTIAL_USERNAME, propono_connection_get_username (conn));
	      break;
	    }
	  need_username= TRUE;
	  break;

	case VNC_DISPLAY_CREDENTIAL_PASSWORD:
	  if (propono_connection_get_password (conn))
	    {
	      vnc_display_set_credential (vnc, VNC_DISPLAY_CREDENTIAL_PASSWORD, propono_connection_get_password (conn));
	      break;
	    }
	  need_password = TRUE;
	  break;

        case VNC_DISPLAY_CREDENTIAL_CLIENTNAME:
          vnc_display_set_credential (vnc, VNC_DISPLAY_CREDENTIAL_CLIENTNAME, "propono");
          break;
      }
  }

  if (need_password || need_username)
    {
      propono_tab_find_credentials_in_keyring (tab, &username, &password);
      if ( (need_username && !username) || (need_password && !password) )
	{
	  host = propono_connection_get_best_name (conn);
	  if (!propono_utils_ask_credential (window,
					     "VNC",
					     host,
					     need_username,
					     need_password,
					     8,
					     &username,
					     &password,
					     &save_in_keyring))
	    {
	      propono_tab_remove_from_notebook (tab);
	      goto out;
	    }
	}

      if (need_username)
	{
	  if (username)
	    {
	      propono_connection_set_username (conn, username);
	      vnc_display_set_credential (vnc, VNC_DISPLAY_CREDENTIAL_USERNAME, username);
	    }
	  else
	    {
	      propono_tab_remove_from_notebook (tab);
	      propono_utils_show_error (_("Authentication error"),
					_("A username is required in order to access this machine."),
					window);
	      goto out;
	    }
	}

      if (need_password)
	{
	  if (password)
	    {
	      propono_connection_set_password (conn, password);
	      vnc_display_set_credential (vnc, VNC_DISPLAY_CREDENTIAL_PASSWORD, password);
	    }
	  else
	    {
	      propono_tab_remove_from_notebook (tab);
	      propono_utils_show_error (_("Authentication error"),
					_("A password is required in order to access this machine."),
					window);
	      goto out;
	    }
	}

      propono_tab_set_save_credentials (tab, save_in_keyring);

out:
      g_free (username);
      g_free (password);
      g_free (host);
    }
}

static void
vnc_pointer_grab_cb (VncDisplay *vnc, ProponoVncTab *vnc_tab)
{
  vnc_tab->priv->pointer_grab = TRUE;
}

static void
vnc_pointer_ungrab_cb (VncDisplay *vnc, ProponoVncTab *vnc_tab)
{
  vnc_tab->priv->pointer_grab = FALSE;
}

static void
vnc_bell_cb (VncDisplay *vnc, ProponoVncTab *vnc_tab)
{
  gdk_window_beep (GTK_WIDGET (vnc_tab)->window);
}

static void
vnc_desktop_resize_cb (VncDisplay *vnc, int x, int y, ProponoVncTab *tab)
{
  g_object_notify (G_OBJECT (tab), "original-width");
  g_object_notify (G_OBJECT (tab), "original-height");
  g_object_notify (G_OBJECT (tab), "tooltip");
}

static GSList *
create_connected_actions (ProponoVncTab *tab)
{
  GSList *list = NULL;
  ProponoTabUiAction *a;

  /* View->Scaling */
  a = g_new (ProponoTabUiAction, 1);
  a->paths = g_new (gchar *, 3);
  a->paths[0] = g_strdup ("/MenuBar/ViewMenu");
  a->paths[1] = g_strdup ("/ToolBar");
  a->paths[2] = NULL;
  a->action = GTK_ACTION (gtk_toggle_action_new ("VNCViewScaling",
						 _("S_caling"),
						 _("Fits the remote screen into the current window size"),
						 "zoom-fit-best"));
  gtk_action_set_icon_name (a->action, "zoom-fit-best");
  g_signal_connect (a->action, "activate", G_CALLBACK (view_scaling_cb), tab);
  list = g_slist_append (list, a);
  tab->priv->scaling_action = a->action;

  /* View->Keep Ratio */
  a = g_new (ProponoTabUiAction, 1);
  a->paths = g_new (gchar *, 2);
  a->paths[0] = g_strdup ("/MenuBar/ViewMenu");
  a->paths[1] = NULL;
  a->action = GTK_ACTION (gtk_toggle_action_new ("VNCViewKeepRatio",
						 _("_Keep Aspect Ratio"),
						 _("Keeps the screen aspect ratio when using scaling"),
						 NULL));
  gtk_action_set_sensitive (a->action, FALSE);
  g_signal_connect (a->action, "activate", G_CALLBACK (view_keep_ratio_cb), tab);
  list = g_slist_append (list, a);
  tab->priv->keep_ratio_action = a->action;

  /* View->View Only */
  a = g_new (ProponoTabUiAction, 1);
  a->paths = g_new (gchar *, 3);
  a->paths[0] = g_strdup ("/MenuBar/ViewMenu");
  a->paths[1] = g_strdup ("/ToolBar");
  a->paths[2] = NULL;
  a->action = GTK_ACTION (gtk_toggle_action_new ("VNCViewViewOnly",
						 _("_View only"),
						 _("Does not send mouse and keyboard events"),
						 "emblem-readonly"));
  gtk_action_set_icon_name (a->action, "emblem-readonly");
  g_signal_connect (a->action, "activate", G_CALLBACK (view_viewonly_cb), tab);
  list = g_slist_append (list, a);
  tab->priv->viewonly_action = a->action;

  /* View->Original Size */
  a = g_new (ProponoTabUiAction, 1);
  a->paths = g_new (gchar *, 2);
  a->paths[0] = g_strdup ("/MenuBar/ViewMenu");
  a->paths[1] = NULL;
  a->action = gtk_action_new ("VNCViewOriginalSize",
			      _("_Original size"),
			      _("Adjusts the window to the remote desktop's size"),
			      "zoom-original");
  gtk_action_set_icon_name (a->action, "zoom-original");
  g_signal_connect (a->action, "activate", G_CALLBACK (view_original_size_cb), tab);
  list = g_slist_append (list, a);
  tab->priv->original_size_action = a->action;

  /* View->Refresh */
  a = g_new (ProponoTabUiAction, 1);
  a->paths = g_new (gchar *, 2);
  a->paths[0] = g_strdup ("/MenuBar/ViewMenu");
  a->paths[1] = NULL;
  a->action = gtk_action_new ("VNCViewRefresh",
			      _("_Refresh Screen"),
			      _("Requests an update of the screen"),
			      "gtk-refresh");
  gtk_action_set_icon_name (a->action, "gtk-refresh");
  g_signal_connect (a->action, "activate", G_CALLBACK (view_refresh_cb), tab);
  list = g_slist_append (list, a);

  return list;
}

static GSList *
create_initialized_actions (ProponoVncTab *tab)
{
  GSList *list = NULL;
  ProponoTabUiAction *a;

  /* Machine->Send CTRL-ALT-DEL */
  a = g_new (ProponoTabUiAction, 1);
  a->paths = g_new (gchar *, 3);
  a->paths[0] = g_strdup ("/MenuBar/MachineMenu/MachineOps_1");
  a->paths[1] = g_strdup ("/ToolBar");
  a->paths[2] = NULL;
  a->action = gtk_action_new ("VNCMachineSendCtrlAltDel",
			      _("Send Ctrl-Alt-Del"),
			      _("Sends Ctrl+Alt+Del to the remote machine"),
			      "preferences-desktop-keyboard-shortcuts");
  gtk_action_set_is_important (a->action, TRUE);
  gtk_action_set_icon_name (a->action, "preferences-desktop-keyboard-shortcuts");
  g_signal_connect (a->action, "activate", G_CALLBACK (send_ctrlaltdel_cb), tab);
  list = g_slist_append (list, a);

  return list;
}

static void
viewonly_button_clicked (GtkToggleToolButton *button,
			 ProponoVncTab       *vnc_tab)
{
  propono_vnc_tab_set_viewonly (vnc_tab, gtk_toggle_tool_button_get_active (button));

  gtk_toggle_action_set_active (GTK_TOGGLE_ACTION (vnc_tab->priv->viewonly_action),
				propono_vnc_tab_get_viewonly (vnc_tab));
}

static void
scaling_button_clicked (GtkToggleToolButton *button,
			ProponoVncTab       *vnc_tab)
{
  if (!propono_vnc_tab_set_scaling (vnc_tab, gtk_toggle_tool_button_get_active (button)))
    gtk_toggle_tool_button_set_active (button, FALSE);

  gtk_toggle_action_set_active (GTK_TOGGLE_ACTION (vnc_tab->priv->scaling_action),
				propono_vnc_tab_get_scaling (vnc_tab));
}

static void
cad_button_clicked (GtkToolButton *button,
		    ProponoVncTab *vnc_tab)
{
  propono_vnc_tab_send_ctrlaltdel (vnc_tab);
}

static void
setup_toolbar (ProponoVncTab *vnc_tab)
{
  GtkWidget *toolbar = propono_tab_get_toolbar (PROPONO_TAB (vnc_tab));
  GtkWidget *button;

  /* Space */
  button = GTK_WIDGET (gtk_separator_tool_item_new ());
  gtk_tool_item_set_expand (GTK_TOOL_ITEM (button), TRUE);
  gtk_widget_show (GTK_WIDGET (button));
  gtk_toolbar_insert (GTK_TOOLBAR (toolbar), GTK_TOOL_ITEM (button), -1);

  /* Scaling */
  button = GTK_WIDGET (gtk_toggle_tool_button_new ());
  gtk_tool_button_set_label (GTK_TOOL_BUTTON (button), _("Scaling"));
  gtk_tool_item_set_tooltip_text (GTK_TOOL_ITEM (button), _("Scaling"));
  gtk_tool_button_set_icon_name (GTK_TOOL_BUTTON (button), "zoom-fit-best");
  gtk_widget_show (GTK_WIDGET (button));
  gtk_toolbar_insert (GTK_TOOLBAR (toolbar), GTK_TOOL_ITEM (button), -1);
  g_signal_connect (button, "toggled", G_CALLBACK (scaling_button_clicked), vnc_tab);
  vnc_tab->priv->scaling_button = button;

  /* Read only */
  button = GTK_WIDGET (gtk_toggle_tool_button_new ());
  gtk_tool_button_set_label (GTK_TOOL_BUTTON (button), _("Read only"));
  gtk_tool_item_set_tooltip_text (GTK_TOOL_ITEM (button), _("Read only"));
  gtk_tool_button_set_icon_name (GTK_TOOL_BUTTON (button), "emblem-readonly");
  gtk_widget_show (GTK_WIDGET (button));
  gtk_toolbar_insert (GTK_TOOLBAR (toolbar), GTK_TOOL_ITEM (button), -1);
  g_signal_connect (button, "toggled", G_CALLBACK (viewonly_button_clicked), vnc_tab);
  vnc_tab->priv->viewonly_button = button;

  /* Send Ctrl-alt-del */
  button = GTK_WIDGET (gtk_tool_button_new (NULL, _("Send Ctrl-Alt-Del")));
  gtk_tool_button_set_icon_name (GTK_TOOL_BUTTON (button), "preferences-desktop-keyboard-shortcuts");
  gtk_tool_item_set_tooltip_text (GTK_TOOL_ITEM (button), _("Send Ctrl-Alt-Del"));
  g_signal_connect (button, "clicked", G_CALLBACK (cad_button_clicked), vnc_tab);
  gtk_widget_show (GTK_WIDGET (button));
  gtk_toolbar_insert (GTK_TOOLBAR (toolbar), GTK_TOOL_ITEM (button), -1);
}

static void
vnc_tab_clipboard_cb (GtkClipboard *cb, GdkEvent *event, ProponoVncTab *vnc_tab)
{
  ProponoTab *tab = PROPONO_TAB (vnc_tab);
  gchar *text;

  if (propono_notebook_get_active_tab (propono_tab_get_notebook (tab)) != tab)
    return;

  if (PROPONO_IS_TAB (gtk_clipboard_get_owner (cb)))
    return;

  text = gtk_clipboard_wait_for_text (cb);
  if (!text)
    return;

  propono_vnc_tab_paste_text (vnc_tab, text);
  g_free (text);
}

static void
propono_vnc_tab_init (ProponoVncTab *vnc_tab)
{
  GtkClipboard *cb;

  vnc_tab->priv = PROPONO_VNC_TAB_GET_PRIVATE (vnc_tab);
  vnc_tab->priv->clipboard_str = NULL;
  vnc_tab->priv->connected_actions = create_connected_actions (vnc_tab);
  vnc_tab->priv->initialized_actions = create_initialized_actions (vnc_tab);

  /* Create the vnc widget */
  vnc_tab->priv->vnc = vnc_display_new ();
  vnc_tab->priv->align = gtk_alignment_new (0.5, 0.5, 1, 1);
  vnc_tab->priv->signal_align = 0;
  gtk_container_add (GTK_CONTAINER (vnc_tab->priv->align), vnc_tab->priv->vnc);

  propono_tab_add_view (PROPONO_TAB (vnc_tab), vnc_tab->priv->align);

  g_signal_connect (vnc_tab->priv->vnc,
		    "vnc-connected",
		    G_CALLBACK (vnc_connected_cb),
		    vnc_tab);

  g_signal_connect (vnc_tab->priv->vnc,
		    "vnc-initialized",
		    G_CALLBACK (vnc_initialized_cb),
		    vnc_tab);

  g_signal_connect (vnc_tab->priv->vnc,
		    "vnc-disconnected",
		    G_CALLBACK (vnc_disconnected_cb),
		    vnc_tab);

  g_signal_connect (vnc_tab->priv->vnc,
		    "vnc-auth-credential",
		    G_CALLBACK (vnc_authentication_cb),
		    vnc_tab);

  g_signal_connect (vnc_tab->priv->vnc,
		    "vnc-pointer-grab",
		    G_CALLBACK (vnc_pointer_grab_cb),
		    vnc_tab);

  g_signal_connect (vnc_tab->priv->vnc,
		    "vnc-pointer-ungrab",
		    G_CALLBACK (vnc_pointer_ungrab_cb),
		    vnc_tab);

  g_signal_connect (vnc_tab->priv->vnc,
		    "vnc-auth-failure",
		    G_CALLBACK (vnc_auth_failed_cb),
		    vnc_tab);

  g_signal_connect (vnc_tab->priv->vnc,
		    "vnc-auth-unsupported",
		    G_CALLBACK (vnc_auth_unsupported_cb),
		    vnc_tab);

  g_signal_connect (vnc_tab->priv->vnc,
		    "vnc-server-cut-text",
		    G_CALLBACK (vnc_server_cut_text_cb),
		    vnc_tab);

  g_signal_connect (vnc_tab->priv->vnc,
		    "vnc-bell",
		    G_CALLBACK (vnc_bell_cb),
		    vnc_tab);

  g_signal_connect (vnc_tab->priv->vnc,
		    "vnc-desktop-resize",
		    G_CALLBACK (vnc_desktop_resize_cb),
		    vnc_tab);

  /* Setup the clipboard */
  cb = gtk_clipboard_get (GDK_SELECTION_CLIPBOARD);
  vnc_tab->priv->signal_clipboard =  g_signal_connect (cb,
						       "owner-change",
						       G_CALLBACK (vnc_tab_clipboard_cb),
						       vnc_tab);


  gtk_widget_show_all (GTK_WIDGET (vnc_tab));
}

GtkWidget *
propono_vnc_tab_new (ProponoConnection *conn,
		     ProponoWindow     *window)
{
  return GTK_WIDGET (g_object_new (PROPONO_TYPE_VNC_TAB,
				   "conn", conn,
				   "window", window,
				   NULL));
}

void
propono_vnc_tab_send_ctrlaltdel (ProponoVncTab *tab)
{
  guint keys[] = { GDK_Control_L, GDK_Alt_L, GDK_Delete };

  g_return_if_fail (PROPONO_IS_VNC_TAB (tab));

  vnc_display_send_keys_ex (VNC_DISPLAY (tab->priv->vnc), keys, sizeof (keys) / sizeof (keys[0]), VNC_DISPLAY_KEY_EVENT_CLICK);
}

void
propono_vnc_tab_paste_text (ProponoVncTab *tab, const gchar *text)
{
  gchar *out;
  gsize a, b;
  GError *error = NULL;

  g_return_if_fail (PROPONO_IS_VNC_TAB (tab));

  out = g_convert_with_fallback (text, -1, "iso8859-1//TRANSLIT", "utf-8", NULL, &a, &b, &error);
  if (out)
    {
      vnc_display_client_cut_text (VNC_DISPLAY (tab->priv->vnc), out);
      g_free (out);
    }
  else
    {
      g_warning ("%s", error->message);
      g_error_free (error);
    }
}

gboolean
propono_vnc_tab_set_scaling (ProponoVncTab *tab, gboolean active) {
  g_return_val_if_fail (PROPONO_IS_VNC_TAB (tab), FALSE);

  if (vnc_display_get_scaling (VNC_DISPLAY (tab->priv->vnc)) == active)
    return TRUE;

  vnc_display_set_force_size (VNC_DISPLAY(tab->priv->vnc), !active);
  if (!vnc_display_set_scaling (VNC_DISPLAY (tab->priv->vnc), active))
    {
      propono_utils_show_error (NULL, _("Scaling is not supported on this installation.\n\nRead the README file (shipped with Propono) in order to know how to enable this feature."),
				GTK_WINDOW (propono_tab_get_window (PROPONO_TAB (tab))));
      return FALSE;
    }

  gtk_toggle_tool_button_set_active (GTK_TOGGLE_TOOL_BUTTON (tab->priv->scaling_button),
				     active);
  gtk_action_set_sensitive (tab->priv->keep_ratio_action, active);

  if (active)
    gtk_widget_set_size_request (tab->priv->vnc, 0, 0);
  else
    gtk_widget_set_size_request (tab->priv->vnc,
				 vnc_display_get_width (VNC_DISPLAY (tab->priv->vnc)),
				 vnc_display_get_height (VNC_DISPLAY (tab->priv->vnc)));

  return TRUE;
}

gboolean
propono_vnc_tab_get_scaling (ProponoVncTab *tab) {
  g_return_val_if_fail (PROPONO_IS_VNC_TAB (tab), FALSE);

  return vnc_display_get_scaling (VNC_DISPLAY (tab->priv->vnc));
}

void
propono_vnc_tab_set_viewonly (ProponoVncTab *tab, gboolean active) {
  g_return_if_fail (PROPONO_IS_VNC_TAB (tab));

  vnc_display_set_read_only (VNC_DISPLAY (tab->priv->vnc), active);
  gtk_toggle_tool_button_set_active (GTK_TOGGLE_TOOL_BUTTON (tab->priv->viewonly_button),
				     active);
}

gboolean
propono_vnc_tab_get_viewonly (ProponoVncTab *tab) {
  g_return_val_if_fail (PROPONO_IS_VNC_TAB (tab), FALSE);

  return vnc_display_get_read_only (VNC_DISPLAY (tab->priv->vnc));
}

/*
 * Called when the main container widget's size has been set.
 * It attempts to fit the VNC widget into this space while
 * maintaining aspect ratio
 *
 * Code borrowed from from virt-viewer, thanks Daniel Berrange :)
 */

static void
vnc_tab_resize_align (GtkWidget *widget,
		      GtkAllocation *alloc,
		      ProponoVncTab *vnc_tab)
{
  double desktopAspect = (double)vnc_display_get_width (VNC_DISPLAY (vnc_tab->priv->vnc)) / (double)vnc_display_get_height (VNC_DISPLAY (vnc_tab->priv->vnc));
  double scrollAspect = (double)alloc->width / (double)alloc->height;
  int height, width;
  GtkAllocation child;
  int dx = 0, dy = 0;

  if (!vnc_display_is_open (VNC_DISPLAY (vnc_tab->priv->vnc)))
    return;

  if (scrollAspect > desktopAspect)
    {
      width = alloc->height * desktopAspect;
      dx = (alloc->width - width) / 2;
      height = alloc->height;
    }
  else
    {
      width = alloc->width;
      height = alloc->width / desktopAspect;
      dy = (alloc->height - height) / 2;
    }

  child.x = alloc->x + dx;
  child.y = alloc->y + dy;
  child.width = width;
  child.height = height;
  gtk_widget_size_allocate(vnc_tab->priv->vnc, &child);
}

void
propono_vnc_tab_set_keep_ratio (ProponoVncTab *tab, gboolean active)
{
  g_return_if_fail (PROPONO_IS_VNC_TAB (tab));

  if (tab->priv->signal_align > 0)
    g_signal_handler_disconnect (tab->priv->align, tab->priv->signal_align);

  if (active)
    tab->priv->signal_align = g_signal_connect (tab->priv->align,
						"size-allocate",
						G_CALLBACK (vnc_tab_resize_align),
						tab);
  else
    tab->priv->signal_align = 0;

  gtk_widget_queue_resize_no_redraw (tab->priv->align);
}

gboolean
propono_vnc_tab_get_keep_ratio (ProponoVncTab *tab)
{
  g_return_val_if_fail (PROPONO_IS_VNC_TAB (tab), FALSE);

  return propono_vnc_connection_get_keep_ratio (PROPONO_VNC_CONNECTION (propono_tab_get_conn (PROPONO_TAB (tab))));
}

gboolean
propono_vnc_tab_is_pointer_grab (ProponoVncTab *tab)
{
  g_return_val_if_fail (PROPONO_IS_VNC_TAB (tab), FALSE);

  return tab->priv->pointer_grab;
}

gint
propono_vnc_tab_get_original_height (ProponoVncTab *tab)
{
  g_return_val_if_fail (PROPONO_IS_VNC_TAB (tab), -1);

  if (VNC_IS_DISPLAY (tab->priv->vnc))
    return vnc_display_get_height (VNC_DISPLAY (tab->priv->vnc));
  else
    return -1;
}

gint
propono_vnc_tab_get_original_width (ProponoVncTab *tab)
{
  g_return_val_if_fail (PROPONO_IS_VNC_TAB (tab), -1);

  if (VNC_IS_DISPLAY (tab->priv->vnc))
    return vnc_display_get_width (VNC_DISPLAY (tab->priv->vnc));
  else
    return -1;
}


typedef struct _ProponoPrefSize {
  gint width, height;
  gulong sig_id;
} ProponoPrefSize;

static gboolean
cb_unset_size (gpointer data)
{
  GtkWidget *widget = data;

  gtk_widget_queue_resize_no_redraw (widget);

  return FALSE;
}

static void
cb_set_preferred_size (GtkWidget *widget, GtkRequisition *req,
		       gpointer data)
{
  ProponoPrefSize *size = data;

  req->width = size->width;
  req->height = size->height;

  g_signal_handler_disconnect (widget, size->sig_id);
  g_free (size);
  g_idle_add (cb_unset_size, widget);
}

static void
propono_widget_set_preferred_size (GtkWidget *widget,
				   gint      width,
				   gint      height)
{
  ProponoPrefSize *size = g_new (ProponoPrefSize, 1);

  size->width  = width;
  size->height = height;
  size->sig_id = g_signal_connect (widget, "size-request",
				   G_CALLBACK (cb_set_preferred_size),
				   size);

  gtk_widget_queue_resize (widget);
}


void
propono_vnc_tab_original_size (ProponoVncTab *tab)
{
  GtkWindow *window;
  g_return_if_fail (PROPONO_IS_VNC_TAB (tab));

  window = GTK_WINDOW (propono_tab_get_window (PROPONO_TAB (tab)));

  gtk_window_unmaximize (window);
  gtk_window_resize (window, 1, 1);
  propono_widget_set_preferred_size (GTK_WIDGET (tab),
				     propono_vnc_tab_get_original_width (tab),
				     propono_vnc_tab_get_original_height (tab));
}

/* vim: set ts=8: */
