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

#include "transport/multitransport-factory.h"

struct _MultiTransportFactory
{
  GHashTable *store;
};

static void
_destroy_transport_factory(gpointer s)
{
  LogTransportFactory *self = (LogTransportFactory *)s;
  log_transport_factory_destroy(self);
}

MultiTransportFactory *
multitransport_factory_new_full()
{
  MultiTransportFactory *instance = g_new0(MultiTransportFactory, 1);
  instance->store = g_hash_table_new_full(g_direct_hash, g_direct_equal, NULL, _destroy_transport_factory);
  return instance;
}

MultiTransportFactory *
multitransport_factory_new()
{
  MultiTransportFactory *instance = g_new0(MultiTransportFactory, 1);
  instance->store = g_hash_table_new(g_direct_hash, g_direct_equal);
  return instance;
}

void
multitransport_factory_free(MultiTransportFactory *self)
{
  g_hash_table_unref(self->store);
}

gboolean
multitransport_factory_add(MultiTransportFactory *self, LogTransportFactory *factory)
{
  if (g_hash_table_contains(self->store, GINT_TO_POINTER(factory->type())))
    return FALSE;

  g_hash_table_insert(self->store, GINT_TO_POINTER(factory->type()), factory);

  return TRUE;
}

LogTransportFactory *
multitransport_factory_lookup(MultiTransportFactory *self, TransportFactoryType type)
{
  return g_hash_table_lookup(self->store, GINT_TO_POINTER(type));
}

