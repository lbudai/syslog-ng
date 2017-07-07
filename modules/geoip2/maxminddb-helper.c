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

#include "maxminddb-helper.h"
#include "scratch-buffers.h"
#include <logmsg/logmsg.h>
#include <messages.h>

void
append_mmdb_entry_data_to_gstring(GString *target, MMDB_entry_data_s *entry_data)
{

  switch (entry_data->type)
    {
    case MMDB_DATA_TYPE_UTF8_STRING:
      g_string_append_printf(target, "%.*s", entry_data->data_size, entry_data->utf8_string);
      break;
    case MMDB_DATA_TYPE_DOUBLE:
      g_string_append_printf(target, "%f", entry_data->double_value);
      break;
    case MMDB_DATA_TYPE_FLOAT:
      g_string_append_printf(target, "%f", entry_data->float_value);
      break;
    case MMDB_DATA_TYPE_UINT16:
      g_string_append_printf(target, "%u", entry_data->uint16);
      break;
    case MMDB_DATA_TYPE_UINT32:
      g_string_append_printf(target, "%u", entry_data->uint32);
      break;
    case MMDB_DATA_TYPE_INT32:
      g_string_append_printf(target, "%d", entry_data->int32);
      break;
    case MMDB_DATA_TYPE_UINT64:
      g_string_append_printf(target, "%lu", entry_data->uint64);
      break;
    case MMDB_DATA_TYPE_UINT128:
    case MMDB_DATA_TYPE_MAP:
    case MMDB_DATA_TYPE_BYTES:
    case MMDB_DATA_TYPE_ARRAY:
    case MMDB_DATA_TYPE_BOOLEAN:
      g_assert_not_reached();
    default:
      g_assert_not_reached();
    }
}

static MMDB_entry_data_list_s *
mmdb_skip_tree(MMDB_entry_data_list_s *entry_data_list, gint *status)
{

  switch (entry_data_list->entry_data.type)
    {
    case MMDB_DATA_TYPE_MAP:
    {
      guint32 size = entry_data_list->entry_data.data_size;

      for (entry_data_list = entry_data_list->next;
           size && entry_data_list; size--)
        {

          if (MMDB_DATA_TYPE_UTF8_STRING != entry_data_list->entry_data.type)
            {
              *status = MMDB_INVALID_DATA_ERROR;
              return NULL;
            }
          entry_data_list = entry_data_list->next;
          entry_data_list = mmdb_skip_tree(entry_data_list, status);
          if (MMDB_SUCCESS != *status)
            {
              return NULL;
            }
        }
    }
    break;
    case MMDB_DATA_TYPE_ARRAY:
    {
      uint32_t size = entry_data_list->entry_data.data_size;

      for (entry_data_list = entry_data_list->next;
           size && entry_data_list; size--)
        {
          entry_data_list = mmdb_skip_tree(entry_data_list, status);

          if (MMDB_SUCCESS != *status)
            {
              return NULL;
            }
        }
    }
    break;
    case MMDB_DATA_TYPE_BYTES:
    case MMDB_DATA_TYPE_BOOLEAN:
    case MMDB_DATA_TYPE_UINT128:
    case MMDB_DATA_TYPE_UTF8_STRING:
    case MMDB_DATA_TYPE_DOUBLE:
    case MMDB_DATA_TYPE_FLOAT:
    case MMDB_DATA_TYPE_UINT16:
    case MMDB_DATA_TYPE_UINT32:
    case MMDB_DATA_TYPE_UINT64:
    case MMDB_DATA_TYPE_INT32:
      entry_data_list = entry_data_list->next;
      break;

    default:
      *status = MMDB_INVALID_DATA_ERROR;
      return NULL;
    }

  *status = MMDB_SUCCESS;
  return entry_data_list;
}

void
geoip_log_msg_add_value(LogMessage *msg, GArray *path, GString *value)
{
  gchar *path_string = g_strjoinv(".", (gchar **)path->data);
  log_msg_set_value_by_name(msg, path_string, value->str, value->len);
  g_free(path_string);
}

static MMDB_entry_data_list_s *
select_language(LogMessage *msg, MMDB_entry_data_list_s *entry_data_list, GArray *path, gint *status)
{

  if (entry_data_list->entry_data.type != MMDB_DATA_TYPE_MAP)
    {
      *status = MMDB_INVALID_DATA_ERROR;
      return NULL;
    }

  guint32 size = entry_data_list->entry_data.data_size;

  for (entry_data_list = entry_data_list->next;
       size && entry_data_list; size--)
    {

      if (entry_data_list->entry_data.type != MMDB_DATA_TYPE_UTF8_STRING)
        {
          *status = MMDB_INVALID_DATA_ERROR;
          return NULL;
        }

      GString *key = scratch_buffers_alloc();
      g_string_printf(key, "%.*s",
                      entry_data_list->entry_data.data_size,
                      entry_data_list->entry_data.utf8_string);

      entry_data_list = entry_data_list->next;
      gchar *preferred_language = "en";
      if (!strcmp(key->str, preferred_language))
        {
          if (entry_data_list->entry_data.type != MMDB_DATA_TYPE_UTF8_STRING)
            {
              *status = MMDB_INVALID_DATA_ERROR;
              return NULL;
            }

          g_array_append_val(path, preferred_language);

          GString *value = scratch_buffers_alloc();
          g_string_printf(value, "%.*s",
                          entry_data_list->entry_data.data_size,
                          entry_data_list->entry_data.utf8_string);
          geoip_log_msg_add_value(msg, path, value);
          g_array_remove_index(path, path->len-1);

          entry_data_list = entry_data_list->next;
        }
      else
        {
          entry_data_list = mmdb_skip_tree(entry_data_list, status);
          if (MMDB_SUCCESS != *status)
            return NULL;
        }
    }
  return entry_data_list;
}

