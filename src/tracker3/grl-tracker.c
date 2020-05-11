/*
 * Copyright (C) 2011-2012 Igalia S.L.
 * Copyright (C) 2011 Intel Corporation.
 *
 * Contact: Iago Toral Quiroga <itoral@igalia.com>
 *
 * Authors: Juan A. Suarez Romero <jasuarez@igalia.com>
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

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <grilo.h>
#include <string.h>
#include <libtracker-sparql/tracker-sparql.h>

#include "grl-tracker.h"
#include "grl-tracker-source.h"
#include "grl-tracker-source-api.h"
#include "grl-tracker-source-notif.h"
#include "grl-tracker-request-queue.h"
#include "grl-tracker-utils.h"

/* --------- Logging  -------- */

#define GRL_LOG_DOMAIN_DEFAULT tracker_general_log_domain
GRL_LOG_DOMAIN_STATIC(tracker_general_log_domain);

/* --- Other --- */

gboolean grl_tracker3_plugin_init (GrlRegistry *registry,
                                   GrlPlugin *plugin,
                                   GList *configs);

/* ===================== Globals  ================= */

TrackerSparqlConnection *grl_tracker_connection = NULL;
GrlPlugin *grl_tracker_plugin;
GCancellable *grl_tracker_plugin_init_cancel = NULL;
GrlTrackerQueue *grl_tracker_queue = NULL;

/* tracker plugin config */
gboolean grl_tracker_show_documents    = FALSE;

/* =================== Tracker Plugin  =============== */

static void
init_sources (void)
{
  grl_tracker_setup_key_mappings ();

  grl_tracker_queue = grl_tracker_queue_new ();

  if (grl_tracker_connection != NULL) {
    grl_tracker_notify_init (grl_tracker_connection);

    grl_tracker_source_sources_init ();
  }
}

static void
tracker_new_connection_cb (GObject      *object,
                           GAsyncResult *res,
                           GrlPlugin    *plugin)
{
  GError *error = NULL;

  GRL_DEBUG ("%s", __FUNCTION__);

  grl_tracker_connection = tracker_sparql_connection_new_finish (res, &error);

  if (error) {
    GRL_INFO ("Could not get connection to Tracker: %s", error->message);
    g_error_free (error);
    return;
  }

  init_sources ();
}

gboolean
grl_tracker3_plugin_init (GrlRegistry *registry,
                          GrlPlugin *plugin,
                          GList *configs)
{
  GrlConfig *config;
  gint config_count;
  GFile *ontology;

  GRL_LOG_DOMAIN_INIT (tracker_general_log_domain, "tracker3-general");

  /* Initialize i18n */
  bindtextdomain (GETTEXT_PACKAGE, LOCALEDIR);
  bind_textdomain_codeset (GETTEXT_PACKAGE, "UTF-8");

  grl_tracker_source_init_requests ();

  grl_tracker_plugin = plugin;

  if (!configs) {
    GRL_INFO ("\tConfiguration not provided! Using default configuration.");
  } else {
    config_count = g_list_length (configs);
    if (config_count > 1) {
      GRL_INFO ("\tProvided %i configs, but will only use one", config_count);
    }

    config = GRL_CONFIG (configs->data);

    grl_tracker_show_documents =
      grl_config_get_boolean (config, "show-documents");
  }

  grl_tracker_plugin_init_cancel = g_cancellable_new ();
  ontology = tracker_sparql_get_ontology_nepomuk ();
  tracker_sparql_connection_new_async (TRACKER_SPARQL_CONNECTION_FLAGS_NONE,
                                       NULL,
                                       ontology,
                                       grl_tracker_plugin_init_cancel,
                                       (GAsyncReadyCallback) tracker_new_connection_cb,
                                       plugin);
  g_object_unref (ontology);

  return TRUE;
}

static void
grl_tracker3_plugin_deinit (GrlPlugin *plugin)
{
  g_cancellable_cancel (grl_tracker_plugin_init_cancel);
  g_clear_object (&grl_tracker_plugin_init_cancel);
  g_clear_object (&grl_tracker_connection);
}

static void
grl_tracker3_plugin_register_keys (GrlRegistry *registry,
                                   GrlPlugin   *plugin)
{
  grl_registry_register_metadata_key (grl_registry_get_default (),
                                      g_param_spec_string ("tracker-category",
                                                           "Tracker category",
                                                           "Category a media belongs to",
                                                           NULL,
                                                           G_PARAM_STATIC_STRINGS |
                                                           G_PARAM_READWRITE),
                                      GRL_METADATA_KEY_INVALID,
                                      NULL);
  grl_registry_register_metadata_key (grl_registry_get_default (),
                                      g_param_spec_string ("gibest-hash",
                                                           "Gibest hash",
                                                           "Gibest hash of the video file",
                                                           NULL,
                                                           G_PARAM_STATIC_STRINGS |
                                                           G_PARAM_READWRITE),
                                      GRL_METADATA_KEY_INVALID,
                                      NULL);
  grl_registry_register_metadata_key (grl_registry_get_default (),
                                      g_param_spec_string ("tracker-urn",
                                                           "Tracker URN",
                                                           "Universal resource number in Tracker's store",
                                                           NULL,
                                                           G_PARAM_STATIC_STRINGS |
                                                           G_PARAM_READWRITE),
                                      GRL_METADATA_KEY_INVALID,
                                      NULL);
}

GRL_PLUGIN_DEFINE (GRL_MAJOR,
                   GRL_MINOR,
                   GRL_TRACKER_PLUGIN_ID,
                   "Tracker3",
                   "A plugin for searching multimedia content using Tracker Miners 3.x",
                   "Igalia S.L.",
                   VERSION,
                   "LGPL",
                   "http://www.igalia.com",
                   grl_tracker3_plugin_init,
                   grl_tracker3_plugin_deinit,
                   grl_tracker3_plugin_register_keys);
