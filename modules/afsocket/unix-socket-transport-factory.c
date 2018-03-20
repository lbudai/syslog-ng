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

#include "unix-socket-transport-factory.h"
#include "transport/transport-factory-type.h"
#include "transport-unix-socket.h"
#include "unix-credentials.h"

typedef struct _LogTransportUnixSocketFactory LogTransportUnixSocketFactory;

struct _LogTransportUnixSocketFactory
{
  LogTransportFactory super;
  gboolean pass_user_credentials;
};

static TransportFactoryType
_type_stream(void)
{
  return TRANSPORT_SOCKET_STREAM;
}

static LogTransport *
_construct_stream(LogTransportFactory *s, gint fd)
{
  LogTransportUnixSocketFactory *self = (LogTransportUnixSocketFactory *)s;

  LogTransport *transport = log_transport_unix_stream_socket_new(fd);

  if (self->pass_user_credentials)
    socket_set_pass_credentials(fd);

  return transport;
}

LogTransportFactory *
log_transport_unix_stream_factory_new(gboolean pass_user_credentials)
{
  LogTransportUnixSocketFactory *instance = g_new0(LogTransportUnixSocketFactory, 1);
  instance->pass_user_credentials = pass_user_credentials;

  instance->super.type = _type_stream;
  instance->super.construct = _construct_stream;

  return &instance->super;
}

static TransportFactoryType
_type_dgram(void)
{
  return TRANSPORT_SOCKET_DGRAM;
}

static LogTransport *
_construct_dgram(LogTransportFactory *s, gint fd)
{
  LogTransportUnixSocketFactory *self = (LogTransportUnixSocketFactory *)s;

  LogTransport *transport = log_transport_unix_dgram_socket_new(fd);

  if (self->pass_user_credentials)
    socket_set_pass_credentials(fd);

  return transport;
}

LogTransportFactory *
log_transport_unix_dgram_factory_new(gboolean pass_user_credentials)
{
  LogTransportUnixSocketFactory *instance = g_new0(LogTransportUnixSocketFactory, 1);
  instance->pass_user_credentials = pass_user_credentials;

  instance->super.type = _type_dgram;
  instance->super.construct = _construct_dgram;

  return &instance->super;
}

