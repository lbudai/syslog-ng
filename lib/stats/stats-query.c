#include "stats-query.h"
#include "stats.h"
#include "stats-cluster.h"
#include "stats-registry.h"
#include <string.h>
#include "messages.h"

static const gchar *_counter_type[SC_TYPE_MAX+1] =
{
  "dropped",
  "processed",
  "stored",
  "suppressed",
  "stamp",
  "MAX"
};

static gboolean
_find_key_by_pattern(gpointer key, gpointer value, gpointer user_data)
{
  StatsCluster *sc = (StatsCluster *)key;
  gchar *pattern = (gchar *)user_data;
  msg_debug("stats_find_by_key", evt_tag_str("key", sc->query_key),
                                 evt_tag_str("pattern", pattern));
  
  return g_pattern_match_simple(pattern, sc->query_key);
}

static void
_str_append_live_counters(GString *str, StatsCluster *sc)
{
  for (guint16 i = SC_TYPE_DROPPED; i < SC_TYPE_MAX; i++)
    {
      if (((1<<i) & sc->live_mask))
        g_string_append_printf(str, "%s=%d\n", _counter_type[i], sc->counters[i].value);
    }
}

static void
_str_append_counter(GString *str, StatsCluster *sc, const gchar *ctr)
{
  for (guint16 i = 0; i < SC_TYPE_MAX; i++)
    {
      if (strcmp(_counter_type[i], ctr) == 0)
        {
          if (((1<<i) & sc->live_mask))
              g_string_append_printf(str, "%s=%d", _counter_type[i], sc->counters[i].value);
          break;
        }
    }
} 


GString *
stats_query(const gchar *expr)
{
  GHashTable *counter_hash = _get_counter_hash();
  StatsCluster *sc = NULL;
  GString *result = g_string_new("");
  const gchar *ctr = strrchr(expr, '.') + 1;
  gchar *pattern = g_strndup(expr, ctr - expr - 1);

  msg_debug("stats_query", evt_tag_str("ctr", ctr));
 
  GHashTableIter iter;
  gpointer key, value;
  g_hash_table_iter_init (&iter, counter_hash);
  gboolean single_match = strchr(expr, '*') >= ctr ? TRUE : FALSE;
  while (g_hash_table_iter_next (&iter, &key, &value))
    {
			if (_find_key_by_pattern(key, value, (gpointer)pattern))
				{
          sc = (StatsCluster *)key;
          g_string_append_printf(result, "key:%s\n", sc->query_key);
          if (ctr[0] == '*')
            _str_append_live_counters(result, sc);
          else
            _str_append_counter(result, sc, ctr);
          if (single_match)
            break;
				}			
    }

  g_free(pattern);

  g_string_append(result, "\n");

  return result;
}
