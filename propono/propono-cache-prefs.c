/*
 * propono-cache-prefs.c
 * This file is part of propono
 *
 * Copyright (C) Jonh Wendell 2010 <wendell@bani.com.br>
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

#include <glib/gi18n.h>
#include "propono-cache-prefs.h"

static GKeyFile *keyfile = NULL;
static char* filename = NULL;

void
propono_cache_prefs_init (void)
{
  keyfile = g_key_file_new ();
  filename = g_build_filename (g_get_user_cache_dir (),
                               "propono",
                               "propono-prefs-cache.ini",
                               NULL);

  g_key_file_load_from_file (keyfile, filename, 0, NULL);
}

static void
save_file (void)
{
  GError *error = NULL;
  gchar *data = g_key_file_to_data (keyfile, NULL, NULL);
  gchar *dir = g_build_filename (g_get_user_cache_dir (), "propono", NULL);

  g_mkdir_with_parents (dir, 0700);

  if (!g_file_set_contents (filename,
			    data,
			    -1,
			    &error))
    {
      g_warning (_("Error while saving preferences: %s"), error ? error->message: _("Unknown error"));
      g_clear_error (&error);
    }

  g_free (data);
  g_free (dir);
}

void
propono_cache_prefs_finalize (void)
{
  g_return_if_fail (keyfile != NULL);

  save_file ();

  g_key_file_free (keyfile);
  keyfile = NULL;

  g_free (filename);
  filename = NULL;
}

gboolean
propono_cache_prefs_get_boolean (const gchar *group, const gchar *key, gboolean default_value)
{
  gboolean result;
  GError *error = NULL;

  g_return_val_if_fail (keyfile != NULL, FALSE);

  result = g_key_file_get_boolean (keyfile, group, key, &error);
  if (error)
    {
      result = default_value;
      g_error_free (error);
    }

  return result;
}

void
propono_cache_prefs_set_boolean (const gchar *group, const gchar *key, gboolean value)
{
  g_return_if_fail (keyfile != NULL);

  g_key_file_set_boolean (keyfile, group, key, value);
}

gchar *
propono_cache_prefs_get_string (const gchar *group, const gchar *key, const gchar *default_value)
{
  gchar *result;
  GError *error = NULL;

  g_return_val_if_fail (keyfile != NULL, NULL);

  result = g_key_file_get_string (keyfile, group, key, &error);
  if (error)
    {
      result = g_strdup (default_value);
      g_error_free (error);
    }

  return result;
}

void
propono_cache_prefs_set_string (const gchar *group, const gchar *key, const gchar *value)
{
  g_return_if_fail (keyfile != NULL);

  g_key_file_set_string (keyfile, group, key, value);
}

gint
propono_cache_prefs_get_integer (const gchar *group, const gchar *key, gint default_value)
{
  gint result;
  GError *error = NULL;

  g_return_val_if_fail (keyfile != NULL, 0);

  result = g_key_file_get_integer (keyfile, group, key, &error);
  if (error)
    {
      result = default_value;
      g_error_free (error);
    }

  return result;
}

void
propono_cache_prefs_set_integer (const gchar *group, const gchar *key, gint value)
{
  g_return_if_fail (keyfile != NULL);

  g_key_file_set_integer (keyfile, group, key, value);
}
