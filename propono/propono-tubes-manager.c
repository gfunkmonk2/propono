/*
 * propono-tubes-manager.c
 * This file is part of propono
 *
 * © 2009, Collabora Ltd
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
 *
 * Authors:
 *      Arnaud Maillet <arnaud.maillet@collabora.co.uk>
 *      Danielle Madeley <danielle.madeley@collabora.co.uk>
 */

#include <glib-object.h>

#include <dbus/dbus-glib.h>
#include <dbus/dbus-glib-bindings.h>

#include <telepathy-glib/enums.h>
#include <telepathy-glib/dbus.h>
#include <telepathy-glib/interfaces.h>
#include <telepathy-glib/defs.h>

#include "propono-tubes-manager.h"
#include "propono-tube-handler.h"
#include "propono-debug.h"

#define CLIENT_NAME "Propono"
#define SERVICE "rfb"

#define BUS_NAME TP_CLIENT_BUS_NAME_BASE CLIENT_NAME
#define OBJECT_PATH TP_CLIENT_OBJECT_PATH_BASE CLIENT_NAME

G_DEFINE_TYPE (ProponoTubesManager, propono_tubes_manager, TP_TYPE_HANDLER);

#define PROPONO_TUBES_MANAGER_GET_PRIVATE(obj)\
    (G_TYPE_INSTANCE_GET_PRIVATE ((obj), PROPONO_TYPE_TUBES_MANAGER,\
    ProponoTubesManagerPrivate))

typedef struct _ProponoTubesManagerPrivate ProponoTubesManagerPrivate;

struct _ProponoTubesManagerPrivate
{
  ProponoWindow *window;
  GSList *tubes_handler;
};

enum
{
  PROP_0,
  PROP_PROPONO_WINDOW
};

static void
propono_tubes_manager_dispose (GObject *object)
{
  ProponoTubesManagerPrivate *priv = PROPONO_TUBES_MANAGER_GET_PRIVATE
      (object);
  GSList *l;

  for (l = priv->tubes_handler; l; l = l->next)
    g_object_unref (l->data);

  g_slist_free (priv->tubes_handler);
  priv->tubes_handler = NULL;

  G_OBJECT_CLASS (propono_tubes_manager_parent_class)->dispose (object);
}

static void
propono_tubes_manager_set_propono_window (ProponoTubesManager *self,
    gpointer *propono_window)
{
  g_return_if_fail (PROPONO_IS_TUBES_MANAGER (self));

  ProponoTubesManagerPrivate *priv = PROPONO_TUBES_MANAGER_GET_PRIVATE (self);
  priv->window = PROPONO_WINDOW (propono_window);
}

static void
propono_tubes_manager_set_property (GObject *object, guint prop_id,
    const GValue *value, GParamSpec   *pspec)
{
  ProponoTubesManager *self = PROPONO_TUBES_MANAGER (object);

  switch (prop_id)
    {
    case PROP_PROPONO_WINDOW:
      propono_tubes_manager_set_propono_window (self,
          g_value_get_object (value));
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
    }
}

static void
propono_tubes_manager_get_property (GObject *object, guint prop_id,
    GValue *value, GParamSpec *pspec)
{
  ProponoTubesManagerPrivate *priv = PROPONO_TUBES_MANAGER_GET_PRIVATE
      (object);

  switch (prop_id)
    {
    case PROP_PROPONO_WINDOW:
      g_value_set_object (value, priv->window);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
    }
}

static void
propono_tubes_manager_disconnected_cb (ProponoTubeHandler *htube,
    gpointer self)
{
  ProponoTubesManagerPrivate *priv = PROPONO_TUBES_MANAGER_GET_PRIVATE (self);
  priv->tubes_handler = g_slist_remove (priv->tubes_handler, htube);
  g_object_unref (htube);
}

