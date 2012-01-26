/*
 * handler.c - convenience class to implement a Telepathy Handler
 *
 * Copyright (C) 2009 Collabora Ltd. <http://www.collabora.co.uk/>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
 */

#include <telepathy-glib/dbus.h>
#include <telepathy-glib/interfaces.h>
#include <telepathy-glib/gtypes.h>
#include <telepathy-glib/enums.h>
#include <telepathy-glib/svc-generic.h>
#include <telepathy-glib/svc-client.h>
#include <telepathy-glib/util.h>

#include "handler.h"
#include "propono-marshal.h"

#define DEBUG

static void handler_iface_init (gpointer, gpointer);
static void requests_iface_init (gpointer, gpointer);

#define GET_PRIVATE(obj)	(G_TYPE_INSTANCE_GET_PRIVATE ((obj), TP_TYPE_HANDLER, ProponoHandlerPrivate))

/**
 * SECTION:handler
 * @title: ProponoHandler
 * @short_description: convenience class to implement a Telepathy Handler
 *
 * Provides a #GObject implementing the Client.Handler interface that can be
 * published using dbus-glib.
 *
 * <example><programlisting>
 * tpdbus = tp_dbus_daemon_dup (NULL);
 * dbus = tp_get_bus ();
 *
 * filter = g_ptr_array_new ();
 * map = tp_asv_new (
 *       TP_IFACE_CHANNEL ".ChannelType", G_TYPE_STRING,
 *           TP_IFACE_CHANNEL_TYPE_DBUS_TUBE,
 *       TP_IFACE_CHANNEL ".TargetHandleType", G_TYPE_UINT,
 *           TP_HANDLE_TYPE_ROOM,
 *       TP_IFACE_CHANNEL ".Requested", G_TYPE_BOOLEAN,
 *           FALSE,
 *       TP_IFACE_CHANNEL_TYPE_DBUS_TUBE ".ServiceName", G_TYPE_STRING,
 *           SERVICE_NAME,
 *       NULL
 *     );
 *  g_ptr_array_add (filter, map);
 *
 * handler = propono_handler_new (filter, FALSE, handle_channels);
 *
 * g_assert (tp_dbus_daemon_request_name (tpdbus,
 *     TP_CLIENT_BUS_NAME_BASE CLIENT_NAME,
 *     TRUE, NULL));
 * dbus_g_connection_register_g_object (dbus,
 *     TP_CLIENT_OBJECT_PATH_BASE CLIENT_NAME,
 *     G_OBJECT (handler));
 * </programlisting></example>
 */

G_DEFINE_TYPE_WITH_CODE (ProponoHandler, propono_handler, G_TYPE_OBJECT,
    G_IMPLEMENT_INTERFACE (TP_TYPE_SVC_DBUS_PROPERTIES,
      tp_dbus_properties_mixin_iface_init);
    G_IMPLEMENT_INTERFACE (TP_TYPE_SVC_CLIENT, NULL);
    G_IMPLEMENT_INTERFACE (TP_TYPE_SVC_CLIENT_HANDLER, handler_iface_init);
    G_IMPLEMENT_INTERFACE (TP_TYPE_SVC_CLIENT_INTERFACE_REQUESTS, requests_iface_init);
    );

static const char *client_interfaces[] = {
    TP_IFACE_CLIENT_HANDLER,
    TP_IFACE_CLIENT_INTERFACE_REQUESTS,
    NULL
};

enum /* properties */
{
  PROP_0,
  PROP_INTERFACES,
  PROP_CHANNEL_FILTER,
  PROP_BYPASS_APPROVAL,
  PROP_HANDLED_CHANNELS,
  PROP_HANDLE_CHANNELS_CB
};

enum /* signals */
{
  ADD_REQUEST,
  REMOVE_REQUEST,
  LAST_SIGNAL
};

static guint propono_handler_signals[LAST_SIGNAL] = { 0, };

typedef struct _ProponoHandlerPrivate ProponoHandlerPrivate;
struct _ProponoHandlerPrivate
{
  GList                     *channels;
  GHashTable                *requests;
  GPtrArray                 *channel_filter;
  gboolean                   bypass_approval;
  ProponoHandlerHandleChannelsCb  callback;
};

