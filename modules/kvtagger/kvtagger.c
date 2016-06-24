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

#include "csv-tagger-scanner.h"
#include "kvtagger.h"
#include "logmsg/logmsg.h"
#include "logpipe.h"
#include "parser/parser-expr.h"
#include "reloc.h"
#include "tagger-scanner.h"
#include "template/templates.h"

#include <stdio.h>
#include <string.h>

typedef struct element_range
{
  gsize offset;
  gsize length;
} element_range;

typedef struct KVTagger
{
  LogParser super;
  GArray *nv_array;
  GHashTable *tag_record_store;
  LogTemplate *selector_template;
  gchar *filename;
  TagRecordScanner *scanner;
} KVTagger;

void
kvtagger_set_filename(LogParser *p, const gchar *filename)
{
  KVTagger *self = (KVTagger *)p;

  g_free(self->filename);
  self->filename = g_strdup(filename);
}

void
kvtagger_set_database_selector_template(LogParser *p, const gchar *selector)
{
  KVTagger *self = (KVTagger *)p;

  log_template_compile(self->selector_template, selector, NULL);
}

static inline element_range const *
kvtagger_lookup_tag(KVTagger *const self, const gchar *const key)
{
  return g_hash_table_lookup(self->tag_record_store, key);
}

static gboolean
kvtagger_parser_process(LogParser *s, LogMessage **pmsg, const LogPathOptions *path_options,
                        const gchar *input, gsize input_len)
{
  KVTagger *self = (KVTagger *)s;
  LogMessage *msg = log_msg_make_writable(pmsg, path_options);

  GString *str = g_string_new(NULL);

  log_template_format(self->selector_template, msg, NULL, LTZ_LOCAL, 0, NULL, str);

  const element_range *position_of_tags_to_be_inserted = kvtagger_lookup_tag(self, str->str);

  if (position_of_tags_to_be_inserted != NULL)
    {
      for (gsize i = position_of_tags_to_be_inserted->offset;
           i < position_of_tags_to_be_inserted->offset + position_of_tags_to_be_inserted->length;
           ++i)
        {
          tag_record matching_tag = g_array_index(self->nv_array, tag_record, i);
          log_msg_set_value_by_name(msg, matching_tag.name, matching_tag.value, -1);
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
  cloned->tag_record_store = g_hash_table_ref(self->tag_record_store);

  return &cloned->super.super;
}

static void
kvtagger_parser_free(LogPipe *s)
{
  KVTagger *self = (KVTagger *)s;

  g_free(self->filename);
  g_free(self->scanner);
  log_parser_free_method(s);
}

static gint
_kv_comparer(gconstpointer k1, gconstpointer k2)
{
  tag_record* r1 = (tag_record *) k1;
  tag_record* r2 = (tag_record *) k2;
  return strcmp(r1->selector, r2->selector);
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
      gchar *absolute_path = _complete_relative_path_with_config_path(filename);
      f = fopen(absolute_path, "r");
      g_free(absolute_path);
    }
  else
    {
      f = fopen(filename, "r");
    }

  return f;
}


static GArray *
_parse_input_file(KVTagger *self, FILE *file)
{
  GArray* nv_array = self->scanner->get_parsed_records(self->scanner, file);

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
  GHashTable *tag_record_store = g_hash_table_new_full(g_str_hash, g_str_equal, NULL, g_free);

  if (record_array->len > 0)
    {
      gsize range_start = 0;
      tag_record range_start_tag = g_array_index(record_array, tag_record, 0);

      for (gsize i = 1; i < record_array->len; ++i)
        {
          tag_record current_record = g_array_index(record_array, tag_record, i);

          if (_kv_comparer(&range_start_tag, &current_record))
            {
              element_range *current_range = g_new(element_range, 1);
              current_range->offset = range_start;
              current_range->length = i - range_start;

              g_hash_table_insert(tag_record_store, range_start_tag.selector, current_range);

              range_start_tag = current_record;
              range_start = i;
            }
        }

      {
        element_range *last_range = g_new(element_range, 1);
        last_range->offset = range_start;
        last_range->length = record_array->len - range_start;
        g_hash_table_insert(tag_record_store, range_start_tag.selector, last_range);
      }
    }

  return tag_record_store;
}

static gboolean
_prepare_scanner(KVTagger *self)
{
  gsize filename_len = strlen(self->filename);
  if (filename_len >= 3)
    {
      gsize diff = filename_len - 3;
      const gchar* extension = self->filename+diff;
      if (strcmp(extension, "csv") == 0)
        {

          GlobalConfig *cfg = log_pipe_get_config(&self->super.super);
          self->scanner = (TagRecordScanner *) csv_tagger_scanner_new(cfg);
        }
      else
        {
            return FALSE;
        }
    }
  else
    {
      return FALSE;
    }
   return TRUE;
}

static gboolean
kvtagger_create_lookup_table_from_file(KVTagger *self)
{
  if (!_prepare_scanner(self))
    {
      msg_error("Unknown file extension", evt_tag_str("filename", self->filename));
      return FALSE;
    }

  FILE *f = _open_data_file(self->filename);
  if (!f)
    {
      msg_error("Error loading kvtagger database", evt_tag_str("filename", self->filename));
      return FALSE;
    }

  self->nv_array = _parse_input_file(self, f);

  fclose(f);
  if (!self->nv_array)
    {
      msg_error("Error while parsing kvtagger database");
      return FALSE;
    }
  kvtagger_sort_array_by_key(self->nv_array);
  self->tag_record_store = kvtagger_classify_array(self->nv_array);

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

  g_hash_table_unref(self->tag_record_store);
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
  self->scanner = NULL;

  return &self->super;
}
