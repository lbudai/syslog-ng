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

#include "logproto-multitransport-server.h"
#include "transport/logtransport.h"
#include "messages.h"

gboolean log_proto_multitransport_server_replace_transport(LogProtoMultiTransportServer *self,
                                                           TransportFactoryType next_transport_type)
{
  LogTransportFactory *transport_factory = multitransport_factory_lookup(self->transport_factory, next_transport_type);

  if (!transport_factory)
    {
      msg_error("Failed to replace transport", evt_tag_int("Internal type code",
                                                           next_transport_type)); //TODO: internal type code??
      return FALSE;
    }

  LogTransport *transport = log_transport_factory_construct(transport_factory, self->super.transport->fd);
  if (!transport)
    {
      msg_error("Failed to construct transport", evt_tag_int("Internal type code",
                                                             log_transport_factory_type(transport_factory)));
      return FALSE;
    }

  log_transport_release_fd(self->super.transport);
  log_transport_free(self->super.transport);
  self->super.transport = transport;

  return TRUE;
}

