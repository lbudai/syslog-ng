/*
 * Copyright (c) 2002-2019 One Identity
 * Copyright (c) 2019 Laszlo Budai
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

#include <criterion/criterion.h>
#include "signal-slot-connector/signal-slot-connector.h"

typedef struct _TestData TestData;

struct _TestData
{
  gint slot_ctr;
};

void
test_data_init(TestData *data)
{
  data->slot_ctr = 0;
}

SIGNAL(test1_signal);

void test1_slot(TestData *user_data)
{
  user_data->slot_ctr++;
}

void test2_slot(TestData *user_data)
{
  user_data->slot_ctr++;
}

Test(basic_signal_slots, when_the_signal_is_emitted_then_the_connected_slot_is_executed)
{
  SignalSlotConnector *ssc = signal_slot_connector_new();
  TestData test_data;
  test_data_init(&test_data);

  CONNECT(ssc, test1_signal, test1_slot);
  EMIT(ssc, test1_signal, &test_data);

  cr_expect_eq(test_data.slot_ctr, 1);

  signal_slot_connector_free(ssc);
}

Test(basic_signal_slots, when_trying_to_connect_multiple_times_the_same_connection_then_connect_only_the_first)
{
  SignalSlotConnector *ssc = signal_slot_connector_new();
  TestData test_data;
  test_data_init(&test_data);

  for (gint i = 0; i < 5; i++)
    CONNECT(ssc, test1_signal, test1_slot);

  EMIT(ssc, test1_signal, &test_data);

  cr_expect_eq(test_data.slot_ctr, 1);

  signal_slot_connector_free(ssc);
}

SIGNAL(test2_signal);

Test(basic_signal_slots, when_trying_to_disconnect_multiple_times_the_same_connection_disconnect_only_the_first)
{
  SignalSlotConnector *ssc = signal_slot_connector_new();
  TestData test_data;
  test_data_init(&test_data);

  CONNECT(ssc, test1_signal, test1_slot);
  CONNECT(ssc, test2_signal, test2_slot);

  for (gint i = 0; i < 5; i++)
    DISCONNECT(ssc, test2_signal, test1_slot);

  EMIT(ssc, test1_signal, &test_data);

  cr_expect_eq(test_data.slot_ctr, 1);

  signal_slot_connector_free(ssc);
}

Test(basic_signal_slots,
     when_slot_is_disconnected_from_a_signal_and_the_signal_is_emitted_then_the_slot_should_not_be_executed)
{
  SignalSlotConnector *ssc = signal_slot_connector_new();
  TestData test_data;
  test_data_init(&test_data);

  CONNECT(ssc, test1_signal, test1_slot);
  CONNECT(ssc, test2_signal, test1_slot);

  DISCONNECT(ssc, test1_signal, test1_slot);
  EMIT(ssc, test1_signal, &test_data);

  cr_expect_eq(test_data.slot_ctr, 0);

  EMIT(ssc, test2_signal, &test_data);

  cr_expect_eq(test_data.slot_ctr, 1);

  DISCONNECT(ssc, test2_signal, test1_slot);
  EMIT(ssc, test2_signal, &test_data);

  cr_expect_eq(test_data.slot_ctr, 1);

  signal_slot_connector_free(ssc);
}

#define CONCAT_STR_3(s1, s2, s3) s1##s2##s3

#define DEFINE_TEST_SLOT(func_name, idx) \
  void CONCAT_STR_3(func_name, _slot_, idx)(TestData *user_data) \
  { \
    user_data->slot_ctr++; \
  }

DEFINE_TEST_SLOT(test, 0)
DEFINE_TEST_SLOT(test, 1)
DEFINE_TEST_SLOT(test, 2)
DEFINE_TEST_SLOT(test, 3)

#define ARRAY_SIZE(a) (sizeof(a) / sizeof((a)[0]))

SIGNAL(signal0);
SIGNAL(signal1);
SIGNAL(signal2);
SIGNAL(signal3);

Signal signals[4] =
{
  signal0,
  signal1,
  signal2,
  signal3
};

Slot slots[4] =
{
  (Slot) test_slot_0,
  (Slot) test_slot_1,
  (Slot) test_slot_2,
  (Slot) test_slot_3
};

void
_connect_a_signal_with_all_test_slots(SignalSlotConnector *ssc, Signal s)
{
  for (gint i = 0; i < ARRAY_SIZE(slots); i++)
    CONNECT(ssc, s, slots[i]);
}

void
_disconnect_all_test_slots_from_a_signal(SignalSlotConnector *ssc, Signal s)
{
  for (gint i = 0; i < ARRAY_SIZE(slots); i++)
    DISCONNECT(ssc, s, slots[i]);
}

Test(multiple_signals_slots,
     given_one_signal_with_multiple_slots_when_the_signal_is_emitted_then_the_slots_are_executed)
{
  SignalSlotConnector *ssc = signal_slot_connector_new();
  TestData test_data;
  test_data_init(&test_data);

  _connect_a_signal_with_all_test_slots(ssc, signals[0]);

  EMIT(ssc, signals[0], &test_data);
  cr_expect_eq(test_data.slot_ctr, ARRAY_SIZE(slots));

  _disconnect_all_test_slots_from_a_signal(ssc, signals[0]);

  signal_slot_connector_free(ssc);
}

Test(multiple_signals_slots,
     given_a_signal_with_multiple_slots_when_slots_are_disconnected_1_by_1_then_remaining_slots_are_still_executed)
{
  SignalSlotConnector *ssc = signal_slot_connector_new();
  TestData test_data;
  test_data_init(&test_data);

  for (gint i = 0; i < 3; i++)
    CONNECT(ssc, signals[0], slots[i]);

  DISCONNECT(ssc, signals[0], slots[0]);
  EMIT(ssc, signals[0], &test_data);
  cr_expect_eq(test_data.slot_ctr, 2);

  DISCONNECT(ssc, signals[0], slots[1]);
  EMIT(ssc, signals[0], &test_data);
  cr_expect_eq(test_data.slot_ctr, 3);

  DISCONNECT(ssc, signals[0], slots[2]);
  EMIT(ssc, signals[0], &test_data);
  cr_expect_eq(test_data.slot_ctr, 3);

  signal_slot_connector_free(ssc);
}

