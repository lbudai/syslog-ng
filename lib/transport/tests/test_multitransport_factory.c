/*
 * Copyright (c) 2002-2018 Balabit
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
#include <criterion/criterion.h>

typedef struct _TestTransport TestTransport;

struct _TestTransport
{
  LogTransport super;
};

static
gssize _transport_read(LogTransport *s, gpointer buf, gsize count, LogTransportAuxData *aux)
{
  return count;
}

static
gssize _transport_write(LogTransport *s, const gpointer buf, gsize count)
{
  return count;
}

typedef struct _TestLogTransportFactory TestLogTransportFactory;
struct _TestLogTransportFactory
{
  LogTransportFactory super;
  gchar *test_data;
  gint *one_after_destroy;
};

static TransportFactoryType
_factory_type(void)
{
  static const TransportFactoryType DUMMY_FACTORY_TYPE = 1111;
  return DUMMY_FACTORY_TYPE;
}

static void
_factory_destroy(LogTransportFactory *s)
{
  TestLogTransportFactory *self = (TestLogTransportFactory *)s;
  *(self->one_after_destroy) = 1;
  g_free(self->test_data);
  self->test_data = NULL;
}

static LogTransport *
_factory_construct(LogTransportFactory *self, gint fd)
{
  TestTransport *instance =  g_new0(TestTransport, 1);
  log_transport_init_instance(&instance->super, fd);

  instance->super.read = _transport_read;
  instance->super.write = _transport_write;

  return &instance->super;
}

static LogTransportFactory *
_test_logtransport_factory_new(const gchar *test_data, gint *one_after_destroy)
{
  TestLogTransportFactory *instance = g_new0(TestLogTransportFactory, 1);
  instance->test_data = g_strdup(test_data);
  instance->one_after_destroy = one_after_destroy;
  instance->super.type = _factory_type;
  instance->super.construct = _factory_construct;
  instance->super.destroy = _factory_destroy;

  return &instance->super;
}

Test(multitransport_factory_full, logmultitransport_factory)
{
  MultiTransportFactory *multitransport_factory = multitransport_factory_new_full();
  gint one_after_destroy = 222;
  LogTransportFactory *factory = _test_logtransport_factory_new("xxx", &one_after_destroy);
  multitransport_factory_add(multitransport_factory, factory);
  LogTransportFactory *lfactory = multitransport_factory_lookup(multitransport_factory, (TransportFactoryType)1111);
  cr_expect_not_null(lfactory);
  cr_expect_eq(lfactory->type(), 1111);
  LogTransportFactory *lfactory_not_exists = multitransport_factory_lookup(multitransport_factory,
                                             (TransportFactoryType)1112);
  cr_expect_null(lfactory_not_exists);

  multitransport_factory_free(multitransport_factory);
  cr_expect_eq(one_after_destroy, 1);
}

Test(multitransport_factory, logmultitransport_factory)
{
  MultiTransportFactory *multitransport_factory = multitransport_factory_new();
  gint one_after_destroy = 222;
  LogTransportFactory *factory = _test_logtransport_factory_new("xxx", &one_after_destroy);
  multitransport_factory_add(multitransport_factory, factory);
  LogTransportFactory *lfactory = multitransport_factory_lookup(multitransport_factory, (TransportFactoryType)1111);
  cr_expect_not_null(lfactory);
  cr_expect_eq(lfactory->type(), 1111);

  multitransport_factory_free(multitransport_factory);
  cr_expect_eq(one_after_destroy, 222);
}
