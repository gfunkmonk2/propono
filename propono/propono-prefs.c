/*
 * propono-prefs.c
 * This file is part of propono
 *
 * Copyright (C) Jonh Wendell 2008,2009 <wendell@bani.com.br>
 * 
 * propono-prefs.c is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation, either version 2 of the License, or
 * (at your option) any later version.
 * 
 * propono-prefs.c is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License along
 * with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <mateconf/mateconf-client.h>
#include <glib/gi18n.h>
#include "propono-prefs.h"
#include "propono-utils.h"

#define PROPONO_BASE_KEY		"/apps/propono"
#define VM_ALWAYS_SHOW_TABS		PROPONO_BASE_KEY "/always_show_tabs"
#define VM_SHOW_ACCELS			PROPONO_BASE_KEY "/show_accels"
#define VM_HISTORY_SIZE			PROPONO_BASE_KEY "/history_size"
#define VM_ALWAYS_ENABLE_LISTENING	PROPONO_BASE_KEY "/always_enable_listening"
#define VM_SHARED_FLAG			PROPONO_BASE_KEY "/shared_flag"
#define PROPONO_PLUGINS_DIR		PROPONO_BASE_KEY "/plugins"
#define VM_ACTIVE_PLUGINS		PROPONO_PLUGINS_DIR "/active-plugins"

struct _ProponoPrefsPrivate
{
  MateConfClient *mateconf_client;
};

/* Properties */
enum
{
  PROP_0,
  PROP_SHARED_FLAG,
  PROP_ALWAYS_SHOW_TABS,
  PROP_SHOW_ACCELS,
  PROP_HISTORY_SIZE,
  PROP_ACTIVE_PLUGINS,
  PROP_LAST_PROTOCOL,
  PROP_ALWAYS_ENABLE_LISTENING
};

G_DEFINE_TYPE (ProponoPrefs, propono_prefs, G_TYPE_OBJECT);

static ProponoPrefs *prefs_singleton = NULL;

ProponoPrefs *
propono_prefs_get_default (void)
{
  if (G_UNLIKELY (!prefs_singleton))
    prefs_singleton = PROPONO_PREFS (g_object_new (PROPONO_TYPE_PREFS,
                                                   NULL));
  return prefs_singleton;
}

static gboolean
propono_prefs_get_bool (ProponoPrefs *prefs, const gchar* key, gboolean def)
{
  GError* error = NULL;
  MateConfValue* val;

  val = mateconf_client_get (prefs->priv->mateconf_client, key, &error);

  if (val != NULL)
    {
      gboolean retval = def;

      g_return_val_if_fail (error == NULL, retval);
      
      if (val->type == MATECONF_VALUE_BOOL)
        retval = mateconf_value_get_bool (val);

      mateconf_value_free (val);

      return retval;
    }
  else
      return def;
}

static gboolean
propono_prefs_get_int (ProponoPrefs *prefs, const gchar* key, gint def)
{
  GError* error = NULL;
  MateConfValue* val;

  val = mateconf_client_get (prefs->priv->mateconf_client, key, &error);

  if (val != NULL)
    {
      gint retval = def;

      g_return_val_if_fail (error == NULL, retval);
      
      if (val->type == MATECONF_VALUE_INT)
        retval = mateconf_value_get_int (val);

      mateconf_value_free (val);

      return retval;
    }
  else
      return def;
}

static gchar *
propono_prefs_get_string (ProponoPrefs *prefs, const gchar *key, const gchar *def)
{
  gchar *result;

  result = mateconf_client_get_string (prefs->priv->mateconf_client, key, NULL);
  if (!result)
    result = g_strdup (def);

  return result;
}

static GSList *
propono_prefs_get_list (ProponoPrefs *prefs, const gchar* key)
{
  return mateconf_client_get_list (prefs->priv->mateconf_client,
				key,
				MATECONF_VALUE_STRING,
				NULL);
}

static void
propono_prefs_set_bool (ProponoPrefs *prefs, const gchar* key, gboolean value)
{
  GError *error = NULL;

  if (!mateconf_client_set_bool (prefs->priv->mateconf_client, key, value, &error))
    {
      g_warning ("Setting key %s failed: %s", key, error->message);
      g_error_free (error);
    }
}

static void
propono_prefs_set_int (ProponoPrefs *prefs, const gchar* key, gint value)
{
  GError *error = NULL;

  if (!mateconf_client_set_int (prefs->priv->mateconf_client, key, value, &error))
    {
      g_warning ("Setting key %s failed: %s", key, error->message);
      g_error_free (error);
    }
}

static void
propono_prefs_set_string (ProponoPrefs *prefs, const gchar *key, const gchar *value)
{
  GError *error = NULL;

  if (!mateconf_client_set_string (prefs->priv->mateconf_client, key, value, &error))
    {
      g_warning ("Setting key %s failed: %s", key, error->message);
      g_error_free (error);
    }
}

