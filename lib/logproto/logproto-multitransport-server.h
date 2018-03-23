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

#ifndef LOGPROTO_MULTITRANSPORT_SERVER_FACTORY_H_INCLUDED
#define LOGPROTO_MULTITRANSPORT_SERVER_FACTORY_H_INCLUDED

#include "logproto-server.h"
#include "transport/logtransport-factory.h"
#include "transport/multitransport-factory.h"

typedef struct _LogProtoMultiTransportServer LogProtoMultiTransportServer;

struct _LogProtoMultiTransportServer
{
  LogProtoServer super;
  MultiTransportFactory *transport_factory;
};

gboolean log_proto_multitransport_server_replace_transport(LogProtoMultiTransportServer *s,
                                                           TransportFactoryType next_transport_type);


#define DEFINE_LOG_PROTO_MULTITRANSPORT_SERVER(prefix) \
  static gpointer                                                       \
  prefix ## _server_plugin_construct(Plugin *self)                      \
  {                                                                     \
    static LogProtoMultiTransportServerFactory proto = {                              \
      .construct = prefix ## _server_new,                       \
    };                                                                  \
    return &proto;                                                      \
  }

#define LOG_PROTO_MULTITRANSPORT_SERVER_PLUGIN(prefix, __name) \
  {             \
    .type = LL_CONTEXT_SERVER_PROTO,            \
    .name = __name,         \
    .construct = prefix ## _server_plugin_construct,  \
  }

typedef struct _LogProtoMultiTransportServerFactory LogProtoMultiTransportServerFactory;
struct _LogProtoMultiTransportServerFactory
{
  LogProtoServer *(*construct)(LogTransport *initial_transport,
                               MultiTransportFactory *multitransport_factory,
                               const LogProtoServerOptions *options);
};

static inline LogProtoServer *
log_proto_multi_transport_server_factory_construct(LogProtoMultiTransportServerFactory *self,
                                                   LogTransport *initial_transport,
                                                   MultiTransportFactory *multitransport_factory,
                                                   const LogProtoServerOptions *options)
{
  return self->construct(initial_transport, multitransport_factory, options);
}

#endif
