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

#ifndef ACK_TRACKER_H_INCLUDED
#define ACK_TRACKER_H_INCLUDED

#include "syslog-ng.h"
#include "logmsg/logmsg.h"
#include "messages.h"

struct _AckTracker
{
  gboolean late;
  Bookmark *(*request_bookmark)(AckTracker *self);
  void (*track_msg)(AckTracker *self, LogMessage *msg);
  void (*manage_msg_ack)(AckTracker *self, LogMessage *msg, AckType ack_type);
};

struct _AckRecord
{
  AckTracker *tracker;
};

AckTracker *late_ack_tracker_new(void);
AckTracker *early_ack_tracker_new(void);

void late_ack_tracker_free(AckTracker *self);
void early_ack_tracker_free(AckTracker *self);

static inline void
ack_tracker_free(AckTracker *self)
{
  if (self)
    {
      if (self->late)
        late_ack_tracker_free(self);
      else
        early_ack_tracker_free(self);
    }
}

static inline gboolean
ack_tracker_is_late(AckTracker *self)
{
  return self->late;
}

static inline Bookmark *
ack_tracker_request_bookmark(AckTracker *self)
{
  return self->request_bookmark(self);
}

static inline void
ack_tracker_track_msg(AckTracker *self, LogMessage *msg)
{
  msg_trace("ack_tracker_track_msg", evt_tag_int("allocated bytes", msg->allocated_bytes));
  self->track_msg(self, msg);
}

static inline void
ack_tracker_manage_msg_ack(AckTracker *self, LogMessage *msg, AckType ack_type)
{
  msg_trace("ack_tracker_manage_msg_ack", evt_tag_int("allocated bytes", msg->allocated_bytes));
  self->manage_msg_ack(self, msg, ack_type);
  log_msg_unref(msg);
}

#endif