typedef struct
{
  ProponoHandler              *self;
  TpAccount              *account;
  TpConnection           *connection;
  TpChannel             **channels;
  TpChannelRequest      **requests_satisfied;
  guint64                 user_action_time;
  GHashTable             *handler_info;
  DBusGMethodInvocation  *context;
} HandleChannelsCall;

static TpChannelRequest *
lookup_channel_request (ProponoHandler  *self,
                        const char *request_path,
                        GHashTable *properties)
{
  ProponoHandlerPrivate *priv = GET_PRIVATE (self);
  TpChannelRequest *request;

  request = g_hash_table_lookup (priv->requests, request_path);

  if (request == NULL)
    {
      TpDBusDaemon *bus;
      GError *error = NULL;

      /* we need to create the request */
      DEBUG ("Creating Channel Request\n");

      bus = tp_dbus_daemon_dup (NULL);
      request = tp_channel_request_new (bus, request_path, properties, &error);
      if (error != NULL)
        {
          g_error ("%s", error->message);
        }

      g_hash_table_insert (priv->requests, g_strdup (request_path), request);
      g_object_unref (bus);
    }

  return g_object_ref (request);
}

static void
handle_channels_call_maybe (HandleChannelsCall *call)
{
  ProponoHandlerPrivate *priv = GET_PRIVATE (call->self);

  /* if all of our D-Bus objects are ready, call the callback */
  if (!tp_account_is_prepared (call->account, TP_ACCOUNT_FEATURE_CORE) ||
      !tp_connection_is_ready (call->connection))
    {
      return;
    }

    {
      TpChannel **ptr;

      for (ptr = call->channels; *ptr; ptr++)
        {
          if (!tp_channel_is_ready (*ptr))
            {
              return;
            }
        }
    }

  DEBUG ("Ready to make callback");

  if ((* priv->callback) (call->self, call->account, call->connection,
                          call->channels, call->requests_satisfied,
                          call->user_action_time, call->handler_info))
    {
      tp_svc_client_handler_return_from_handle_channels (call->context);
    }
  else
    {
      GError error = { TP_ERRORS, TP_ERROR_NOT_AVAILABLE,
                       "Handler failed to accept channels" };

      DEBUG ("Callback failed, returning NotAvailable");

      dbus_g_method_return_error (call->context, &error);
    }

  /* release refs/free mem */
  g_object_unref (call->account);
  g_object_unref (call->connection);
  /* we don't need to unref each channel, because it's still referenced by
   * priv->channels */
  g_free (call->channels);

    {
      TpChannelRequest **ptr;

      for (ptr = call->requests_satisfied; *ptr; ptr++)
        {
          /* the request is no longer pending, drop it from the map */
          g_hash_table_remove (priv->requests, tp_proxy_get_object_path (
                TP_PROXY (*ptr)));

          g_object_unref (*ptr);
        }
    }
  g_free (call->requests_satisfied);

  g_hash_table_unref (call->handler_info);

  g_slice_free (HandleChannelsCall, call);
}

static void
account_ready_cb (GObject      *account,
                  GAsyncResult *res,
                  gpointer      user_data)
{
  GError *error = NULL;

  tp_account_prepare_finish (TP_ACCOUNT (account), res, &error);
  if (error != NULL)
    {
      g_error ("%s", error->message);
    }

  DEBUG ("Account ready");

  handle_channels_call_maybe (user_data);
}

static void
connection_ready_cb (TpConnection *connection,
                     const GError *error,
                     gpointer      user_data)
{
  if (error != NULL)
    {
      g_error ("%s", error->message);
    }

  DEBUG ("Connection Ready");

  handle_channels_call_maybe (user_data);
}

static void
channel_closed_cb (TpChannel *channel,
                   gpointer   user_data,
                   GObject   *self)
{
  ProponoHandlerPrivate *priv = GET_PRIVATE (self);

  DEBUG ("Channel Closed: %s", tp_proxy_get_object_path (channel));

  /* remove this channel from the list of channels we're handling */
  priv->channels = g_list_remove (priv->channels, channel);
  g_object_unref (channel);
}

