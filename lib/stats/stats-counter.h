/*
 * Copyright (c) 2002-2013 Balabit
 * Copyright (c) 1998-2013 Balázs Scheidler
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
#ifndef STATS_COUNTER_H_INCLUDED
#define STATS_COUNTER_H_INCLUDED 1

#include "syslog-ng.h"
#include "atomic-gssize.h"

typedef struct _StatsCounterItem StatsCounterItem;

struct _StatsCounterItem
{
  atomic_gssize value;
  gchar *name;
  gint type;
  void (*add)(StatsCounterItem *self, gsize add);
  void (*sub)(StatsCounterItem *self, gsize sub);
  void (*inc)(StatsCounterItem *self);
  void (*dec)(StatsCounterItem *self);
  void (*set)(StatsCounterItem *self, gsize value);
  gsize (*get)(StatsCounterItem *self);
};

static inline void
stats_counter_add(StatsCounterItem *counter, gssize add)
{
  if (!counter)
    return;

  if (!counter->add)
    {
      atomic_gssize_add(&counter->value, add);
      return;
    }

  counter->add(counter, add);
}

static inline void
stats_counter_sub(StatsCounterItem *counter, gssize sub)
{
  if (!counter)
    return;

  if (!counter->sub)
    {
      atomic_gssize_sub(&counter->value, sub);
      return;
    }

  counter->sub(counter, sub);
}

static inline void
stats_counter_inc(StatsCounterItem *counter)
{
  if (!counter)
    return;

  if (!counter->inc)
    {
      atomic_gssize_inc(&counter->value);
      return;
    }

  counter->inc(counter);
}

static inline void
stats_counter_dec(StatsCounterItem *counter)
{
  if (!counter)
    return;

  if (!counter->dec)
    {
      atomic_gssize_dec(&counter->value);
      return;
    }

  counter->dec(counter);
}

/* NOTE: this is _not_ atomic and doesn't have to be as sets would race anyway */
static inline void
stats_counter_set(StatsCounterItem *counter, gsize value)
{
  if (!counter)
    return;

  if (!counter->set)
    {
      atomic_gssize_racy_set(&counter->value, value);
      return;
    }

  counter->set(counter, value);
}

/* NOTE: this is _not_ atomic and doesn't have to be as sets would race anyway */
static inline gsize
stats_counter_get(StatsCounterItem *counter)
{
  if (!counter)
    return 0;

  if (!counter->get)
   return atomic_gssize_get_unsigned(&counter->value);

  return counter->get(counter);
}

static inline gchar *
stats_counter_get_name(StatsCounterItem *counter)
{
  if (counter)
    return counter->name;
  return NULL;
}

void stats_reset_counters(void);
void stats_counter_free(StatsCounterItem *counter);

#endif
