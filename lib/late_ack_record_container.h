/*
 * Copyright (c) 2002-2019 Balabit
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

#ifndef ACK_RECORD_CONTAINER_H_INCLUDED
#define ACK_RECORD_CONTAINER_H_INCLUDED

typedef struct _LateAckRecord
{
  AckRecord super;
  gboolean acked;
  Bookmark bookmark;
} LateAckRecord;

typedef struct _LateAckRecordContainer LateAckRecordContainer;

struct _LateAckRecordContainer
{
  gboolean (*is_empty)(const LateAckRecordContainer *s);
  LateAckRecord * (*request_pending)(LateAckRecordContainer *s);
  void (*store_pending)(LateAckRecordContainer *s);
  void (*drop)(LateAckRecordContainer *s, gsize n);
  LateAckRecord *(*at)(const LateAckRecordContainer *s, gsize idx);
  void (*free)(LateAckRecordContainer *s);
  gsize (*size)(const LateAckRecordContainer *s);
  gsize (*get_continual_range_length)(const LateAckRecordContainer *s);
};

static void
late_ack_record_destroy(LateAckRecord *self)
{
  if (self->bookmark.destroy)
    self->bookmark.destroy(&(self->bookmark));
}

static gboolean
late_ack_record_container_is_empty(LateAckRecordContainer *s)
{
  return s->is_empty(s);
}

static LateAckRecord *
late_ack_record_container_request_pending(LateAckRecordContainer *s)
{
  return s->request_pending(s);
}

static void
late_ack_record_container_store_pending(LateAckRecordContainer *s)
{
  s->store_pending(s);
}

static void
late_ack_record_container_drop(LateAckRecordContainer *s, gsize n)
{
  s->drop(s, n);
}

static LateAckRecord *
late_ack_record_container_at(const LateAckRecordContainer *s, gsize idx)
{
  return s->at(s, idx);
}

static void
late_ack_record_container_free(LateAckRecordContainer *s)
{
  s->free(s);
}

static gsize
late_ack_record_container_size(const LateAckRecordContainer *s)
{
  return s->size(s);
}

static gsize
late_ack_record_container_get_continual_range_length(const LateAckRecordContainer *s)
{
  return s->get_continual_range_length(s);
}

#endif