static void
channel_ready_cb (TpChannel    *channel,
                  const GError *error,
                  gpointer      user_data)
{
  HandleChannelsCall *call = user_data;
  ProponoHandler *self = PROPONO_HANDLER (call->self);
  ProponoHandlerPrivate *priv = GET_PRIVATE (self);
  GError *error2 = NULL;

  if (error != NULL)
    {
      g_error ("%s", error->message);
    }

  DEBUG ("Channel Ready: %s", tp_proxy_get_object_path (channel));

  /* put this channel into the list of channels we're handling */
  priv->channels = g_list_prepend (priv->channels, channel);
  tp_cli_channel_connect_to_closed (channel, channel_closed_cb,
      NULL, NULL, G_OBJECT (self), &error2);
  if (error2 != NULL)
    {
      g_error ("%s", error->message);
    }

  handle_channels_call_maybe (call);
}

static void
propono_handler_handle_channels (TpSvcClientHandler   *self,
                            const char            *account_path,
                            const char            *connection_path,
                            const GPtrArray       *channels,
                            const GPtrArray       *requests_satisfied,
                            guint64                user_action_time,
                            GHashTable            *handler_info,
                            DBusGMethodInvocation *context)
{
  TpDBusDaemon *bus;
  HandleChannelsCall *call;
  GError *error = NULL;
  guint i;

  DEBUG ("HandleChannels called");

  call = g_slice_new0 (HandleChannelsCall);
  call->self = PROPONO_HANDLER (self);

  bus = tp_dbus_daemon_dup (&error);
  if (error != NULL)
    {
      g_error ("%s", error->message);
    }

  call->account = tp_account_new (bus, account_path, &error);
  if (error != NULL)
    {
      g_error ("%s", error->message);
    }
  tp_account_prepare_async (call->account, NULL, account_ready_cb, call);

  call->connection = tp_connection_new (bus, NULL, connection_path, &error);
  if (error != NULL)
    {
      g_error ("%s", error->message);
    }
  tp_connection_call_when_ready (call->connection, connection_ready_cb, call);

  call->channels = g_new0 (TpChannel *, channels->len + 1);
  for (i = 0; i < channels->len; i++)
    {
      GValueArray *array = g_ptr_array_index (channels, i);
      const char *channel_path = g_value_get_boxed (
          g_value_array_get_nth (array, 0));
      GHashTable *map = g_value_get_boxed (
          g_value_array_get_nth (array, 1));

      call->channels[i] = tp_channel_new_from_properties (call->connection,
          channel_path, map, &error);
      if (error != NULL)
        {
          g_error ("%s", error->message);
        }
      tp_channel_call_when_ready (call->channels[i], channel_ready_cb, call);
    }

  call->requests_satisfied = g_new0 (TpChannelRequest *,
      requests_satisfied->len + 1);
  for (i = 0; i < requests_satisfied->len; i++)
    {
      const char *request_path = g_ptr_array_index (requests_satisfied, i);

      call->requests_satisfied[i] = lookup_channel_request (PROPONO_HANDLER (self),
          request_path, NULL);
    }

  call->user_action_time = user_action_time;
  call->handler_info = g_hash_table_ref (handler_info);
  call->context = context;

  g_object_unref (bus);
}

static void
propono_handler_add_request (TpSvcClientInterfaceRequests *self,
                        const char                   *request_path,
                        GHashTable                   *properties,
                        DBusGMethodInvocation        *context)
{
  TpChannelRequest *request;

  DEBUG ("AddRequest called: %s", request_path);

  request = lookup_channel_request (PROPONO_HANDLER (self),
      request_path, properties);

  g_signal_emit (self, propono_handler_signals[ADD_REQUEST], 0, request, properties);
  tp_svc_client_interface_requests_return_from_add_request (context);

  g_object_unref (request);
}

static void
propono_handler_remove_request (TpSvcClientInterfaceRequests *self,
                           const char                   *request_path,
                           const char                   *error_name,
                           const char                   *message,
                           DBusGMethodInvocation        *context)
{
  ProponoHandlerPrivate *priv = GET_PRIVATE (self);
  TpChannelRequest *request;

  DEBUG ("RemoveRequest called: %s", request_path);

  request = lookup_channel_request (PROPONO_HANDLER (self), request_path, NULL);
  /* the request is no longer pending, drop it from the map */
  g_hash_table_remove (priv->requests, request_path);

  g_signal_emit (self, propono_handler_signals[REMOVE_REQUEST], 0, request,
      error_name, message);
  tp_svc_client_interface_requests_return_from_remove_request (context);

  g_object_unref (request);
}

