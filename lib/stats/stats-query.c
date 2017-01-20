/*
 * Copyright (c) 2002-2017 Balabit
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
 *
 * As an additional exemption you are allowed to compile & link against the
 * OpenSSL libraries as published by the OpenSSL project. See the file
 * COPYING for details.
 *
 */

#include "stats-query.h"
#include "stats.h"
#include "stats-cluster.h"
#include "stats-registry.h"
#include <string.h>

static const gint _COUNTER_ALL_IDX = SC_TYPE_MAX;

static inline gboolean
_is_counter_alive(StatsCluster *sc, gint type)
{
  if (((1<<type) & sc->live_mask))
    return TRUE;

  return FALSE;
}

static void 
_append_counter_with_value(GString *str, StatsCluster *sc, StatsCounterItem *ctr, const gchar *ctr_name)
{
  g_string_append_printf(str, "%s.%s:%d\n", sc->query_key, ctr_name ? ctr_name : "", ctr->value);
}

static void
_append_counter_without_value(GString *str, StatsCluster *sc, StatsCounterItem *ctr, const gchar *ctr_name)
{
  g_string_append_printf(str, "%s.%s\n", sc->query_key, ctr_name ? ctr_name : "");
}

static void
_append_live_counters_to_str(GString *str, StatsCluster *sc, gboolean with_value)
{
  for (gint i = 0; i < SC_TYPE_MAX; i++)
    {
      if (_is_counter_alive(sc, i))
        {
          if (with_value)
            _append_counter_with_value(str, sc, &sc->counters[i], stats_cluster_get_type_name(i));
          else
            _append_counter_without_value(str, sc, &sc->counters[i], stats_cluster_get_type_name(i));
        }
    }
}

static void 
_append_counter_by_type(StatsCluster *sc, gint type, GString *result, gboolean with_value)
{ 
  if (with_value)
    _append_counter_with_value(result, sc, &sc->counters[type], stats_cluster_get_type_name(type));
  else
    _append_counter_without_value(result, sc, &sc->counters[type], stats_cluster_get_type_name(type));
}

void
_split_expr(const gchar *expr, gchar **key, gint *counter_type)
{
  const gchar *last_delim_pos = strrchr(expr, '.');
  const gchar *ctr = NULL;
  if (last_delim_pos)
    {
      size_t key_len = last_delim_pos - expr ;
      ctr = expr + key_len + 1;
      *key = g_strndup(expr, key_len);
   }
  else
   {
     *key = g_strdup_printf("%s.*", expr);
   }
  *counter_type = stats_cluster_get_type_by_name(ctr);
}

static gboolean
_find_key_by_pattern(gpointer key, gpointer value, gpointer user_data)
{
  StatsCluster *sc = (StatsCluster *)key;
  GPatternSpec *pattern = (GPatternSpec *)user_data;

  return g_pattern_match_string(pattern, sc->query_key);
}

static void
_get_counters(GString *result, const gchar *key_pattern, gint counter_type, gboolean with_value)
{
  GHashTable *counter_hash = stats_registry_get_counter_hash();
  GHashTableIter iter;
  gpointer key, value;
  g_hash_table_iter_init (&iter, counter_hash);
  GPatternSpec *pattern = g_pattern_spec_new(key_pattern);
  StatsCluster *sc = NULL;
  gboolean single_match = (counter_type != _COUNTER_ALL_IDX) && strchr(key_pattern, '*') == NULL;
  while (g_hash_table_iter_next (&iter, &key, &value))
    {
      if (_find_key_by_pattern(key, value, (gpointer)pattern))
        {
          sc = (StatsCluster *)key;
          if ( counter_type == _COUNTER_ALL_IDX )
            {
              _append_live_counters_to_str(result, sc, with_value);
            }
          else
            {
              _append_counter_by_type(sc, counter_type, result, with_value);
            }
          if (single_match)
            break;
        }
    }
}

static void
_get_counters_with_value(GString *result, const gchar *key_pattern, gint counter_type)
{
  return _get_counters(result, key_pattern, counter_type, TRUE);
}

static void
_get_counters_without_value(GString *result, const gchar *key_pattern, gint counter_type)
{
  return _get_counters(result, key_pattern, counter_type, FALSE);
}

GString *
stats_query_get(const gchar *expr)
{
  GString *result = g_string_new("");
  gint ctr_type = 0;
  gchar *key = NULL;

  if (!expr)
    return result;
 
  _split_expr(expr, &key, &ctr_type);

  _get_counters_with_value(result, key, ctr_type);

  g_free(key);
  return result;
}

GString *
stats_query_get_aggregated(const gchar *expr)
{
  return g_string_new("");
}

GString *
stats_query_list(const gchar *expr)
{
  GString *result = g_string_new("");
  gint ctr_type = 0;
  gchar *key = NULL;

  if (!expr)
    {
      key = g_strdup("*.*");
      ctr_type = _COUNTER_ALL_IDX;
    }
  else
    {
      _split_expr(expr, &key, &ctr_type);
    }

  _get_counters_without_value(result, key, ctr_type);

  g_free(key);
  return result;
}

