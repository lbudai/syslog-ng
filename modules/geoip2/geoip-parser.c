/*
 * Copyright (c) 2017 Balabit
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License version 2 as published
 * by the Free Software Foundation, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
 *
 * As an additional exemption you are allowed to compile & link against the
 * OpenSSL libraries as published by the OpenSSL project. See the file
 * COPYING for details.
 *
 */

#include "geoip-parser.h"
#include "maxminddb-helper.h"

typedef struct _GeoIPParser GeoIPParser;

struct _GeoIPParser
{
  LogParser super;
  MMDB_s *gi;

  gchar *database;
  gchar *prefix;
};

void
geoip_parser_set_prefix(LogParser *s, const gchar *prefix)
{
  GeoIPParser *self = (GeoIPParser *) s;

  g_free(self->prefix);
  self->prefix = g_strdup(prefix);
}

void
geoip_parser_set_database(LogParser *s, const gchar *database)
{
  GeoIPParser *self = (GeoIPParser *) s;

  g_free(self->database);
  self->database = g_strdup(database);
}

void
mmdb_problem_to_debug(const int gai_error, const int mmdb_error, gchar *where)
{
  if (0 != gai_error)
    msg_debug("Error from call to getaddrinfo",
              evt_tag_str("gai error", gai_strerror(gai_error)),
              evt_tag_str("where", where));

  if (MMDB_SUCCESS != mmdb_error)
    msg_debug("maxminddb_error",
              evt_tag_str("error", MMDB_strerror(mmdb_error)),
              evt_tag_str("where", where));
}

static gboolean
maxminddb_parser_process(LogParser *s, LogMessage **pmsg,
                         const LogPathOptions *path_options,
                         const gchar *input, gsize input_len)
{
  GeoIPParser *self = (GeoIPParser *) s;
  LogMessage *msg = log_msg_make_writable(pmsg, path_options);

  int gai_error, mmdb_error;
  MMDB_lookup_result_s result =
    MMDB_lookup_string(self->gi, input, &gai_error, &mmdb_error);

  if (!result.found_entry)
    {
      mmdb_problem_to_debug(gai_error, mmdb_error, "lookup");
      return TRUE;
    }

  MMDB_entry_data_list_s *entry_data_list = NULL;
  mmdb_error = MMDB_get_entry_data_list(&result.entry, &entry_data_list);
  if (MMDB_SUCCESS != mmdb_error)
    {
      msg_debug("GeoIP2: MMDB_get_entry_data_list",
                evt_tag_str("error", MMDB_strerror(mmdb_error)));
      return TRUE;
    }

  GArray *path = g_array_new(TRUE, FALSE, sizeof(gchar *));
  g_array_append_val(path, self->prefix);

  gint status;
  dump_geodata_into_msg(msg, entry_data_list, path, &status);

  MMDB_free_entry_data_list(entry_data_list);
  g_array_free(path, TRUE);

  return TRUE;
}

static LogPipe *
maxminddb_parser_clone(LogPipe *s)
{
  GeoIPParser *self = (GeoIPParser *) s;
  GeoIPParser *cloned;

  cloned = (GeoIPParser *) maxminddb_parser_new(s->cfg);

  geoip_parser_set_database(&cloned->super, self->database);
  geoip_parser_set_prefix(&cloned->super, self->prefix);
  log_parser_set_template(&cloned->super, log_template_ref(self->super.template));

  return &cloned->super.super;
}

static void
maxminddb_parser_free(LogPipe *s)
{
  GeoIPParser *self = (GeoIPParser *) s;

  g_free(self->database);
  g_free(self->prefix);
  g_free(self->gi);

  log_parser_free_method(s);
}

static gboolean
maxminddb_parser_init(LogPipe *s)
{
  GeoIPParser *self = (GeoIPParser *) s;

  self->gi = g_new0(MMDB_s, 1);
  if (!self->gi)
    return FALSE;

  if (self->database)
    {
      int mmdb_status = MMDB_open(self->database, MMDB_MODE_MMAP, self->gi);
      if (mmdb_status != MMDB_SUCCESS)
        {
          msg_error("MMDB_open",
                    evt_tag_str("error", MMDB_strerror(mmdb_status)));
          return FALSE;
        }
    }

  g_assert(strlen(self->prefix));
  if (self->prefix[strlen(self->prefix)-1] == '.')
    self->prefix[strlen(self->prefix)-1] = 0;

  if (!self->gi)
    return FALSE;

  return log_parser_init_method(s);
}

LogParser *
maxminddb_parser_new(GlobalConfig *cfg)
{
  GeoIPParser *self = g_new0(GeoIPParser, 1);

  log_parser_init_instance(&self->super, cfg);
  self->super.super.init = maxminddb_parser_init;
  self->super.super.free_fn = maxminddb_parser_free;
  self->super.super.clone = maxminddb_parser_clone;
  self->super.process = maxminddb_parser_process;

  geoip_parser_set_prefix(&self->super, ".geoip2");

  return &self->super;
}