static void
propono_handler_get_property (GObject    *self,
                         guint       property_id,
                         GValue     *value,
                         GParamSpec *pspec)
{
  ProponoHandlerPrivate *priv = GET_PRIVATE (self);

  switch (property_id)
    {
      case PROP_INTERFACES:
        g_value_set_boxed (value, client_interfaces);
        break;

      case PROP_CHANNEL_FILTER:
        g_value_set_boxed (value, priv->channel_filter);
        break;

      case PROP_BYPASS_APPROVAL:
        g_value_set_boolean (value, priv->bypass_approval);
        break;

      case PROP_HANDLED_CHANNELS:
        /* the Telepathy spec says that HandledChannels should report the
         * union of all Handlers sharing a unique name, but we are deciding to
         * ignore this, because it's a pain in the neck to implement correctly
         * (how do you deal with people not using ProponoHandler, or how do you
         * deal with people not even using tp-glib).
         *
         * This is filed as bug
         * https://bugs.freedesktop.org/show_bug.cgi?id=25286 */
          {
            GPtrArray *array = g_ptr_array_new ();
            GList *ptr;

            for (ptr = priv->channels; ptr; ptr = ptr->next)
              {
                g_ptr_array_add (array,
                    g_strdup (tp_proxy_get_object_path (TP_PROXY (ptr->data))));
              }

            g_value_take_boxed (value, array);
          }

        break;

      case PROP_HANDLE_CHANNELS_CB:
        g_value_set_pointer (value, priv->callback);
        break;

      default:
        G_OBJECT_WARN_INVALID_PROPERTY_ID (self, property_id, pspec);
        break;
    }
}

static void
propono_handler_set_property (GObject      *self,
                         guint         property_id,
                         const GValue *value,
                         GParamSpec   *pspec)
{
  ProponoHandlerPrivate *priv = GET_PRIVATE (self);

  switch (property_id)
    {
      case PROP_CHANNEL_FILTER:
        priv->channel_filter = g_value_dup_boxed (value);
        break;

      case PROP_BYPASS_APPROVAL:
        priv->bypass_approval = g_value_get_boolean (value);
        break;

      case PROP_HANDLE_CHANNELS_CB:
        priv->callback = g_value_get_pointer (value);
        break;

      default:
        G_OBJECT_WARN_INVALID_PROPERTY_ID (self, property_id, pspec);
        break;
    }
}

static void
propono_handler_dispose (GObject *self)
{
  ProponoHandlerPrivate *priv = GET_PRIVATE (self);
  GList *ptr;
  guint i;

  for (ptr = priv->channels; ptr; ptr = ptr->next)
    {
      g_object_unref (ptr->data);
    }
  g_list_free (priv->channels);
  priv->channels = NULL;

  if (priv->requests)
    {
      g_hash_table_destroy (priv->requests);
      priv->requests = NULL;
    }

  /* free the filter we dupped */
  for (i = 0; i < priv->channel_filter->len; i++)
    {
      g_hash_table_destroy (g_ptr_array_index (priv->channel_filter, i));
    }
  g_ptr_array_free (priv->channel_filter, TRUE);
  priv->channel_filter = NULL;

  G_OBJECT_CLASS (propono_handler_parent_class)->dispose (self);
}

