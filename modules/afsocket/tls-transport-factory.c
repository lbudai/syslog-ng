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

#include "tls-transport-factory.h"
#include "transport/transport-factory-type.h"
#include "transport/transport-tls.h"

typedef struct _LogTransportTLSFactory LogTransportTLSFactory;

struct _LogTransportTLSFactory
{
  LogTransportFactory super;
  TLSContext *tls_context;
  TLSSessionVerifyFunc tls_verify_callback;
  gpointer tls_verify_data;
};

static TransportFactoryType
_type(void)
{
  return TRANSPORT_TLS;
}

static LogTransport *
_construct(LogTransportFactory *s, gint fd)
{
  LogTransportTLSFactory *self = (LogTransportTLSFactory *)s;

  TLSSession *tls_session = tls_context_setup_session(self->tls_context);
  if (!tls_session)
    return NULL;

  tls_session_set_verify(tls_session, self->tls_verify_callback, self->tls_verify_data, NULL);
  return log_transport_tls_new(tls_session, fd);
}

LogTransportFactory *
log_transport_tls_factory_new(const TLSContext *ctx, const TLSSessionVerifyFunc tls_verify_callback,
                              const gpointer tls_verify_data)
{
  LogTransportTLSFactory *instance = g_new0(LogTransportTLSFactory, 1);

  instance->tls_context = ctx;
  instance->tls_verify_callback = tls_verify_callback;
  instance->tls_verify_data = tls_verify_data;
  instance->super.type = _type;
  instance->super.construct = _construct;

  return &instance->super;
}
