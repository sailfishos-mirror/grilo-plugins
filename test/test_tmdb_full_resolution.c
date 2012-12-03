/*
 * Copyright (C) 2012 Openismus GmbH
 *
 * Author: Jens Georg <jensg@openismus.com>
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

#include <grilo.h>
#include "test_tmdb_utils.h"
#include <math.h>
#include <float.h>

#define TMDB_PLUGIN_ID "grl-tmdb"

/** Compare the floats.
 * A simple == will fail on values that are effectively the same,
 * due to rounding issues.
 */
static gboolean compare_floats(gfloat a, gfloat b)
{
   return fabs(a - b) < DBL_EPSILON;
}

#define DESCRIPTION \
"In a dynamic new portrayal of Arthur Conan Doyle’s most famous characters, “Sherlock Holmes” sends Holmes and his stalwart partner Watson on their latest challenge. Revealing fighting skills as lethal as his legendary intellect, Holmes will battle as never before to bring down a new nemesis and unravel a deadly plot that could destroy England."

static void
test (void)
{
  GError *error = NULL;
  GrlRegistry *registry;
  GrlKeyID backdrop, posters, imdb_id;
  GrlOperationOptions *options = NULL;
  GrlMedia *media = NULL;
  GDateTime *date, *orig;

  test_setup_tmdb ();

  registry = grl_registry_get_default ();
  backdrop = grl_registry_lookup_metadata_key (registry, "tmdb-backdrop");
  g_assert_cmpint (backdrop, !=, GRL_METADATA_KEY_INVALID);
  posters = grl_registry_lookup_metadata_key (registry, "tmdb-poster");
  g_assert_cmpint (posters, !=, GRL_METADATA_KEY_INVALID);
  imdb_id = grl_registry_lookup_metadata_key (registry, "tmdb-imdb-id");
  g_assert_cmpint (imdb_id, !=, GRL_METADATA_KEY_INVALID);

  media = grl_media_video_new ();
  g_assert (media != NULL);
  grl_media_set_title (media, "Sherlock Holmes");

  GrlSource *source = test_get_source();
  g_assert (source);
  options = grl_operation_options_new (NULL);
  g_assert (options != NULL);
  grl_source_resolve_sync (source,
                           media,
                           grl_source_supported_keys (source),
                           options,
                           &error);
  g_assert (error == NULL);

  /* Check if we got everything we need for the fast resolution */
  g_assert (compare_floats (grl_media_get_rating (media), 3.8f));
  g_assert_cmpstr (grl_data_get_string (GRL_DATA (media), GRL_METADATA_KEY_ORIGINAL_TITLE), ==,
                   "Sherlock Holmes");
  /* There's only one poster/backdrop in the search result */
  g_assert_cmpstr (grl_data_get_string (GRL_DATA (media), backdrop), ==,
                   "http://cf2.imgobject.com/t/p/original/uM414ugc1B910bTvGEIzsucfMMC.jpg");

  g_assert_cmpstr (grl_data_get_string (GRL_DATA (media), posters), ==,
                   "http://cf2.imgobject.com/t/p/original/22ngurXbLqab7Sko6aTSdwOCe5W.jpg");
  orig = g_date_time_new_utc (2009, 12, 25, 0, 0, 0.0);
  date = grl_media_get_publication_date (media);
  g_assert_cmpint (g_date_time_compare (orig, date), ==, 0);
  g_date_time_unref (orig);

  /* And now the slow properties */
  g_assert_cmpstr (grl_media_get_site (media), ==,
                   "http://sherlock-holmes-movie.warnerbros.com/");
  g_assert_cmpint (grl_data_length (GRL_DATA (media), GRL_METADATA_KEY_GENRE), ==, 6);
  g_assert_cmpint (grl_data_length (GRL_DATA (media), GRL_METADATA_KEY_STUDIO), ==, 3);

  g_assert_cmpstr (grl_media_get_description (media), ==, DESCRIPTION);

  /* TODO: See https://bugzilla.gnome.org/show_bug.cgi?id=679686#c13
  g_assert_cmpstr (grl_media_get_certificate (media), ==, "PG-13");
  */

  g_assert_cmpstr (grl_data_get_string (GRL_DATA (media), imdb_id), ==, "tt0988045");
  g_assert_cmpint (grl_data_length (GRL_DATA (media), GRL_METADATA_KEY_KEYWORD), ==, 15);

  g_assert_cmpint (grl_data_length (GRL_DATA (media), GRL_METADATA_KEY_PERFORMER), ==, 10);

  g_assert_cmpint (grl_data_length (GRL_DATA (media), GRL_METADATA_KEY_PRODUCER), ==, 9);

  g_assert_cmpint (grl_data_length (GRL_DATA (media), GRL_METADATA_KEY_DIRECTOR), ==, 1);
  g_assert_cmpstr (grl_data_get_string (GRL_DATA (media), GRL_METADATA_KEY_DIRECTOR), ==, "Guy Ritchie");

  /* TODO: See https://bugzilla.gnome.org/show_bug.cgi?id=679686#c13
  g_assert_cmpstr (grl_data_get_string (GRL_DATA (media), age_certs), ==,
                   "GB:12A;NL:12;BG:C;HU:16;DE:12;DK:15;US:PG-13");
  */

  g_object_unref (media);
  media = NULL;

  g_object_unref (options);
  options = NULL;

  test_shutdown_tmdb ();
}


int
main(int argc, char **argv)
{
  g_setenv ("GRL_PLUGIN_LIST", TMDB_PLUGIN_ID, TRUE);
  g_setenv ("GRL_PLUGIN_PATH", g_getenv ("PWD"), TRUE);

  /* We must set this before calling grl_init.
   * See https://bugzilla.gnome.org/show_bug.cgi?id=685967#c17
   */
  g_setenv ("GRL_NET_MOCKED", GRILO_PLUGINS_TESTS_TMDB_DATA_PATH "sherlock.ini", TRUE);

  grl_init (&argc, &argv);
#if !GLIB_CHECK_VERSION(2,32,0)
  g_thread_init (NULL);
#endif

  test ();
}