static void
_geoip_log_msg_add_value(LogMessage *msg, GArray *path, GString *value)
{
  gchar *path_string = g_strjoinv(".", (gchar **)path->data);
  log_msg_set_value_by_name(msg, path_string, value->str, value->len);
  g_free(path_string);
}

static void
_index_array_in_path(GArray *path, guint32 index, GString *indexer)
{
  g_string_printf(indexer, "%d", index++);
  ((gchar **)path->data)[path->len-1] = indexer->str;
}

MMDB_entry_data_list_s *
dump_geodata_into_msg(LogMessage *msg, MMDB_entry_data_list_s *entry_data_list, GArray *path, gint *status)
{
  GString *value = NULL;

  switch (entry_data_list->entry_data.type)
    {
    case MMDB_DATA_TYPE_MAP:
    {
      guint32 size = entry_data_list->entry_data.data_size;

      for (entry_data_list = entry_data_list->next;
           size && entry_data_list; size--)
        {

          if (MMDB_DATA_TYPE_UTF8_STRING != entry_data_list->entry_data.type)
            {
              *status = MMDB_INVALID_DATA_ERROR;
              return NULL;
            }

          GString *key = scratch_buffers_alloc();
          g_string_printf(key, "%.*s",
                          entry_data_list->entry_data.data_size,
                          entry_data_list->entry_data.utf8_string);

          g_array_append_val(path, key->str);
          entry_data_list = entry_data_list->next;


          if (!strcmp(key->str, "names"))
            entry_data_list = select_language(msg, entry_data_list, path, status);
          else
            entry_data_list = dump_geodata_into_msg(msg, entry_data_list, path, status);

          if (MMDB_SUCCESS != *status)
            return NULL;

          g_array_remove_index(path, path->len-1);
        }
    }
    break;
    case MMDB_DATA_TYPE_BYTES:
    case MMDB_DATA_TYPE_BOOLEAN:
    case MMDB_DATA_TYPE_UINT128:
      g_assert_not_reached();

    case MMDB_DATA_TYPE_ARRAY:
    {
      guint32 size = entry_data_list->entry_data.data_size;
      guint32 index = 0;
      GString *indexer = scratch_buffers_alloc();
      g_array_append_val(path, indexer->str);

      for (entry_data_list = entry_data_list->next;
           (index < size) && entry_data_list;
           index++)
        {
          _index_array_in_path(path, index, indexer);
          entry_data_list = dump_geodata_into_msg(msg, entry_data_list, path, status);

          if (MMDB_SUCCESS != *status)
            return NULL;
        }
      g_array_remove_index(path, path->len-1);
    }
    break;

    case MMDB_DATA_TYPE_UTF8_STRING:

      value = scratch_buffers_alloc();
      g_string_printf(value, "%.*s",
                      entry_data_list->entry_data.data_size,
                      entry_data_list->entry_data.utf8_string);

      _geoip_log_msg_add_value(msg, path, value);
      entry_data_list = entry_data_list->next;
      break;

    case MMDB_DATA_TYPE_DOUBLE:

      value = scratch_buffers_alloc();
      g_string_printf(value, "%f", entry_data_list->entry_data.double_value);

      _geoip_log_msg_add_value(msg, path, value);
      entry_data_list = entry_data_list->next;
      break;

    case MMDB_DATA_TYPE_FLOAT:

      value = scratch_buffers_alloc();
      g_string_printf(value, "%f", entry_data_list->entry_data.float_value);

      _geoip_log_msg_add_value(msg, path, value);
      entry_data_list = entry_data_list->next;
      break;

    case MMDB_DATA_TYPE_UINT16:

      value = scratch_buffers_alloc();
      g_string_printf(value, "%u", entry_data_list->entry_data.uint16);

      _geoip_log_msg_add_value(msg, path, value);
      entry_data_list = entry_data_list->next;
      break;

    case MMDB_DATA_TYPE_UINT32:

      value = scratch_buffers_alloc();
      g_string_printf(value, "%u", entry_data_list->entry_data.uint32);

      _geoip_log_msg_add_value(msg, path, value);
      entry_data_list = entry_data_list->next;
      break;

    case MMDB_DATA_TYPE_UINT64:

      value = scratch_buffers_alloc();
      g_string_printf(value, "%" PRIu64, entry_data_list->entry_data.uint64);

      _geoip_log_msg_add_value(msg, path, value);
      entry_data_list = entry_data_list->next;
      break;

    case MMDB_DATA_TYPE_INT32:

      value = scratch_buffers_alloc();
      g_string_printf(value, "%d" PRIu64, entry_data_list->entry_data.int32);

      _geoip_log_msg_add_value(msg, path, value);
      entry_data_list = entry_data_list->next;
      break;

    default:
      *status = MMDB_INVALID_DATA_ERROR;
      return NULL;
    }

  *status = MMDB_SUCCESS;
  return entry_data_list;
}
