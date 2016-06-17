/*
 * Copyright (c) 2002-2015 Balabit
 * Copyright (c) 1998-2015 Balázs Scheidler
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

#include "kvtagger.h"
#include "logmsg/logmsg.h"
#include "logpipe.h"
#include "parser/parser-expr.h"
#include "template/templates.h"
#include "reloc.h"
#include <glib.h>
#include <stdio.h>
#include <string.h>

typedef struct database_record
{
  const gchar *selector;
  const gchar *name;
  const gchar *value;
} database_record;

typedef struct element_range
{
  gsize offset;
  gsize length;
} element_range;

typedef struct KVTagger
{
  LogParser super;
  GArray *nv_array;
  GHashTable *selector_lookup_table;
  LogTemplate *selector_template;
  const gchar *filename;
} KVTagger;

void
kvtagger_set_filename(LogParser *p, const gchar *filename)
{
  KVTagger *self = (KVTagger *)p;

  g_free(self->filename);
  self->filename = g_strdup(filename);
}

void
kvtagger_set_database_selector_template(LogParser *p, const char *selector)
{
  KVTagger *self = (KVTagger *)p;

  log_template_compile(self->selector_template, selector, NULL);
}

static inline element_range const *
kvtagger_lookup_key(KVTagger *const self, const gchar *const key)
{
  return g_hash_table_lookup(self->selector_lookup_table, key);
}

static gboolean
kvtagger_parser_process(LogParser *s, LogMessage **pmsg, const LogPathOptions *path_options,
                        const gchar *input, gsize input_len)
{
  KVTagger *self = (KVTagger *)s;
  LogMessage *msg = log_msg_make_writable(pmsg, path_options);

  GString *str = g_string_new(NULL);

  log_template_format(self->selector_template, msg, NULL, LTZ_LOCAL, 0, NULL, str);

  const element_range *range_of_elements_to_be_injected = kvtagger_lookup_key(self, str->str);

  if (range_of_elements_to_be_injected != NULL)
    {
      for (gsize i = range_of_elements_to_be_injected->offset;
           i < range_of_elements_to_be_injected->offset + range_of_elements_to_be_injected->length;
           ++i)
        {
          database_record KV = g_array_index(self->nv_array, database_record, i);
          log_msg_set_value_by_name(msg, KV.name, KV.value, -1);
        }
    }

  return TRUE;
}

static LogPipe *
kvtagger_parser_clone(LogPipe *s)
{
  KVTagger *self = (KVTagger *)s;
  KVTagger *cloned = (KVTagger *)kvtagger_parser_new(s->cfg);

  cloned->super.template = log_template_ref(self->super.template);
  cloned->nv_array = g_array_ref(self->nv_array);
  cloned->selector_lookup_table = g_hash_table_ref(self->selector_lookup_table);

  return &cloned->super.super;
}

static void
kvtagger_parser_free(LogPipe *s)
{
  KVTagger *self = (KVTagger *)s;

  g_free(self->filename);
  log_parser_free_method(s);
}

static gint
_kv_comparer(const database_record *k1, const database_record *k2)
{
  return strcmp(k1->selector, k2->selector);
}

static gboolean
_is_relative_path(const gchar *filename)
{
  return (filename[0] != '/');
}

static gchar *
_complete_relative_path_with_config_path(const gchar *filename)
{
  return g_build_filename(get_installation_path_for(SYSLOG_NG_PATH_SYSCONFDIR), filename, NULL);
}

static FILE *
_open_data_file(const gchar *filename)
{
  FILE *f = NULL;

  if (_is_relative_path(filename))
    {
      gchar *completed_filename = _complete_relative_path_with_config_path(filename);
      f = fopen(completed_filename, "r");
      g_free(completed_filename);
    }
  else
    {
      f = fopen(filename, "r");
    }

  return f;
}

static const int NUMBER_OF_ITEMS_PER_LINE = 3;

static GArray *
_parse_csv_file(FILE *file)
{
  GArray *nv_array = g_array_new(FALSE, FALSE, sizeof(database_record));

  static gchar tablekey[1024];
  static gchar tagname[1024];
  static gchar tagvalue[1024];

  for (int i = 0; !feof(file); ++i)
    {
      if (fscanf(file, " %[^,] , %[^,] , %[^,\n] \n", tablekey, tagname, tagvalue) ==
          NUMBER_OF_ITEMS_PER_LINE)
        {
          database_record last_record = {
              .selector = g_strdup(tablekey),
              .name = g_strdup(tagname),
              .value = g_strdup(tagvalue)
          };

          g_array_append_val(nv_array, last_record);
        }
    }

  return nv_array;
}

static void
kvtagger_sort_array_by_key(GArray *array)
{
  g_array_sort(array, _kv_comparer);
}

static GHashTable *
kvtagger_classify_array(const GArray *const record_array)
{
  GHashTable *selector_lookup_table = g_hash_table_new_full(g_str_hash, g_str_equal, NULL, g_free);

  if (record_array->len > 0)
    {
      gsize range_start = 0;
      database_record range_start_kv = g_array_index(record_array, database_record, 0);

      for (gsize i = 1; i < record_array->len; ++i)
        {
          database_record current_record = g_array_index(record_array, database_record, i);

          if (_kv_comparer(&range_start_kv, &current_record))
            {
              element_range *current_range = g_new(element_range, 1);
              current_range->offset = range_start;
              current_range->length = i - range_start;

              g_hash_table_insert(selector_lookup_table, range_start_kv.selector, current_range);

              range_start_kv = current_record;
              range_start = i;
            }
        }

      {
        element_range *last_range = g_new(element_range, 1);
        last_range->offset = range_start;
        last_range->length = record_array->len - range_start;
        g_hash_table_insert(selector_lookup_table, range_start_kv.selector, last_range);
      }
    }

  return selector_lookup_table;
}

static gboolean
kvtagger_create_lookup_table_from_file(KVTagger *self)
{

  FILE *f = _open_data_file(self->filename);
  if (!f)
    {
      msg_error("Error loading kvtagger database", evt_tag_str("filename", self->filename));
      return FALSE;
    }

  self->nv_array = _parse_csv_file(f);

  fclose(f);

  kvtagger_sort_array_by_key(self->nv_array);
  self->selector_lookup_table = kvtagger_classify_array(self->nv_array);

  return TRUE;
}

static gboolean
kvtagger_parser_init(LogPipe *s)
{
  KVTagger *self = (KVTagger *)s;

  if (self->filename == NULL)
    return FALSE;

  return kvtagger_create_lookup_table_from_file(self) && log_parser_init_method(s);
}

static gboolean
kvtagger_parser_deinit(LogPipe *s)
{
  KVTagger *self = (KVTagger *)s;

  g_hash_table_unref(self->selector_lookup_table);
  g_array_unref(self->nv_array);

  return TRUE;
}

LogParser *
kvtagger_parser_new(GlobalConfig *cfg)
{
  KVTagger *self = g_new0(KVTagger, 1);

  log_parser_init_instance(&self->super, cfg);

  self->selector_template = log_template_new(cfg, NULL);

  self->super.process = kvtagger_parser_process;

  self->super.super.clone = kvtagger_parser_clone;
  self->super.super.deinit = kvtagger_parser_deinit;
  self->super.super.free_fn = kvtagger_parser_free;
  self->super.super.init = kvtagger_parser_init;

  return &self->super;
}
