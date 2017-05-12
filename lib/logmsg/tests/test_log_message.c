/*
 * Copyright (c) 2002-2016 Balabit
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

#include "msg_parse_lib.h"
#include "apphook.h"
#include "logpipe.h"
#include "rcptid.h"
#include "libtest/persist_lib.h"

#include <stdlib.h>

typedef struct _LogMessageTestParams
{
  LogMessage *message;
  LogMessage *cloned_message;
  NVHandle nv_handle;
  NVHandle sd_handle;
  const gchar *tag_name;
} LogMessageTestParams;

static LogMessage *
_construct_log_message(void)
{
  const gchar *raw_msg = "foo";
  LogMessage *msg;

  msg = log_msg_new(raw_msg, strlen(raw_msg), NULL, &parse_options);
  log_msg_set_value(msg, LM_V_HOST, raw_msg, -1);
  return msg;
}

static LogMessage *
_construct_merge_base_message(void)
{
  LogMessage *msg;

  msg = log_msg_new_empty();
  log_msg_set_value_by_name(msg, "base", "basevalue", -1);
  log_msg_set_tag_by_name(msg, "basetag");
  return msg;
}

static LogMessage *
_construct_merged_message(const gchar *name, const gchar *value)
{
  LogMessage *msg;

  msg = log_msg_new_empty();
  log_msg_set_value_by_name(msg, name, value, -1);
  log_msg_set_tag_by_name(msg, "mergedtag");
  return msg;
}

static void
assert_log_msg_clear_clears_all_properties(LogMessage *message, NVHandle nv_handle,
                                           NVHandle sd_handle, const gchar *tag_name)
{
  log_msg_clear(message);

  cr_assert_str_empty(log_msg_get_value(message, nv_handle, NULL),
                      "Message still contains value after log_msg_clear");

  cr_assert_str_empty(log_msg_get_value(message, sd_handle, NULL),
                      "Message still contains sdata value after log_msg_clear");

  cr_assert_null(message->saddr, "Message still contains an saddr after log_msg_clear");
  cr_assert_not(log_msg_is_tag_by_name(message, tag_name),
                "Message still contains a valid tag after log_msg_clear");
}

static void
assert_sdata_value_with_seqnum_equals(LogMessage *msg, guint32 seq_num, const gchar *expected)
{
  GString *result = g_string_sized_new(0);

  log_msg_append_format_sdata(msg, result, seq_num);
  cr_assert_str_eq(result->str, expected, "SDATA value does not match");
  g_string_free(result, TRUE);
}

static void
assert_sdata_value_equals(LogMessage *msg, const gchar *expected)
{
  assert_sdata_value_with_seqnum_equals(msg, 0, expected);
}

static LogMessageTestParams *
log_message_test_params_new(void)
{
  LogMessageTestParams *params = g_new0(LogMessageTestParams, 1);

  params->tag_name = "tag";
  params->message = _construct_log_message();

  params->nv_handle = log_msg_get_value_handle("foo");
  params->sd_handle = log_msg_get_value_handle(".SDATA.foo.bar");

  log_msg_set_value(params->message, params->nv_handle, "value", -1);
  log_msg_set_value(params->message, params->sd_handle, "value", -1);
  params->message->saddr = g_sockaddr_inet_new("1.2.3.4", 5050);
  log_msg_set_tag_by_name(params->message, params->tag_name);

  return params;
}

void
log_message_test_params_free(LogMessageTestParams *params)
{
  log_msg_unref(params->message);

  if (params->cloned_message)
    log_msg_unref(params->cloned_message);

  g_free(params);
}

LogMessage *
log_message_test_params_clone_message(LogMessageTestParams *params)
{
  LogPathOptions path_options = LOG_PATH_OPTIONS_INIT;
  params->cloned_message = log_msg_clone_cow(params->message, &path_options);

  return params->cloned_message;
}


void
setup(void)
{
  app_startup();
  init_and_load_syslogformat_module();
}

void
teardown(void)
{
  deinit_syslogformat_module();
  app_shutdown();
}

TestSuite(log_message, .init = setup, .fini = teardown);

Test(log_message, test_log_message_can_be_created_and_freed)
{
  LogMessage *msg = _construct_log_message();
  log_msg_unref(msg);
}

Test(log_message, test_log_message_can_be_cleared)
{
  LogMessageTestParams *params = log_message_test_params_new();

  log_message_test_params_clone_message(params);

  assert_log_msg_clear_clears_all_properties(params->message, params->nv_handle,
                                             params->sd_handle, params->tag_name);
  assert_log_msg_clear_clears_all_properties(params->cloned_message, params->nv_handle,
                                             params->sd_handle, params->tag_name);

  log_message_test_params_free(params);
}

Test(log_message, test_rcptid_is_automatically_assigned_to_a_newly_created_log_message)
{
  LogMessage *msg;
  PersistState *state = clean_and_create_persist_state_for_test("test_values.persist");
  rcptid_init(state, TRUE);

  msg = log_msg_new_empty();
  cr_assert_eq(msg->rcptid, 1, "rcptid is not automatically set");
  log_msg_unref(msg);

  commit_and_destroy_persist_state(state);
  rcptid_deinit();
}

Test(log_message, test_log_message_merge_with_empty_context)
{
  LogMessageTestParams *params = log_message_test_params_new();
  LogMessage *context[] = {};

  log_message_test_params_clone_message(params);

  log_msg_merge_context(params->message, context, 0);
  assert_log_messages_equal(params->message, params->cloned_message);

  log_message_test_params_free(params);
}

Test(log_message, test_log_message_merge_unset_value)
{
  LogMessage *msg;
  GPtrArray *context = g_ptr_array_sized_new(0);

  msg = _construct_merge_base_message();
  g_ptr_array_add(context, _construct_merged_message("merged", "mergedvalue"));
  log_msg_merge_context(msg, (LogMessage **) context->pdata, context->len);

  assert_log_message_value_by_name(msg, "base", "basevalue");
  assert_log_message_value_by_name(msg, "merged", "mergedvalue");
  g_ptr_array_foreach(context, (GFunc) log_msg_unref, NULL);
  g_ptr_array_free(context, TRUE);
  log_msg_unref(msg);
}

Test(log_message, test_log_message_merge_doesnt_overwrite_already_set_values)
{
  LogMessage *msg;
  GPtrArray *context = g_ptr_array_sized_new(0);

  msg = _construct_merge_base_message();
  g_ptr_array_add(context, _construct_merged_message("base", "mergedvalue"));
  log_msg_merge_context(msg, (LogMessage **) context->pdata, context->len);

  assert_log_message_value_by_name(msg, "base", "basevalue");
  g_ptr_array_foreach(context, (GFunc) log_msg_unref, NULL);
  g_ptr_array_free(context, TRUE);
  log_msg_unref(msg);
}

Test(log_message, test_log_message_merge_merges_the_closest_value_in_the_context)
{
  LogMessage *msg;
  GPtrArray *context = g_ptr_array_sized_new(0);

  msg = _construct_merge_base_message();
  g_ptr_array_add(context, _construct_merged_message("merged", "mergedvalue1"));
  g_ptr_array_add(context, _construct_merged_message("merged", "mergedvalue2"));
  log_msg_merge_context(msg, (LogMessage **) context->pdata, context->len);

  assert_log_message_value_by_name(msg, "merged", "mergedvalue2");
  g_ptr_array_foreach(context, (GFunc) log_msg_unref, NULL);
  g_ptr_array_free(context, TRUE);
  log_msg_unref(msg);
}

Test(log_message, test_log_message_merge_merges_from_all_messages_in_the_context)
{
  LogMessage *msg;
  GPtrArray *context = g_ptr_array_sized_new(0);

  msg = _construct_merge_base_message();
  g_ptr_array_add(context, _construct_merged_message("merged1", "mergedvalue1"));
  g_ptr_array_add(context, _construct_merged_message("merged2", "mergedvalue2"));
  g_ptr_array_add(context, _construct_merged_message("merged3", "mergedvalue3"));
  log_msg_merge_context(msg, (LogMessage **) context->pdata, context->len);

  assert_log_message_value_by_name(msg, "merged1", "mergedvalue1");
  assert_log_message_value_by_name(msg, "merged2", "mergedvalue2");
  assert_log_message_value_by_name(msg, "merged3", "mergedvalue3");
  g_ptr_array_foreach(context, (GFunc) log_msg_unref, NULL);
  g_ptr_array_free(context, TRUE);
  log_msg_unref(msg);
}

Test(log_message, test_log_message_merge_leaves_base_tags_intact)
{
  LogMessage *msg;
  GPtrArray *context = g_ptr_array_sized_new(0);

  msg = _construct_merge_base_message();
  g_ptr_array_add(context, _construct_merged_message("merged1", "mergedvalue1"));
  log_msg_merge_context(msg, (LogMessage **) context->pdata, context->len);

  assert_log_message_has_tag(msg, "basetag");
  assert_log_message_doesnt_have_tag(msg, "mergedtag");
  g_ptr_array_foreach(context, (GFunc) log_msg_unref, NULL);
  g_ptr_array_free(context, TRUE);
  log_msg_unref(msg);
}

Test(log_message, test_log_msg_set_value_indirect_with_self_referencing_handle_results_in_a_nonindirect_value)
{
  LogMessageTestParams *params = log_message_test_params_new();
  gssize value_len;

  log_msg_set_value_indirect(params->message, params->nv_handle, params->nv_handle, 0, 0, 5);
  cr_assert_str_eq(log_msg_get_value(params->message, params->nv_handle, &value_len), "value",
                   "indirect self-reference value doesn't match");

  log_message_test_params_free(params);
}

Test(log_message, test_log_msg_get_value_with_time_related_macro)
{
  LogMessage *msg;
  gssize value_len;
  NVHandle handle;
  const char *date_value;

  msg = log_msg_new_empty();
  msg->timestamps[LM_TS_STAMP].tv_sec = 1389783444;
  msg->timestamps[LM_TS_STAMP].zone_offset = 3600;

  handle = log_msg_get_value_handle("ISODATE");
  date_value = log_msg_get_value(msg, handle, &value_len);
  cr_assert_str_eq(date_value, "2014-01-15T11:57:24+01:00", "ISODATE macro value does not match! value=%s", date_value);

  log_msg_unref(msg);
}

Test(log_message, test_local_logmsg_created_with_the_right_flags_and_timestamps)
{
  LogMessage *msg = log_msg_new_local();

  gboolean are_equals = log_stamp_eq(&msg->timestamps[LM_TS_STAMP], &msg->timestamps[LM_TS_RECVD]);

  cr_assert_neq((msg->flags & LF_LOCAL), 0, "LogMessage created by log_msg_new_local() should have LF_LOCAL flag set");
  cr_assert(are_equals, "The timestamps in a LogMessage created by log_msg_new_local() should be equals");
}

Test(log_message, test_sdata_value_is_updated_by_sdata_name_value_pairs)
{
  LogMessage *msg;

  msg = log_msg_new_empty();
  log_msg_set_value_by_name(msg, ".SDATA.foo.bar1", "value", -1);
  assert_sdata_value_equals(msg, "[foo bar1=\"value\"]");
  log_msg_set_value_by_name(msg, ".SDATA.foo.bar2", "value", -1);
  assert_sdata_value_equals(msg, "[foo bar1=\"value\" bar2=\"value\"]");
  log_msg_set_value_by_name(msg, ".SDATA.foo.bar3", "value", -1);
  assert_sdata_value_equals(msg, "[foo bar1=\"value\" bar2=\"value\" bar3=\"value\"]");
  log_msg_set_value_by_name(msg, ".SDATA.post.value1", "value", -1);
  assert_sdata_value_equals(msg, "[post value1=\"value\"][foo bar1=\"value\" bar2=\"value\" bar3=\"value\"]");
  log_msg_set_value_by_name(msg, ".SDATA.post.value2", "value", -1);
  assert_sdata_value_equals(msg,
                            "[post value1=\"value\" value2=\"value\"][foo bar1=\"value\" bar2=\"value\" bar3=\"value\"]");
  log_msg_unref(msg);
}

Test(log_message, test_sdata_seqnum_adds_meta_sequence_id)
{
  LogMessage *msg;

  msg = log_msg_new_empty();
  log_msg_set_value_by_name(msg, ".SDATA.foo.bar1", "value", -1);
  log_msg_set_value_by_name(msg, ".SDATA.foo.bar2", "value", -1);
  log_msg_set_value_by_name(msg, ".SDATA.foo.bar3", "value", -1);
  assert_sdata_value_with_seqnum_equals(msg, 5,
                                        "[foo bar1=\"value\" bar2=\"value\" bar3=\"value\"][meta sequenceId=\"5\"]");
  log_msg_set_value_by_name(msg, ".SDATA.meta.foobar", "value", -1);
  assert_sdata_value_with_seqnum_equals(msg, 6,
                                        "[meta sequenceId=\"6\" foobar=\"value\"][foo bar1=\"value\" bar2=\"value\" bar3=\"value\"]");
  log_msg_unref(msg);
}

Test(log_message, test_sdata_value_omits_unset_values)
{
  LogMessage *msg;

  msg = log_msg_new_empty();
  log_msg_set_value_by_name(msg, ".SDATA.foo.bar1", "value", -1);
  log_msg_set_value_by_name(msg, ".SDATA.foo.bar2", "value", -1);
  log_msg_set_value_by_name(msg, ".SDATA.foo.bar3", "value", -1);
  assert_sdata_value_equals(msg, "[foo bar1=\"value\" bar2=\"value\" bar3=\"value\"]");
  log_msg_unset_value_by_name(msg, ".SDATA.foo.bar2");
  assert_sdata_value_equals(msg, "[foo bar1=\"value\" bar3=\"value\"]");
  log_msg_unset_value_by_name(msg, ".SDATA.foo.bar1");
  log_msg_unset_value_by_name(msg, ".SDATA.foo.bar3");
  assert_sdata_value_equals(msg, "");
  log_msg_unref(msg);
}

#define DEFUN_KEY_VALUE(name, key, value, size) \
  gchar name ## _key[size]; \
  gchar name ## _value[size]; \
  name ## _key[size-1] = name ## _value[size-1] = 0; \
  memset(name ## _key, key, sizeof(name ##_key)-1); \
  memset(name ## _value, value, sizeof(name ##_value)-1); \


typedef struct
{
  guint32 nvtable_size_old;
  guint32 nvtable_size_new;
  guint32 msg_size_old;
  guint32 msg_size_new;
} sizes_t;

sizes_t
add_kv_and_check(LogMessage *msg, gchar *key, gchar *value)
{
  sizes_t sizes;
  sizes.msg_size_old = log_msg_get_size(msg);
  sizes.nvtable_size_old = nv_table_get_memory_consumption(msg->payload);
  log_msg_set_value_by_name(msg, key, value, -1);
  sizes.nvtable_size_new = nv_table_get_memory_consumption(msg->payload);
  sizes.msg_size_new = log_msg_get_size(msg);
  return sizes;
}

Test(log_message, test_message_size)
{
  LogMessage *msg = log_msg_new_empty();

  sizes_t sizes;
  DEFUN_KEY_VALUE(small, 'C', 'D', 9);
  sizes = add_kv_and_check(msg, small_key, small_value);
  // 9*'C'+'\0' + 9*'D'+'\0'
  guint32 entry_size = NV_ENTRY_DIRECT_HDR + 9 + 9 + 2;
  cr_assert_eq(sizes.nvtable_size_old + entry_size, sizes.nvtable_size_new);
  cr_assert_eq(sizes.msg_size_old + entry_size, sizes.msg_size_new);  // Size increased because of nvtable

  guint32 msg_size = sizes.msg_size_new;
  log_msg_set_tag_by_name(msg, "test_tag_storage");
  cr_assert(log_msg_is_tag_by_name(msg, "test_tag_storage"));
  cr_assert_eq(msg_size, log_msg_get_size(msg)); // Tag is not increased until tag id 65

  char *tag_name = strdup("00tagname");
  // (*8 to convert ot bits) + no need plus 1 bcause we already added one tag: test_tag_storage
  for (int i = 0; i < GLIB_SIZEOF_LONG*8; i++)
    {
      sprintf(tag_name, "%dtagname", i);
      log_msg_set_tag_by_name(msg, tag_name);
    }
  free(tag_name);
  cr_assert_eq(msg_size + 2*GLIB_SIZEOF_LONG, log_msg_get_size(msg));


  DEFUN_KEY_VALUE(big, 'A', 'B', 256);
  sizes = add_kv_and_check(msg, big_key, big_value); // nvtable is expanded
  entry_size = NV_ENTRY_DIRECT_HDR + 255 + 255 + 2;
  cr_assert_eq(sizes.nvtable_size_old + entry_size, sizes.nvtable_size_new); // but only increased by the entry
  cr_assert_eq(sizes.msg_size_old + entry_size, sizes.msg_size_new);  // nvtable is doubled

  log_msg_unref(msg);

  msg = log_msg_new_empty();

  guint32 old_size = log_msg_get_size(msg);
  log_msg_set_value_by_name(msg, ".SDATA.t1", "sdataVa1", -1);
  cr_assert_eq(msg->alloc_sdata, 8);
  guint32 new_size = log_msg_get_size(msg);
  int delta = (msg->sdata?sizeof(msg->sdata[0]) * msg->alloc_sdata : 0) + NV_ENTRY_DIRECT_HDR + 10*2;
  cr_assert_eq(new_size, old_size + delta);

  log_msg_unref(msg);
}
