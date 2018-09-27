/*
 * Copyright (c) 2018 Balabit
 * Copyright (c) 2018 László Várady <laszlo.varady@balabit.com>
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

#include "msg-generator-source.h"

#include "logsource.h"
#include "logpipe.h"
#include "timeutils.h"
#include "logmsg/logmsg.h"
#include "template/templates.h"
#include "mainloop.h"

#include <iv.h>
#include <iv_event.h>

struct MsgGeneratorSource
{
  LogSource super;
  MsgGeneratorSourceOptions *options;
  struct iv_task send_like_hell;
  struct iv_event wakeup;
  gboolean suspended;
};

static gboolean
_init(LogPipe *s)
{
  main_loop_assert_main_thread();

  MsgGeneratorSource *self = (MsgGeneratorSource *) s;
  if (!log_source_init(s))
    return FALSE;

  if (!iv_task_registered(&self->send_like_hell))
    iv_task_register(&self->send_like_hell);

  iv_event_register(&self->wakeup);

  return TRUE;
}

static gboolean
_deinit(LogPipe *s)
{
  main_loop_assert_main_thread();

  MsgGeneratorSource *self = (MsgGeneratorSource *) s;

  iv_event_unregister(&self->wakeup);

  if (iv_task_registered(&self->send_like_hell))
    iv_task_unregister(&self->send_like_hell);

  return log_source_deinit(s);
}

static void
_free(LogPipe *s)
{
  log_source_free(s);
}

static void
_send_generated_message(MsgGeneratorSource *self)
{
  main_loop_assert_main_thread();

  LogMessage *msg = log_msg_new_empty();
  log_msg_set_value(msg, LM_V_MESSAGE, "-- Generated message. --", -1);

  if (self->options->template)
    {
      GString *msg_body = g_string_sized_new(128);
      log_template_format(self->options->template, msg, NULL, LTZ_LOCAL, 0, NULL, msg_body);
      log_msg_set_value(msg, LM_V_MESSAGE, msg_body->str, msg_body->len);
      g_string_free(msg_body, TRUE);
    }

  msg_debug("Incoming generated message", evt_tag_str("msg", log_msg_get_value(msg, LM_V_MESSAGE, NULL)));
  log_source_post(&self->super, msg);
}

static void
_send_like_hell(void *cookie)
{
  main_loop_assert_main_thread();

  msg_error("---------------------send_like_hell");
  MsgGeneratorSource *self = (MsgGeneratorSource *) cookie;

  _send_generated_message(self);

  if (log_source_free_to_send(&self->super))
    {
      if (!iv_task_registered(&self->send_like_hell))
        iv_task_register(&self->send_like_hell);
    }
  else
    {
      self->suspended = TRUE;
    }
}

static void
_wakeup(gpointer cookie)
{
  main_loop_assert_main_thread();

  MsgGeneratorSource *self = (MsgGeneratorSource *) cookie;

  if (self->suspended) {
    msg_error("---------------------wakeup real");

    if (!log_source_free_to_send(&self->super))
      msg_error("!!!!!!!!!!!!!!!!!!!!!!!!!!!!! IMPOSSIBLEEEEE");

    self->suspended = FALSE;

    if (!iv_task_registered(&self->send_like_hell))
      iv_task_register(&self->send_like_hell);
  }
}

static void
_schedule_wakeup_please(LogSource *s)
{
  /* not main thread */
  MsgGeneratorSource *self = (MsgGeneratorSource *) s;

  if (self->super.super.flags & PIF_INITIALIZED)
    iv_event_post(&self->wakeup);
}

gboolean
msg_generator_source_init(MsgGeneratorSource *self)
{
  return log_pipe_init(&self->super.super);
}

gboolean
msg_generator_source_deinit(MsgGeneratorSource *self)
{
  return log_pipe_deinit(&self->super.super);
}

void
msg_generator_source_free(MsgGeneratorSource *self)
{
  log_pipe_unref(&self->super.super);
}

void
msg_generator_source_set_options(MsgGeneratorSource *self, MsgGeneratorSourceOptions *options,
                                 const gchar *stats_id, const gchar *stats_instance, gboolean threaded,
                                 gboolean pos_tracked, LogExprNode *expr_node)
{
  log_source_set_options(&self->super, &options->super, stats_id, stats_instance, threaded, pos_tracked, expr_node);

  self->options = options;
}

MsgGeneratorSource *
msg_generator_source_new(GlobalConfig *cfg)
{
  MsgGeneratorSource *self = g_new0(MsgGeneratorSource, 1);
  log_source_init_instance(&self->super, cfg);

  IV_TASK_INIT(&self->send_like_hell);
  self->send_like_hell.cookie = self;
  self->send_like_hell.handler = _send_like_hell;

  IV_EVENT_INIT(&self->wakeup);
  self->wakeup.cookie = self;
  self->wakeup.handler = _wakeup;

  self->super.super.init = _init;
  self->super.super.deinit = _deinit;
  self->super.super.free_fn = _free;
  self->super.wakeup = _schedule_wakeup_please;

  return self;
}
