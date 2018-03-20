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

#ifndef TRANSPORT_LOG_TRANSPORT_FACTORY_H_INCLUDED
#define TRANSPORT_LOG_TRANSPORT_FACTORY_H_INCLUDED

#include "transport/logtransport.h"
#include "transport/transport-factory-type.h"

typedef struct _LogTransportFactory LogTransportFactory;

struct _LogTransportFactory
{
  LogTransport *(*construct)(LogTransportFactory *self, gint fd);
  TransportFactoryType (*type)(void);
  void (*destroy)(LogTransportFactory *self);
};

static inline LogTransport *log_transport_factory_construct(LogTransportFactory *self, gint fd)
{
  g_assert(self->construct);
  return self->construct(self, fd);
}

static inline void log_transport_factory_destroy(LogTransportFactory *self)
{
  if (self->destroy)
    self->destroy(self);
}

static inline TransportFactoryType log_transport_factory_type(LogTransportFactory *self)
{
  g_assert(self->type);
  return self->type();
}

#endif