static void
propono_prefs_set_list (ProponoPrefs *prefs, const gchar* key, GSList *list)
{
  GError *error = NULL;

  if (!mateconf_client_set_list (prefs->priv->mateconf_client, key,
			      MATECONF_VALUE_STRING, list,
			      &error))
    {
      g_warning ("Setting key %s failed: %s", key, error->message);
      g_error_free (error);
    }
}

static void
propono_prefs_always_show_tabs_notify (MateConfClient           *client,
				       guint                  cnx_id,
				       MateConfEntry            *entry,
				       ProponoPrefs          *prefs)
{
  g_object_notify (G_OBJECT (prefs), "always-show-tabs");
}

static void
propono_prefs_show_accels_notify (MateConfClient           *client,
				  guint                  cnx_id,
				  MateConfEntry            *entry,
				  ProponoPrefs          *prefs)
{
  g_object_notify (G_OBJECT (prefs), "show-accels");
}

static void
propono_prefs_init (ProponoPrefs *prefs)
{
  prefs->priv = G_TYPE_INSTANCE_GET_PRIVATE (prefs, PROPONO_TYPE_PREFS, ProponoPrefsPrivate);

  prefs->priv->mateconf_client = mateconf_client_get_default ();
  if (prefs->priv->mateconf_client == NULL)
    g_critical (_("Cannot initialize preferences manager."));

  mateconf_client_add_dir (prefs->priv->mateconf_client,
			PROPONO_BASE_KEY,
			MATECONF_CLIENT_PRELOAD_ONELEVEL,
			NULL);

  mateconf_client_notify_add (prefs->priv->mateconf_client,
			   VM_ALWAYS_SHOW_TABS,
                           (MateConfClientNotifyFunc) propono_prefs_always_show_tabs_notify,
                           prefs, NULL, NULL);
  mateconf_client_notify_add (prefs->priv->mateconf_client,
			   VM_SHOW_ACCELS,
                           (MateConfClientNotifyFunc) propono_prefs_show_accels_notify,
                           prefs, NULL, NULL);
}

static void
propono_prefs_set_property (GObject *object, guint prop_id, const GValue *value, GParamSpec *pspec)
{
  ProponoPrefs *prefs = PROPONO_PREFS (object);

  switch (prop_id)
    {
      case PROP_SHARED_FLAG:
	propono_prefs_set_bool (prefs, VM_SHARED_FLAG, g_value_get_boolean (value));
	break;
      case PROP_ALWAYS_SHOW_TABS:
	propono_prefs_set_bool (prefs, VM_ALWAYS_SHOW_TABS, g_value_get_boolean (value));
	break;
      case PROP_SHOW_ACCELS:
	propono_prefs_set_bool (prefs, VM_SHOW_ACCELS, g_value_get_boolean (value));
	break;
      case PROP_HISTORY_SIZE:
	propono_prefs_set_int (prefs, VM_HISTORY_SIZE, g_value_get_int (value));
	break;
      case PROP_ACTIVE_PLUGINS:
	propono_prefs_set_list (prefs, VM_ACTIVE_PLUGINS, g_value_get_pointer (value));
	break;
      case PROP_ALWAYS_ENABLE_LISTENING:
	propono_prefs_set_bool (prefs, VM_ALWAYS_ENABLE_LISTENING, g_value_get_boolean (value));
	break;
      default:
	G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
	break;
    }
}

static void
propono_prefs_get_property (GObject *object, guint prop_id, GValue *value, GParamSpec *pspec)
{
  ProponoPrefs *prefs = PROPONO_PREFS (object);
  gchar        *str;

  switch (prop_id)
    {
      case PROP_SHARED_FLAG:
	g_value_set_boolean (value, propono_prefs_get_bool (prefs, VM_SHARED_FLAG, TRUE));
	break;
      case PROP_ALWAYS_SHOW_TABS:
	g_value_set_boolean (value, propono_prefs_get_bool (prefs, VM_ALWAYS_SHOW_TABS, FALSE));
	break;
      case PROP_SHOW_ACCELS:
	g_value_set_boolean (value, propono_prefs_get_bool (prefs, VM_SHOW_ACCELS, TRUE));
	break;
      case PROP_HISTORY_SIZE:
	g_value_set_int (value, propono_prefs_get_int (prefs, VM_HISTORY_SIZE, 15));
	break;
      case PROP_ACTIVE_PLUGINS:
	g_value_set_pointer (value, propono_prefs_get_list (prefs, VM_ACTIVE_PLUGINS));
	break;
      case PROP_ALWAYS_ENABLE_LISTENING:
	g_value_set_boolean (value, propono_prefs_get_bool (prefs, VM_ALWAYS_ENABLE_LISTENING, FALSE));
	break;
      default:
	G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
	break;
    }
}

static void
propono_prefs_dispose (GObject *object)
{
  ProponoPrefs *prefs = PROPONO_PREFS (object);

  if (prefs->priv->mateconf_client)
    {
      mateconf_client_remove_dir (prefs->priv->mateconf_client,
			       PROPONO_BASE_KEY,
			       NULL);
      g_object_unref (prefs->priv->mateconf_client);
      prefs->priv->mateconf_client = NULL;
    }

  G_OBJECT_CLASS (propono_prefs_parent_class)->dispose (object);
}


