/*
 * Copyright (c) 2002-2014 Balabit
 * Copyright (c) 2014 Laszlo Budai
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

#include "late_ack_tracker.h"
#include "logsource.h"

typedef struct _LateAckRecord
{
  AckRecord super;
  gboolean acked;
  Bookmark bookmark;
} LateAckRecord;

typedef struct LateAckTracker
{
  AckTracker super;
  LateAckRecord *pending_ack_record;
  GList *ack_record_storage;
  GStaticMutex storage_mutex;
  AckTrackerOnAllAcked on_all_acked;
  GList *ack_record_storage_tail;
} LateAckTracker;

static inline void
late_ack_record_destroy(LateAckRecord *self)
{
  if (self->bookmark.destroy)
    self->bookmark.destroy(&(self->bookmark));
}

void
late_ack_tracker_lock(AckTracker *s)
{
  LateAckTracker *self = (LateAckTracker *)s;
  g_static_mutex_lock(&self->storage_mutex);
}

void
late_ack_tracker_unlock(AckTracker *s)
{
  LateAckTracker *self = (LateAckTracker *)s;
  g_static_mutex_unlock(&self->storage_mutex);
}

void
late_ack_tracker_set_on_all_acked(AckTracker *s, AckTrackerOnAllAckedFunc func, gpointer user_data,
                                  GDestroyNotify user_data_free_fn)
{
  LateAckTracker *self = (LateAckTracker *)s;
  if (self->on_all_acked.user_data && self->on_all_acked.user_data_free_fn)
    {
      self->on_all_acked.user_data_free_fn(self->on_all_acked.user_data);
    }

  self->on_all_acked = (AckTrackerOnAllAcked)
  {
    .func = func,
     .user_data = user_data,
      .user_data_free_fn = user_data_free_fn
  };
}

static inline void
late_ack_tracker_on_all_acked_call(AckTracker *s)
{
  LateAckTracker *self = (LateAckTracker *)s;
  AckTrackerOnAllAcked *handler = &self->on_all_acked;
  if (handler->func)
    {
      handler->func(handler->user_data);
    }
}

static inline gboolean
_ack_range_is_continuous(void *data)
{
  LateAckRecord *ack_rec = (LateAckRecord *)data;

  return ack_rec->acked;
}

static void
late_ack_tracker_track_msg(AckTracker *s, LogMessage *msg)
{
  LateAckTracker *self = (LateAckTracker *)s;

  g_assert(self->pending_ack_record != NULL);

  msg->ack_record = (AckRecord *)self->pending_ack_record;
  self->pending_ack_record->bookmark.persist_state = msg->source->super.cfg->state;

  late_ack_tracker_lock(s);
  {
    g_assert(self->pending_ack_record != NULL);
    msg->ack_record = (AckRecord *)self->pending_ack_record;
    self->ack_record_storage = g_list_prepend(self->ack_record_storage, msg->ack_record);
    if (!self->ack_record_storage_tail)
      self->ack_record_storage_tail = self->ack_record_storage;
    self->pending_ack_record = NULL;
  }
  late_ack_tracker_unlock(s);

}

static void
late_ack_tracker_manage_msg_ack(AckTracker *s, LogMessage *msg, AckType ack_type)
{
  LateAckTracker *self = (LateAckTracker *)s;
  LateAckRecord *ack_rec = (LateAckRecord *)msg->ack_record;
  LateAckRecord *last_in_range = NULL;
  GList *continuous_range_head = NULL;
  guint32 ack_range_length = 0;

  if (ack_type == AT_SUSPENDED)
    log_source_flow_control_suspend(msg->source);

  late_ack_tracker_lock(s);
  {
    ack_rec->acked = TRUE;

    GList *it = NULL;

    g_assert(self->ack_record_storage != NULL && self->ack_record_storage_tail != NULL);

    for (it = self->ack_record_storage_tail; it != NULL && _ack_range_is_continuous(it->data); it = it->prev)
      ++ack_range_length;

    if (it && !_ack_range_is_continuous(it->data))
      continuous_range_head = it->next;
    else
      continuous_range_head = it;

    if (ack_range_length > 0)
      {
        if (continuous_range_head == NULL)
          {
            continuous_range_head = self->ack_record_storage;
          }

        last_in_range = (LateAckRecord *)continuous_range_head->data;
        if (ack_type != AT_ABORTED)
          {
            Bookmark *bookmark = &(last_in_range->bookmark);
            bookmark->save(bookmark);
          }
        if (continuous_range_head == self->ack_record_storage)
          {
            g_assert(g_list_length(continuous_range_head) == g_list_length(self->ack_record_storage));
            g_list_free_full(self->ack_record_storage, g_free);
            self->ack_record_storage = self->ack_record_storage_tail = NULL;
            msg->ack_record = NULL;
          }
        else
          {
            it = self->ack_record_storage_tail;
            continuous_range_head->prev->next = NULL; //detach continuous range
            self->ack_record_storage_tail = continuous_range_head->prev;
            continuous_range_head->prev = NULL;
            msg->ack_record = NULL;
            g_assert(g_list_length(continuous_range_head) == ack_range_length);
            g_list_free_full(continuous_range_head, g_free);
          }
        if (ack_type == AT_SUSPENDED)
          log_source_flow_control_adjust(msg->source); //TODO: check
        else
          log_source_flow_control_adjust(msg->source);

        if (!self->ack_record_storage || g_list_length(self->ack_record_storage) == 0)
          late_ack_tracker_on_all_acked_call(s);
      }
  }
  late_ack_tracker_unlock(s);

  log_msg_unref(msg);
}

gboolean
late_ack_tracker_is_empty(AckTracker *s)
{
  LateAckTracker *self = (LateAckTracker *)s;
  return !self->ack_record_storage || g_list_length(self->ack_record_storage) == 0;
}

static LateAckRecord *
_alloc_ack_record(void)
{
  LateAckRecord *record = g_new0(LateAckRecord, 1);

  return record;
}

static Bookmark *
late_ack_tracker_request_bookmark(AckTracker *s)
{
  LateAckTracker *self = (LateAckTracker *)s;

  late_ack_tracker_lock(s);
  {
    if (!self->pending_ack_record)
      self->pending_ack_record = _alloc_ack_record();

    if (self->pending_ack_record)
      {
        self->pending_ack_record->super.tracker = (AckTracker *)self;
        late_ack_tracker_unlock(s);

        return &(self->pending_ack_record->bookmark);
      }
  }
  late_ack_tracker_unlock(s);

  return NULL;
}

static void
late_ack_tracker_free(AckTracker *s)
{
  LateAckTracker *self = (LateAckTracker *)s;
  AckTrackerOnAllAcked *handler = &self->on_all_acked;

  if (handler->user_data_free_fn && handler->user_data)
    {
      handler->user_data_free_fn(handler->user_data);
    }

  g_static_mutex_free(&self->storage_mutex);

  g_list_free_full(self->ack_record_storage, g_free);
  self->ack_record_storage = NULL;

  g_free(self);
}

static void
_setup_callbacks(LateAckTracker *self)
{
  self->super.request_bookmark = late_ack_tracker_request_bookmark;
  self->super.track_msg = late_ack_tracker_track_msg;
  self->super.manage_msg_ack = late_ack_tracker_manage_msg_ack;
  self->super.free_fn = late_ack_tracker_free;
}

static void
late_ack_tracker_init_instance(LateAckTracker *self)
{
  g_static_mutex_init(&self->storage_mutex);
  _setup_callbacks(self);
}

AckTracker *
late_ack_tracker_new(void)
{
  LateAckTracker *self = g_new0(LateAckTracker, 1);

  late_ack_tracker_init_instance(self);

  return (AckTracker *)self;
}