static void
propono_handler_class_init (ProponoHandlerClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  static TpDBusPropertiesMixinPropImpl client_props[] = {
        { "Interfaces", "interfaces", NULL },
        { NULL }
  };

  static TpDBusPropertiesMixinPropImpl client_handler_props[] = {
        { "HandlerChannelFilter", "channel-filter", NULL },
        { "BypassApproval", "bypass-approval", NULL },
        { "HandledChannels", "handled-channels", NULL },
        { NULL }
  };

  static TpDBusPropertiesMixinIfaceImpl prop_interfaces[] = {
        { TP_IFACE_CLIENT,
          tp_dbus_properties_mixin_getter_gobject_properties,
          NULL,
          client_props
        },
        { TP_IFACE_CLIENT_HANDLER,
          tp_dbus_properties_mixin_getter_gobject_properties,
          NULL,
          client_handler_props
        },
        { NULL }
  };

  object_class->get_property = propono_handler_get_property;
  object_class->set_property = propono_handler_set_property;
  object_class->dispose      = propono_handler_dispose;

  g_object_class_install_property (object_class, PROP_INTERFACES,
      g_param_spec_boxed ("interfaces",
                          "Interfaces",
                          "Available D-Bus Interfaces",
                          G_TYPE_STRV,
                          G_PARAM_READABLE | G_PARAM_STATIC_STRINGS));

  /**
   * ProponoHandler:channel-filter
   *
   * Filter for channels we handle, matches the HandlerChannelFilter property.
   * A #GPtrArray of a{sv} #GHashTable maps for the types of channels that
   * can be dispatched to this client.
   *
   * <example><programlisting>
   * GPtrArray *filter;
   * GHashTable *map;
   *
   * filter = g_ptr_array_new ();
   * map = tp_asv_new (
   *       TP_IFACE_CHANNEL ".ChannelType", G_TYPE_STRING, TP_IFACE_CHANNEL_TYPE_DBUS_TUBE,
   *       TP_IFACE_CHANNEL ".TargetHandleType", G_TYPE_UINT, TP_HANDLE_TYPE_ROOM,
   *       TP_IFACE_CHANNEL ".Requested", G_TYPE_BOOLEAN, FALSE,
   *       TP_IFACE_CHANNEL_TYPE_DBUS_TUBE ".ServiceName", G_TYPE_STRING, SERVICE_NAME,
   *       NULL
   *     );
   *  g_ptr_array_add (filter, map);
   * </programlisting></example>
   */
  g_object_class_install_property (object_class, PROP_CHANNEL_FILTER,
      g_param_spec_boxed ("channel-filter",
                          "Channel Filter",
                          "Filter for channels we handle",
                          TP_ARRAY_TYPE_CHANNEL_CLASS_LIST,
                          G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY));

  g_object_class_install_property (object_class, PROP_BYPASS_APPROVAL,
      g_param_spec_boolean ("bypass-approval",
                            "Bypass Approval",
                            "Whether or not this Client should bypass approval",
                            FALSE,
                            G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY));

  g_object_class_install_property (object_class, PROP_HANDLED_CHANNELS,
      g_param_spec_boxed ("handled-channels",
                          "Handled Channels",
                          "List of channels we're handling",
                          TP_ARRAY_TYPE_OBJECT_PATH_LIST,
                          G_PARAM_READABLE));

  g_object_class_install_property (object_class, PROP_HANDLE_CHANNELS_CB,
      g_param_spec_pointer ("handle-channels-cb",
                            "Handle Channels Callback",
                            "Callback to pass HandleChannels to",
                            G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY));

  /**
   * ProponoHandler::add-request:
   * @self: the handler
   * @request: the incoming request
   * @properties: immutable properties for the request (an a{sv} map)
   *
   * Emitted in response to the AddRequest call being made on this Handler
   * by the Channel Dispatcher. The AddRequest method is used by the Channel
   * Dispatcher to indicate a request that you <emphasis>may</emphasis> be
   * asked to handle, but this isn't guaranteed. This signal is provided so
   * that your user interface can provide some pending notification, etc.
   *
   * If you are asked to handle the channels, @request will be passed to your
   * callback as a satisfied request. If another Handler ends up handling the
   * channels, or the request dies for some reason, @request will be passed in
   * ProponoHandler::remove-request.
   */
  propono_handler_signals[ADD_REQUEST] = g_signal_new ("add-request",
      G_OBJECT_CLASS_TYPE (klass),
      G_SIGNAL_RUN_LAST,
      G_STRUCT_OFFSET (ProponoHandlerClass, add_request),
      NULL, NULL,
      _propono_marshal_VOID__OBJECT_BOXED,
      G_TYPE_NONE, 2,
      TP_TYPE_CHANNEL_REQUEST, TP_HASH_TYPE_QUALIFIED_PROPERTY_VALUE_MAP);

  /**
   * ProponoHandler::remove-request:
   * @self: the handler
   * @request: the removed request
   * @error: the D-Bus name of an error, why this request was lost
   * @message: a more informative message
   *
   * Emitted in response to the RemoveRequest call being made on this Handler
   * by the Channel Dispatcher. The RemoveRequest method is used by the
   * Channel Dispatcher to indicate a request that you were previously
   * notified about through ProponoHandler::add-request.
   *
   * @error gives the name of a D-Bus error for why the request was lost. The
   * error <literal>org.freedesktop.Telepathy.Error.NotYours</literal> means
   * that the channels were given to another handler.
   */
  propono_handler_signals[REMOVE_REQUEST] = g_signal_new ("remove-request",
      G_OBJECT_CLASS_TYPE (klass),
      G_SIGNAL_RUN_LAST,
      G_STRUCT_OFFSET (ProponoHandlerClass, remove_request),
      NULL, NULL,
      _propono_marshal_VOID__OBJECT_BOXED,
      G_TYPE_NONE, 3,
      TP_TYPE_CHANNEL_REQUEST, G_TYPE_STRING, G_TYPE_STRING);

  /* call our mixin class init */
  klass->dbus_props_class.interfaces = prop_interfaces;
  tp_dbus_properties_mixin_class_init (object_class,
      G_STRUCT_OFFSET (ProponoHandlerClass, dbus_props_class));

  g_type_class_add_private (klass, sizeof (ProponoHandlerPrivate));
}