static void
propono_prefs_class_init (ProponoPrefsClass *klass)
{
  GObjectClass* object_class = G_OBJECT_CLASS (klass);
  GObjectClass* parent_class = G_OBJECT_CLASS (klass);

  g_type_class_add_private (klass, sizeof (ProponoPrefsPrivate));

  object_class->dispose = propono_prefs_dispose;
  object_class->set_property = propono_prefs_set_property;
  object_class->get_property = propono_prefs_get_property;

  g_object_class_install_property (object_class,
				   PROP_SHARED_FLAG,
				   g_param_spec_boolean ("shared-flag",
							 "Shared Flag",
							 "Whether we should share the remote connection",
							 TRUE,
							 G_PARAM_READWRITE));
  g_object_class_install_property (object_class,
				   PROP_ALWAYS_SHOW_TABS,
				   g_param_spec_boolean ("always-show-tabs",
							 "Always show tabs",
							 "Whether we should show the tabs even when there is ony one active connection",
							 FALSE,
							 G_PARAM_READWRITE));
  g_object_class_install_property (object_class,
				   PROP_SHOW_ACCELS,
				   g_param_spec_boolean ("show-accels",
							 "Show menu accelerators",
							 "Whether we should show the menu accelerators (keyboard shortcuts)",
							 FALSE,
							 G_PARAM_READWRITE));
  g_object_class_install_property (object_class,
				   PROP_HISTORY_SIZE,
				   g_param_spec_int ("history-size",
						     "History size",
						     "Max number of items in history dropdown entry",
						     0, G_MAXINT, 15,
						     G_PARAM_READWRITE));

  g_object_class_install_property (object_class,
				   PROP_ACTIVE_PLUGINS,
				   g_param_spec_pointer ("active-plugins",
							 "Active plugins",
							 "The list of active plugins",
							 G_PARAM_READWRITE));

  g_object_class_install_property (object_class,
				   PROP_ALWAYS_ENABLE_LISTENING,
				   g_param_spec_boolean ("always-enable-listening",
							 "Always enable listening",
							 "Whether we always should listen for reverse connections",
							 FALSE,
							 G_PARAM_READWRITE));
}

/* Preferences dialog */

typedef struct {
  GtkBuilder  *xml;
  GtkWidget   *dialog;
  GtkWidget   *show_tabs;
  GtkWidget   *show_accels;
  GtkWindow   *parent;
} ProponoPrefsDialog;

static void
propono_prefs_dialog_setup (ProponoPrefsDialog *dialog)
{
  gboolean show_accels, show_tabs;

  g_object_get (propono_prefs_get_default (),
		"show-accels", &show_accels,
		"always-show-tabs", &show_tabs,
		NULL);

  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (dialog->show_accels), show_accels);
  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (dialog->show_tabs), show_tabs);
}

static void
propono_prefs_dialog_response (GtkDialog *d, gint response_id, ProponoPrefsDialog *dialog)
{
  if (response_id > 0)
    {
      propono_utils_help_contents (dialog->parent, "preferences");
      return;
    }

  gtk_widget_destroy (dialog->dialog);
  g_object_unref (dialog->xml);
  g_free (dialog);
  dialog = NULL;
}

static void
propono_prefs_dialog_show_tabs_cb (ProponoPrefsDialog *dialog)
{
  g_object_set (propono_prefs_get_default (),
		"always-show-tabs", gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (dialog->show_tabs)),
		NULL);
}

static void
propono_prefs_dialog_show_accels_cb (ProponoPrefsDialog *dialog)
{
  g_object_set (propono_prefs_get_default (),
		"show-accels", gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (dialog->show_accels)),
		NULL);
}

void
propono_prefs_dialog_show (ProponoWindow *window)
{
  ProponoPrefsDialog *dialog;

  dialog = g_new (ProponoPrefsDialog, 1);

  dialog->xml = propono_utils_get_builder (NULL, NULL);
  dialog->dialog = GTK_WIDGET (gtk_builder_get_object (dialog->xml, "preferences_dialog"));
  dialog->parent = GTK_WINDOW (window);
  gtk_window_set_transient_for (GTK_WINDOW (dialog->dialog), dialog->parent);

  dialog->show_tabs = GTK_WIDGET (gtk_builder_get_object (dialog->xml, "always_show_tabs_check"));
  dialog->show_accels = GTK_WIDGET (gtk_builder_get_object (dialog->xml, "show_accels_check"));

  propono_prefs_dialog_setup (dialog);

  g_signal_connect (dialog->dialog,
		    "response",
		    G_CALLBACK (propono_prefs_dialog_response),
		    dialog);

  g_signal_connect_swapped (dialog->show_tabs,
			    "toggled",
			     G_CALLBACK (propono_prefs_dialog_show_tabs_cb),
			     dialog);

  g_signal_connect_swapped (dialog->show_accels,
			    "toggled",
			     G_CALLBACK (propono_prefs_dialog_show_accels_cb),
			     dialog);

  gtk_widget_show_all (dialog->dialog);
}
/* vim: set ts=8: */
