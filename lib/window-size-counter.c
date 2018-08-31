/*
 * Copyright (c) 2002-2018 Balabit
 * Copyright (c) 2018 Laszlo Budai <laszlo.budai@balabit.com>
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

#include "window-size-counter.h"

#define COUNTER_MASK (((gsize)1<<(8*sizeof(gsize)-1)) - 1)
static const gsize COUNTER_MAX = COUNTER_MASK;
static const gsize SUSPEND_MASK = G_MAXSIZE ^ COUNTER_MASK;
static const gsize RESUME_MASK = G_MAXSIZE >> 1;

static gboolean
_is_suspended(gsize v, gsize threshold)
{
  if (threshold == 0)
    return (v == threshold) || ((v & SUSPEND_MASK) == SUSPEND_MASK);
  return (v >= threshold) || ((v & SUSPEND_MASK) == SUSPEND_MASK);
}

gsize
window_size_counter_get_max(void)
{
  return COUNTER_MAX;
}

void
window_size_counter_set(WindowSizeCounter *c, gsize value)
{
  atomic_gssize_set(&c->super.value, value & COUNTER_MASK);
}

gsize
window_size_counter_get(WindowSizeCounter *c, gboolean *suspended)
{
  gsize v = atomic_gssize_get_unsigned(&c->super.value);
  if (suspended)
    *suspended = _is_suspended(v, c->suspend_threshold);
  return v & COUNTER_MASK;
}

gsize
window_size_counter_add(WindowSizeCounter *c, gsize value, gboolean *suspended)
{
  gsize v = (gsize)atomic_gssize_add(&c->super.value, value);
  gsize old_value = v & COUNTER_MASK;
  g_assert (old_value + value <= COUNTER_MAX);
  if (suspended)
    *suspended = _is_suspended(v, c->suspend_threshold);

  return old_value;
}

gsize
window_size_counter_sub(WindowSizeCounter *c, gsize value, gboolean *suspended)
{
  gsize v = (gsize)atomic_gssize_add(&c->super.value, -1 * value);
  gsize old_value = v & COUNTER_MASK;
  g_assert (old_value >= value);
  if (suspended)
    *suspended = _is_suspended(v, c->suspend_threshold);

  return old_value;
}

void
window_size_counter_suspend(WindowSizeCounter *c)
{
  atomic_gssize_or(&c->super.value, SUSPEND_MASK);
}

void
window_size_counter_resume(WindowSizeCounter *c)
{
  atomic_gssize_and(&c->super.value, RESUME_MASK);
}

gboolean
window_size_counter_suspended(WindowSizeCounter *c)
{
  gsize v = atomic_gssize_get_unsigned(&c->super.value);
  return _is_suspended(v, c->suspend_threshold);
}

static void
_add(StatsCounterItem *s, gsize add)
{
  WindowSizeCounter *self = (WindowSizeCounter *)s;
  window_size_counter_add(self, add, NULL);
}

static void
_sub(StatsCounterItem *s, gsize sub)
{
  WindowSizeCounter *self = (WindowSizeCounter *)s;
  window_size_counter_sub(self, sub, NULL);
}

static void
_inc(StatsCounterItem *s)
{
  _add(s, 1);
}

static void
_dec(StatsCounterItem *s)
{
  _sub(s, 1);
}

static void
_set(StatsCounterItem *s, gsize value)
{
  WindowSizeCounter *self = (WindowSizeCounter *)s;
  window_size_counter_set(self, value);
}

static gsize
_get(StatsCounterItem *s)
{
  WindowSizeCounter *self = (WindowSizeCounter *)s;
  return window_size_counter_get(self, NULL);
}

static void
_init_vtable(StatsCounterItem *s)
{
  s->add = _add;
  s->sub = _sub;
  s->inc = _inc;
  s->dec = _dec;
  s->set = _set;
  s->get = _get;
}

void
window_size_counter_init(WindowSizeCounter *c, gsize suspend_threshold)
{
  _init_vtable(&c->super);
  c->suspend_threshold = suspend_threshold;
}