static void
propono_handler_init (ProponoHandler *self)
{
  ProponoHandlerPrivate *priv = GET_PRIVATE (self);

  priv->requests = g_hash_table_new_full (g_str_hash, g_str_equal,
                                          g_free, g_object_unref);
}

static void
handler_iface_init (gpointer g_iface, gpointer iface_data)
{
  TpSvcClientHandlerClass *klass = (TpSvcClientHandlerClass *) g_iface;

#define IMPLEMENT(x) tp_svc_client_handler_implement_##x (klass, \
    propono_handler_##x)
  IMPLEMENT (handle_channels);
#undef IMPLEMENT
}

static void
requests_iface_init (gpointer g_iface, gpointer iface_data)
{
  TpSvcClientInterfaceRequestsClass *klass = g_iface;

#define IMPLEMENT(x) tp_svc_client_interface_requests_implement_##x (klass, \
    propono_handler_##x)
  IMPLEMENT (add_request);
  IMPLEMENT (remove_request);
#undef IMPLEMENT
}

/**
 * propono_handler_new:
 * @channel_filter: a #GPtrArray of #GHashTable a{sv} maps defining
 * HandleChannelFilter
 * @bypass_approval: whether or not this Handler bypasses any Approvers
 * @handle_channels_cb: the function to call to handle dispatched channels
 *
 * Creates a new Client.Handler object that can be published on the session bus
 * with dbus-glib. The parameters @channel_filter and @bypass_approval are set
 * when the object is published and cannot be changed afterwards.
 *
 * For further information consult the <link url="http://telepathy.freedesktop.org/spec/org.freedesktop.Telepathy.Client.Handler.html">Telepathy spec</link>.
 *
 * Returns: a #ProponoHandler ready to be published on the session bus.
 */
ProponoHandler *
propono_handler_new (GPtrArray                 *channel_filter,
                gboolean                   bypass_approval,
                ProponoHandlerHandleChannelsCb  handle_channels_cb)
{
  g_return_val_if_fail (channel_filter != NULL, NULL);
  g_return_val_if_fail (handle_channels_cb != NULL, NULL);

  return g_object_new (TP_TYPE_HANDLER,
      "channel-filter", channel_filter,
      "bypass-approval", bypass_approval,
      "handle-channels-cb", handle_channels_cb,
      NULL);
}

/**
 * propono_handler_get_pending_requests:
 * @self: the handler
 *
 * Retrieves a list of all pending #TpChannelRequest objects for this handler.
 * The addition of channel requests is notified via ProponoHandler::add-request and
 * resolved by ProponoHandler::remove-request and the ProponoHandler:handle-channels-cb
 * callback.
 *
 * Returns: a #GList of #TpChannelRequest objects, owned by the Handler, do
 * not unreference. Free list with g_list_free() when done.
 */
GList *
propono_handler_get_pending_requests (ProponoHandler *self)
{
  ProponoHandlerPrivate *priv;

  g_return_val_if_fail (TP_IS_HANDLER (self), NULL);

  priv = GET_PRIVATE (self);

  return g_hash_table_get_values (priv->requests);
}