static gboolean
propono_tubes_manager_handle_channels (ProponoHandler         *self,
                                       TpAccount         *account,
                                       TpConnection      *connection,
                                       TpChannel        **channels,
                                       TpChannelRequest **requests,
                                       guint64            user_action_time,
                                       GHashTable        *handler_info)
{
  ProponoTubesManagerPrivate *priv = PROPONO_TUBES_MANAGER_GET_PRIVATE (self);
  TpChannel *channel, **ptr;

  for (ptr = channels; channel = *ptr; ptr++)
    {
      ProponoTubeHandler *htube;

      htube = propono_tube_handler_new (priv->window, channel);
      priv->tubes_handler = g_slist_prepend (priv->tubes_handler, htube);

      g_signal_connect (G_OBJECT (htube), "disconnected", G_CALLBACK
          (propono_tubes_manager_disconnected_cb), self);
    }

  return TRUE;
}

static void
propono_tubes_manager_register_tube_handler (GObject *object)
{
  TpDBusDaemon *bus;
  guint result;
  GError *error = NULL;

  bus = tp_dbus_daemon_dup (&error);
  if (error != NULL)
    {
      propono_debug_message (DEBUG_TUBE, "Failed to connect to bus: %s",
          error ? error->message : "No error given");
      g_clear_error (&error);
      goto OUT;
    }

  if (!tp_dbus_daemon_request_name (bus, BUS_NAME, FALSE, &error))
    {
      propono_debug_message (DEBUG_TUBE, "Failed to request name: %s",
          error ? error->message : "No error given");
      g_clear_error (&error);
      goto OUT;
    }

  propono_debug_message (DEBUG_TUBE,
      "Creating tube handler %s object_path:%s\n",
      BUS_NAME, OBJECT_PATH);
  dbus_g_connection_register_g_object (
      tp_proxy_get_dbus_connection (TP_PROXY (bus)),
      OBJECT_PATH, G_OBJECT (object));

OUT:
  g_object_unref (bus);
}

static void
propono_tubes_manager_constructed (GObject *object)
{
  ProponoTubesManagerPrivate *priv = PROPONO_TUBES_MANAGER_GET_PRIVATE
      (object);

  propono_tubes_manager_register_tube_handler (object);
}

static void
propono_tubes_manager_class_init (ProponoTubesManagerClass *klass)
{
  GObjectClass *gobject_class = G_OBJECT_CLASS (klass);

  gobject_class->constructed = propono_tubes_manager_constructed;
  gobject_class->set_property = propono_tubes_manager_set_property;
  gobject_class->get_property = propono_tubes_manager_get_property;
  gobject_class->dispose = propono_tubes_manager_dispose;

  g_object_class_install_property (gobject_class,
      PROP_PROPONO_WINDOW,
      g_param_spec_object ("propono-window",
      "Propono window",
      "The Propono window",
      PROPONO_TYPE_WINDOW,
      G_PARAM_READWRITE      |
      G_PARAM_CONSTRUCT_ONLY |
      G_PARAM_STATIC_STRINGS));

  g_type_class_add_private (klass, sizeof (ProponoTubesManagerPrivate));
}

static void
propono_tubes_manager_init (ProponoTubesManager *object)
{
}

ProponoTubesManager *
propono_tubes_manager_new (ProponoWindow *propono_window)
{
  ProponoTubesManager *self;
  GPtrArray *filter = g_ptr_array_new ();
  GHashTable *map = tp_asv_new (
      TP_IFACE_CHANNEL ".ChannelType", G_TYPE_STRING,
              TP_IFACE_CHANNEL_TYPE_STREAM_TUBE,
      TP_IFACE_CHANNEL ".TargetHandleType", G_TYPE_UINT,
              TP_HANDLE_TYPE_CONTACT,
      TP_IFACE_CHANNEL ".Requested", G_TYPE_BOOLEAN,
              FALSE,
      TP_IFACE_CHANNEL_TYPE_STREAM_TUBE ".Service", G_TYPE_STRING,
              SERVICE,
      NULL);
  g_ptr_array_add (filter, map);

  self = g_object_new (PROPONO_TYPE_TUBES_MANAGER,
      "channel-filter", filter,
      "bypass-approval", FALSE,
      "handle-channels-cb", propono_tubes_manager_handle_channels,
      "propono-window", propono_window,
      NULL);

  g_hash_table_destroy (map);
  g_ptr_array_free (filter, TRUE);

  return self;
}
