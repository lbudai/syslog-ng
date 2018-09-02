/*
 * Copyright (c) 2017 Balabit
 * Copyright (c) 2017 Laszlo Budai
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

#include "stats/stats-cluster-single.h"
#include "stats/stats-cluster.h"
#include "stats/stats-registry.h"

static const gchar *tag_names[SC_TYPE_SINGLE_MAX] =
{
  /* [SC_TYPE_SINGLE_VALUE]   = */ "value",
};

static void
_counter_group_free(StatsCounterGroup *counter_group)
{
  g_free(counter_group->counters);
}

static gboolean
_group_init_equals(const StatsCounterGroupInit *self, const StatsCounterGroupInit *other)
{
  g_assert(self != NULL && other != NULL);
  return (self->init == other->init) && (self->sizeof_counter_type == other->sizeof_counter_type);
}


static void
_counter_group_init(StatsCounterGroupInit *self, StatsCounterGroup *counter_group)
{
  counter_group->counters = g_new0(StatsCounterItem, SC_TYPE_SINGLE_MAX);
  counter_group->capacity = SC_TYPE_SINGLE_MAX;
  counter_group->counter_names = self->counter_names;
  counter_group->free_fn = _counter_group_free;
}

void
stats_cluster_single_key_set(StatsClusterKey *key, guint16 component, const gchar *id, const gchar *instance)
{
  stats_cluster_key_set(key, component, id, instance, (StatsCounterGroupInit)
  {
    .sizeof_counter_type = sizeof(StatsCounterItem),
    .counter_names = tag_names,
    .init = _counter_group_init,
    .equals = _group_init_equals
  });
}

static void
_counter_group_with_name_free(StatsCounterGroup *counter_group)
{
  g_free(counter_group->counter_names);
  _counter_group_free(counter_group);
}

static void
_counter_group_init_with_name(StatsCounterGroupInit *self, StatsCounterGroup *counter_group)
{
  counter_group->counters = g_malloc0_n(SC_TYPE_SINGLE_MAX, self->sizeof_counter_type);
  counter_group->capacity = SC_TYPE_SINGLE_MAX;
  counter_group->counter_names = self->counter_names;
  counter_group->free_fn = _counter_group_with_name_free;
}

static gboolean
_group_init_equals_with_name(const StatsCounterGroupInit *self, const StatsCounterGroupInit *other)
{
  gboolean res = _group_init_equals(self, other);
  g_assert(self->counter_names != NULL && other->counter_names != NULL);
  return res && (g_strcmp0(self->counter_names[0], other->counter_names[0]) == 0);
}

void
stats_cluster_single_key_set_with_name(StatsClusterKey *key, guint16 component, const gchar *id, const gchar *instance,
                                       const gchar *name)
{
  stats_cluster_key_set(key, component, id, instance, (StatsCounterGroupInit)
  {
    .sizeof_counter_type = sizeof(StatsCounterItem),
    .counter_names = g_new0(const char *, 1), 
    .init = _counter_group_init_with_name,
    .equals = _group_init_equals_with_name
  });
  key->counter_group_init.counter_names[0] = name;
}

StatsCounterItem*
stats_cluster_single_register_counter(guint16 component, const gchar *id,
                                      const gchar *instance, const gchar *name,
                                      gsize sizeof_counter)
{
  StatsClusterKey sc_key;
  stats_cluster_single_key_set_with_name(&sc_key,component,id,instance,name);
  sc_key.counter_group_init.sizeof_counter_type = sizeof_counter;
  StatsCounterItem *ctr = NULL;
  stats_register_counter(0, &sc_key, SC_TYPE_SINGLE_VALUE, &ctr);

  return ctr;
}

StatsCounterItem *
stats_cluster_single_unregister_counter(guint16 component, const gchar *id,
                                        const gchar *instance, const gchar *name,
                                        gsize sizeof_counter)
{
  StatsClusterKey sc_key;
  stats_cluster_single_key_set_with_name(&sc_key,component,id,instance,name);
  sc_key.counter_group_init.sizeof_counter_type = sizeof_counter;
  StatsCounterItem *ctr = NULL;
  stats_unregister_counter(&sc_key, SC_TYPE_SINGLE_VALUE, &ctr);

  return ctr;
}
