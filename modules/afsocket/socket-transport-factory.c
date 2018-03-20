/*
 * Copyright (c) 2002-2018 Balabit
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

#include "socket-transport-factory.h"
#include "transport/transport-factory-type.h"
#include "transport/transport-socket.h"

static TransportFactoryType
_type_stream(void)
{
  return TRANSPORT_SOCKET_STREAM;
}

static LogTransport *
_construct_stream(LogTransportFactory *self, gint fd)
{
  return log_transport_stream_socket_new(fd);
}

LogTransportFactory *
log_transport_socket_stream_factory_new(void)
{
  LogTransportFactory *instance = g_new0(LogTransportFactory, 1);

  instance->type = _type_stream;
  instance->construct = _construct_stream;

  return instance;
}

static TransportFactoryType
_type_dgram(void)
{
  return TRANSPORT_SOCKET_DGRAM;
}

static LogTransport *
_construct_dgram(LogTransportFactory *self, gint fd)
{
  return log_transport_dgram_socket_new(fd);
}

LogTransportFactory *
log_transport_socket_dgram_factory_new(void)
{
  LogTransportFactory *instance = g_new(LogTransportFactory, 1);

  instance->type = _type_dgram;
  instance->construct = _construct_dgram;

  return instance;
}
