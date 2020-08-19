/*
 * Copyright (C) 2011-2012 Igalia S.L.
 * Copyright (C) 2011 Intel Corporation.
 * Copyright (C) 2015 Collabora Ltd.
 *
 * Contact: Iago Toral Quiroga <itoral@igalia.com>
 *
 * Authors: Lionel Landwerlin <lionel.g.landwerlin@linux.intel.com>
 *          Juan A. Suarez Romero <jasuarez@igalia.com>
 *          Xavier Claessens <xavier.claessens@collabora.com>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public License
 * as published by the Free Software Foundation; version 2.1 of
 * the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA
 * 02110-1301 USA
 *
 */

#include <libtracker-sparql/tracker-sparql.h>

#include "grl-tracker.h"
#include "grl-tracker-source-notif.h"
#include "grl-tracker-source-priv.h"
#include "grl-tracker-utils.h"

#define GRL_LOG_DOMAIN_DEFAULT tracker_notif_log_domain
GRL_LOG_DOMAIN_STATIC(tracker_notif_log_domain);

struct _GrlTrackerSourceNotify {
  GObject parent;
  TrackerSparqlConnection *connection;
  TrackerNotifier *notifier;
  GrlSource *source;
  guint events_signal_id;
};

enum {
  PROP_0,
  PROP_CONNECTION,
  PROP_SOURCE,
  N_PROPS
};

static GParamSpec *props[N_PROPS] = { 0, };

G_DEFINE_TYPE (GrlTrackerSourceNotify, grl_tracker_source_notify, G_TYPE_OBJECT)

static GrlMedia *
media_for_event (GrlTrackerSourceNotify *self,
                 TrackerNotifierEvent   *event)
{
  gchar *id_str;
  GrlMedia *media;

  id_str = g_strdup_printf ("%" G_GINT64_FORMAT, tracker_notifier_event_get_id (event));
  // FIXME
  media = grl_tracker_build_grilo_media (NULL,//tracker_notifier_event_get_type (event),
                                         GRL_TYPE_FILTER_NONE);
  grl_media_set_id (media, id_str);
  grl_media_set_url (media, tracker_notifier_event_get_urn (event));

  g_free (id_str);

  return media;
}

static void
handle_changes (GrlTrackerSourceNotify   *self,
                GPtrArray                *events,
                TrackerNotifierEventType  tracker_type,
                GrlSourceChangeType       change_type)
{
  TrackerNotifierEvent *event;
  GPtrArray *change_list;
  GrlMedia *media;
  gint i;

  change_list = g_ptr_array_new ();

  for (i = 0; i < events->len; i++) {
    event = g_ptr_array_index (events, i);
    if (tracker_notifier_event_get_event_type (event) != tracker_type)
      continue;

    media = media_for_event (self, event);
    g_ptr_array_add (change_list, media);
  }

  grl_source_notify_change_list (self->source, change_list,
                                 change_type, FALSE);
}

static void
notifier_event_cb (GrlTrackerSourceNotify *self,
                   const gchar            *service,
                   const gchar            *graph,
                   GPtrArray              *events,
                   gpointer                user_data)
{
  handle_changes (self, events,
                  TRACKER_NOTIFIER_EVENT_CREATE,
                  GRL_CONTENT_ADDED);
  handle_changes (self, events,
                  TRACKER_NOTIFIER_EVENT_UPDATE,
                  GRL_CONTENT_CHANGED);
  handle_changes (self, events,
                  TRACKER_NOTIFIER_EVENT_DELETE,
                  GRL_CONTENT_REMOVED);
}

static void
grl_tracker_source_notify_get_property (GObject    *object,
                                        guint       prop_id,
                                        GValue     *value,
                                        GParamSpec *pspec)
{
  GrlTrackerSourceNotify *self = GRL_TRACKER_SOURCE_NOTIFY (object);

  switch (prop_id) {
  case PROP_CONNECTION:
    g_value_set_object (value, self->connection);
    break;
  case PROP_SOURCE:
    g_value_set_object (value, self->source);
    break;
  default:
    G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
    break;
  }
}

static void
grl_tracker_source_notify_set_property (GObject      *object,
                                        guint         prop_id,
                                        const GValue *value,
                                        GParamSpec   *pspec)
{
  GrlTrackerSourceNotify *self = GRL_TRACKER_SOURCE_NOTIFY (object);

  switch (prop_id) {
  case PROP_CONNECTION:
    self->connection = g_value_get_object (value);
    break;
  case PROP_SOURCE:
    self->source = g_value_get_object (value);
    break;
  default:
    G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
    break;
  }
}

static void
grl_tracker_source_notify_constructed (GObject *object)
{
  GrlTrackerSourceNotify *self = GRL_TRACKER_SOURCE_NOTIFY (object);

  self->notifier =
    tracker_sparql_connection_create_notifier (self->connection);
  self->events_signal_id =
    g_signal_connect_swapped (self->notifier, "events",
                              G_CALLBACK (notifier_event_cb), object);

  G_OBJECT_CLASS (grl_tracker_source_notify_parent_class)->constructed (object);
}

static void
grl_tracker_source_notify_finalize (GObject *object)
{
  GrlTrackerSourceNotify *self = GRL_TRACKER_SOURCE_NOTIFY (object);

  if (self->events_signal_id)
    g_signal_handler_disconnect (self->notifier, self->events_signal_id);
  g_clear_object (&self->notifier);
  G_OBJECT_CLASS (grl_tracker_source_notify_parent_class)->finalize (object);
}

static void
grl_tracker_source_notify_class_init (GrlTrackerSourceNotifyClass *klass)
{
  GObjectClass *object_class = (GObjectClass *) klass;

  GRL_LOG_DOMAIN_INIT (tracker_notif_log_domain, "tracker-notif");
  object_class->set_property = grl_tracker_source_notify_set_property;
  object_class->get_property = grl_tracker_source_notify_get_property;
  object_class->finalize = grl_tracker_source_notify_finalize;
  object_class->constructed = grl_tracker_source_notify_constructed;

  props[PROP_CONNECTION] =
    g_param_spec_object ("connection",
                         "SPARQL Connection",
                         "SPARQL Connection",
                         TRACKER_TYPE_SPARQL_CONNECTION,
                         G_PARAM_READWRITE |
                         G_PARAM_CONSTRUCT_ONLY |
                         G_PARAM_STATIC_STRINGS);
  props[PROP_SOURCE] =
    g_param_spec_object ("source",
                         "Source",
                         "Source being notified",
                         GRL_TYPE_SOURCE,
                         G_PARAM_READWRITE |
                         G_PARAM_CONSTRUCT_ONLY |
                         G_PARAM_STATIC_STRINGS);

  g_object_class_install_properties (object_class, N_PROPS, props);
}

static void
grl_tracker_source_notify_init (GrlTrackerSourceNotify *self)
{
}

GrlTrackerSourceNotify *
grl_tracker_source_notify_new (GrlSource               *source,
                               TrackerSparqlConnection *sparql_conn)
{
  return g_object_new (GRL_TRACKER_TYPE_SOURCE_NOTIFY,
                       "source", source,
                       "connection", sparql_conn,
                       NULL);
}