/*
 * Copyright (c) 2002-2019 One Identity
 * Copyright (c) 2019 Laszlo Budai <laszlo.budai@oneidentity.com>
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

#include "signal-slot-connector.h"

struct _SignalSlotConnector
{
  // map<Signal, set<SlotObject>> connections;
  GHashTable *connections;
  GMutex *lock; // connect/disconnect guarded by lock, emit is not
};

GList *
_slot_lookup_node(GList *slot_objects, Slot slot)
{
  for (GList *it = slot_objects; it != NULL; it = it->next)
    {
      Slot s = (Slot) it->data;

      if (s == slot)
        return it;
    }

  return NULL;
}

Slot
_slot_lookup(GList *slot_objects, Slot slot)
{
  GList *slot_node = _slot_lookup_node(slot_objects, slot);
  if (!slot_node)
    return NULL;

  return (Slot) slot_node->data;
}

GList *
_slots_remove_safely(GList *slot_objects, Slot slot)
{
  GList *slot_node = _slot_lookup_node(slot_objects, slot);

  if (!slot_node)
    return slot_objects;

  if (slot_node->data == slot)
    {
      if (slot_node != slot_objects)
        {
          GList *head_new = g_list_remove_link(slot_objects, slot_node);
          g_assert(head_new == slot_objects);
          return slot_objects;
        }

      if (slot_node->next == NULL)
        {
          GList *head_null = g_list_remove_link(slot_objects, slot_node);
          g_assert(head_null == NULL);
          return NULL;
        }

      g_assert(slot_node == slot_objects);
      GList *remove = slot_objects->next;
      slot_objects->data = slot_objects->next->data;
      GList *head_new = g_list_remove_link(slot_objects, slot_objects->next);
      g_list_free(remove);
      g_assert(head_new == slot_objects);
    }

  return slot_objects;
}

void
signal_slot_connect(SignalSlotConnector *self, Signal signal, Slot slot)
{
  g_assert(signal != NULL);
  g_assert(slot != NULL);

  g_mutex_lock(self->lock);

  GList *slots = g_hash_table_lookup(self->connections, signal);

  gboolean signal_registered = (slots != NULL);

  if (_slot_lookup(slots, slot))
    {
      msg_debug("SignalSlotConnector::connect",
                evt_tag_printf("already_connected",
                               "connect(connector=%p,signal=%p,slot=%p)",
                               self, signal, slot));
      goto exit_;
    }

  GList *new_slots = g_list_append(slots, slot);

  if (!signal_registered)
    {
      g_hash_table_insert(self->connections, signal, new_slots);
    }
  else
    {
      g_assert(new_slots == slots);
    }

  msg_debug("SignalSlotConnector::connect",
            evt_tag_printf("new connection registered",
                           "connect(connector=%p,signal=%p,slot=%p)",
                           self, signal, slot));
exit_:
  g_mutex_unlock(self->lock);
}

void
signal_slot_disconnect(SignalSlotConnector *self, Signal signal, Slot slot)
{
  g_assert(signal != NULL);
  g_assert(slot != NULL);

  g_mutex_lock(self->lock);

  GList *slots = g_hash_table_lookup(self->connections, signal);

  if (!slots)
    goto exit_;

  GList *new_slots = _slots_remove_safely(slots, slot);
  g_assert(new_slots == NULL || new_slots == slots);

  msg_debug("SignalSlotConnector::disconnect",
            evt_tag_printf("connector", "%p", self),
            evt_tag_printf("signal", "%p", signal),
            evt_tag_printf("slot", "%p", slot));

  if (!new_slots)
    {
      g_hash_table_remove(self->connections, signal);
      msg_debug("SignalSlotConnector::disconnect last slot is disconnected, unregister signal",
                evt_tag_printf("connector", "%p", self),
                evt_tag_printf("signal", "%p", signal),
                evt_tag_printf("slot", "%p", slot));
    }
exit_:
  g_mutex_unlock(self->lock);
}

static void
_run_slot(gpointer data, gpointer user_data)
{
  g_assert(data);

  Slot slot = (Slot)data;
  slot(user_data);
}

void
signal_slot_emit(SignalSlotConnector *self, Signal signal, gpointer user_data)
{
  g_assert(signal != NULL);

  GList *slots = g_hash_table_lookup(self->connections, signal);

  if (!slots)
    {
      msg_debug("SignalSlotConnector: unregistered signal emitted",
                evt_tag_printf("connector", "%p", self),
                evt_tag_printf("signal", "%p", signal));
      return;
    }

  signal(user_data);

  g_list_foreach(slots, _run_slot, user_data);
}

static void
_destroy_list_of_slots(gpointer data)
{
  if (!data)
    return;

  GList *list = (GList *)data;
  g_list_free(list);
}

SignalSlotConnector *
signal_slot_connector_new(void)
{
  SignalSlotConnector *self = g_new0(SignalSlotConnector, 1);

  self->connections = g_hash_table_new_full(g_direct_hash,
                                            g_direct_equal,
                                            NULL,
                                            _destroy_list_of_slots);

  self->lock = g_mutex_new();

  return self;
}

void
signal_slot_connector_free(SignalSlotConnector *self)
{
  g_mutex_free(self->lock);
  g_hash_table_unref(self->connections);
  g_free(self);
}

