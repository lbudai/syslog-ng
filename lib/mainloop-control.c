/*
 * Copyright (c) 2002-2018 Balabit
 * Copyright (c) 1998-2018 Balázs Scheidler
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
#include "mainloop.h"
#include "control/control-commands.h"
#include "control/control-server.h"
#include "messages.h"
#include "cfg.h"
#include "cfg-path.h"
#include "apphook.h"
#include "secret-storage/secret-storage.h"
#include "cfg-walker.h"
#include "logpipe.h"

#include <iv.h>
#include <iv_event.h>
#include <string.h>

typedef struct _ControlCommandAsync
{
  ControlCommand super;
  void (*handler)(void *);
  MainLoop *main_loop;
  struct
  {
    GList *commands;
    GList *connections;
    GMutex lock;
  } args;
  struct iv_event command_requested;
} ControlCommandAsync;

static void
control_connection_message_log(ControlConnection *cc, GString *command, gpointer user_data)
{
  gchar **cmds = g_strsplit(command->str, " ", 3);
  gboolean on;
  int *type = NULL;
  GString *result = g_string_sized_new(128);

  if (!cmds[1])
    {
      g_string_assign(result, "Invalid arguments received, expected at least one argument");
      goto exit;
    }

  if (g_str_equal(cmds[1], "DEBUG"))
    type = &debug_flag;
  else if (g_str_equal(cmds[1], "VERBOSE"))
    type = &verbose_flag;
  else if (g_str_equal(cmds[1], "TRACE"))
    type = &trace_flag;

  if (type)
    {
      if (cmds[2])
        {
          on = g_str_equal(cmds[2], "ON");
          if (*type != on)
            {
              msg_info("Verbosity changed", evt_tag_str("type", cmds[1]), evt_tag_int("on", on));
              *type = on;
            }
        }
      g_string_printf(result, "OK %s=%d", cmds[1], *type);
    }
  else
    g_string_assign(result, "FAIL Invalid arguments received");
exit:
  g_strfreev(cmds);
  control_connection_send_reply(cc, result);
}

static void
control_connection_stop_process(ControlConnection *cc, GString *command, gpointer user_data)
{
  GString *result = g_string_new("OK Shutdown initiated");
  MainLoop *main_loop = (MainLoop *) user_data;

  // We are at ControlLoop which handles the command list, so it is safe
  // to reset. After reset, all incoming ctl requests will be rejected.
  // It could be useful when testing with valgrind and sending commands
  // in an infinite loop.
  reset_control_command_list();
  main_loop_exit(main_loop);

  control_connection_send_reply(cc, result);
}

static void
control_connection_config(ControlConnection *cc, GString *command, gpointer user_data)
{
  MainLoop *main_loop = (MainLoop *) user_data;
  GlobalConfig *config = main_loop_lock_current_config(main_loop);
  GString *result = g_string_sized_new(128);
  gchar **arguments = g_strsplit(command->str, " ", 0);

  if (!config)
    {
      g_string_assign(result, "FAIL syslog-ng is being reloaded");
      goto exit;
    }

  if (g_str_equal(arguments[1], "GET"))
    {
      if (g_str_equal(arguments[2], "ORIGINAL"))
        {
          g_string_assign(result, config->original_config->str);
          goto exit;
        }
      else if (g_str_equal(arguments[2], "PREPROCESSED"))
        {
          g_string_assign(result, config->preprocess_config->str);
          goto exit;
        }
    }

  if (g_str_equal(arguments[1], "VERIFY"))
    {
      main_loop_verify_config(result, main_loop);
      goto exit;
    }

  g_string_assign(result, "FAIL Invalid arguments received");

exit:
  main_loop_unlock_current_config(main_loop);
  g_strfreev(arguments);
  control_connection_send_reply(cc, result);
}

static void
show_ose_license_info(ControlConnection *cc, GString *command, gpointer user_data)
{
  control_connection_send_reply(cc,
                                g_string_new("OK You are using the Open Source Edition of syslog-ng."));
}

static void
_send_response(gpointer data, gpointer user_data)
{
  ControlConnection *cc = (ControlConnection *) data;
  GString *response = (GString *) user_data;
  control_connection_send_reply_async(cc, g_string_new(response->str));
}

static void
_control_command_async_send_response(ControlCommandAsync *cmd, GString *response)
{
  g_mutex_lock(&cmd->args.lock);
  {
    g_list_foreach(cmd->args.connections, _send_response, response);
    g_string_free(response, TRUE);
    g_list_free(cmd->args.commands);
    g_list_free(cmd->args.connections);
    cmd->args.commands = NULL;
    cmd->args.connections = NULL;
  }
  g_mutex_unlock(&cmd->args.lock);
}

static void
_respond_config_reload_status(gint type, gpointer user_data)
{
  gpointer *args = (gpointer *) user_data;
  ControlCommandAsync *cmd = (ControlCommandAsync *) args[0];
  ControlConnection *cc = (ControlConnection *) args[1];
  MainLoop *main_loop = cmd->main_loop;
  GString *reply;

  if (main_loop_was_last_reload_successful(main_loop))
    reply = g_string_new("OK Config reload successful");
  else
    reply = g_string_new("FAIL Config reload failed, reverted to previous config");

  _send_response(cc, reply);
  g_string_free(reply, TRUE);
  g_free(args);
}

static void
__control_connection_reload(gpointer data, gpointer user_data)
{
  main_loop_assert_main_thread();

  ControlConnection *cc = (ControlConnection *) data;
  ControlCommandAsync *cmd = (ControlCommandAsync *) user_data;
  MainLoop *main_loop = cmd->main_loop;

  GError *error = NULL;

  if (!main_loop_reload_config_prepare(main_loop, &error))
    {
      GString *result = g_string_new("");
      g_string_printf(result, "FAIL %s, previous config remained intact", error->message);
      g_clear_error(&error);
      _send_response(cc, result);
      g_string_free(result, TRUE);
      return;
    }
  gpointer *args = g_new0(gpointer, 2);
  args[0] = cmd;
  args[1] = cc;
  register_application_hook(AH_CONFIG_CHANGED, _respond_config_reload_status, args);
  main_loop_reload_config_commence(main_loop);
}

static void
_control_connection_reload(gpointer user_data)
{
  ControlCommandAsync *cmd = (ControlCommandAsync *) user_data;
  g_mutex_lock(&cmd->args.lock);
  {
    g_list_foreach(cmd->args.connections, __control_connection_reload, user_data);
    g_list_free(cmd->args.commands);
    g_list_free(cmd->args.connections);
    cmd->args.commands = NULL;
    cmd->args.connections = NULL;
  }
  g_mutex_unlock(&cmd->args.lock);
}

static void
_control_connection_reopen(gpointer user_data)
{
  main_loop_assert_main_thread();
  ControlCommandAsync *cmd = (ControlCommandAsync *) user_data;
  GString *result = g_string_new("OK Re-open of log destination files initiated");
  app_reopen_files();
  _control_command_async_send_response(cmd, result);
}

static void
control_connection_async_runner(ControlConnection *cc, GString *command, gpointer user_data)
{
  ControlCommandAsync *cmd = (ControlCommandAsync *) user_data;
  gboolean first_reload = FALSE;

  g_mutex_lock(&cmd->args.lock);
  {
    first_reload = cmd->args.commands == NULL;
    cmd->args.commands = g_list_append(cmd->args.commands, command);
    cmd->args.connections = g_list_append(cmd->args.connections, cc);
  }
  g_mutex_unlock(&cmd->args.lock);

  if (first_reload)
    iv_event_post(&cmd->command_requested);
}

static const gchar *
secret_status_to_string(SecretStorageSecretState state)
{
  switch (state)
    {
    case SECRET_STORAGE_STATUS_PENDING:
      return "PENDING";
    case SECRET_STORAGE_SUCCESS:
      return "SUCCESS";
    case SECRET_STORAGE_STATUS_FAILED:
      return "FAILED";
    case SECRET_STORAGE_STATUS_INVALID_PASSWORD:
      return "INVALID_PASSWORD";
    default:
      g_assert_not_reached();
    }
  return "SHOULD NOT BE REACHED";
}

gboolean
secret_storage_status_accumulator(SecretStatus *status, gpointer user_data)
{
  GString *status_str = (GString *) user_data;
  g_string_append_printf(status_str, "%s\t%s\n", status->key, secret_status_to_string(status->state));
  return TRUE;
}

static GString *
process_credentials_status(GString *result)
{
  g_string_assign(result, "OK Credential storage status follows\n");
  secret_storage_status_foreach(secret_storage_status_accumulator, (gpointer) result);
  return result;
}

static GString *
process_credentials_add(GString *result, guint argc, gchar **arguments)
{
  if (argc < 4)
    {
      g_string_assign(result, "FAIL missing arguments to add\n");
      return result;
    }

  gchar *id = arguments[2];
  gchar *secret = arguments[3];

  if (secret_storage_store_secret(id, secret, strlen(secret)))
    g_string_assign(result, "OK Credentials stored successfully\n");
  else
    g_string_assign(result, "FAIL Error while saving credentials\n");

  secret_storage_wipe(secret, strlen(secret));
  return result;
}

static void
process_credentials(ControlConnection *cc, GString *command, gpointer user_data)
{
  gchar **arguments = g_strsplit(command->str, " ", 4);
  guint argc = g_strv_length(arguments);

  GString *result = g_string_new(NULL);

  if (argc < 1)
    {
      g_string_assign(result, "FAIL missing subcommand\n");
      g_strfreev(arguments);
      control_connection_send_reply(cc, result);
      return;
    }

  gchar *subcommand = arguments[1];

  if (strcmp(subcommand, "status") == 0)
    result = process_credentials_status(result);
  else if (g_strcmp0(subcommand, "add") == 0)
    result = process_credentials_add(result, argc, arguments);
  else
    g_string_printf(result, "FAIL invalid subcommand %s\n", subcommand);

  g_strfreev(arguments);
  control_connection_send_reply(cc, result);
}

static void
control_connection_list_files(ControlConnection *cc, GString *command, gpointer user_data)
{
  MainLoop *main_loop = (MainLoop *) user_data;
  GString *result = g_string_new("");

  GlobalConfig *config = main_loop_lock_current_config(main_loop);
  if (!config)
    {
      g_string_assign(result, "FAIL syslog-ng is being reloaded");
      goto exit;
    }
  for (GList *v = config->file_list; v; v = v->next)
    {
      CfgFilePath *cfg_file_path = (CfgFilePath *) v->data;
      g_string_append_printf(result, "%s: %s\n", cfg_file_path->path_type, cfg_file_path->file_path);
    }

  if (result->len == 0)
    g_string_assign(result, "No files available\n");

exit:
  main_loop_unlock_current_config(main_loop);
  control_connection_send_reply(cc, result);
}

static void
_append_arc(Arc *self, gpointer dummy, GPtrArray *arcs)
{
  g_ptr_array_add(arcs, g_strdup_printf("{\"from\" : \"%p\", \"to\" : \"%p\", \"type\" : \"%s\"}",
                                        self->from, self->to,
                                        self->arc_type == ARC_TYPE_NEXT_HOP ? "next_hop" : "pipe_next"));
};

static gchar *
arcs_as_json(GHashTable *arcs)
{
  GPtrArray *arcs_list = g_ptr_array_new_with_free_func(g_free);
  g_hash_table_foreach(arcs, (GHFunc)_append_arc, arcs_list);
  g_ptr_array_add(arcs_list, NULL);

  gchar *arcs_joined = g_strjoinv(", ", (gchar **)arcs_list->pdata);
  g_ptr_array_free(arcs_list, TRUE);

  gchar *json = g_strdup_printf("[%s]", arcs_joined);
  g_free(arcs_joined);

  return json;
}

static GList *
_collect_info(LogPipe *self)
{
  GList *info = g_list_copy_deep(self->info, (GCopyFunc)g_strdup, NULL);

  if (self->plugin_name)
    info = g_list_append(info, g_strdup(self->plugin_name));

  if (self->expr_node)
    {
      gchar buf[128];
      log_expr_node_format_location(self->expr_node, buf, sizeof(buf));
      info = g_list_append(info, g_strdup(buf));
    }

  if (log_pipe_get_persist_name(self))
    info = g_list_append(info, g_strdup(log_pipe_get_persist_name(self)));

  return info;
}

static gchar *
g_str_join_list(GList *self, gchar *separator)
{
  if (!self)
    return g_strdup("");

  if (g_list_length(self) == 1)
    return g_strdup(self->data);

  GString *joined = g_string_new(self->data);
  GList *rest = self->next;

  for (GList *e = rest; e; e = e->next)
    {
      g_string_append(joined, separator);
      g_string_append(joined, e->data);
    }

  return g_string_free(joined, FALSE);
}

static gchar *
_add_quotes(gchar *self)
{
  return g_strdup_printf("\"%s\"", self);
}

static void
_append_node(LogPipe *self, gpointer dummy, GPtrArray *nodes)
{
  GList *raw_info = _collect_info(self);
  GList *info_with_quotes = g_list_copy_deep(raw_info, (GCopyFunc)_add_quotes, NULL);
  g_list_free_full(raw_info, g_free);
  gchar *info = g_str_join_list(info_with_quotes, ", ");
  g_list_free_full(info_with_quotes, g_free);

  g_ptr_array_add(nodes, g_strdup_printf("{\"node\" : \"%p\", \"info\" : [%s]}", self, info));
  g_free(info);
};

static gchar *
nodes_as_json(GHashTable *nodes)
{
  GPtrArray *nodes_list = g_ptr_array_new_with_free_func(g_free);
  g_hash_table_foreach(nodes, (GHFunc)_append_node, nodes_list);
  g_ptr_array_add(nodes_list, NULL);

  gchar *nodes_joined = g_strjoinv(", ", (gchar **)nodes_list->pdata);
  g_ptr_array_free(nodes_list, TRUE);

  gchar *json = g_strdup_printf("[%s]", nodes_joined);
  g_free(nodes_joined);
  return json;
}

static GString *
generate_json(GHashTable *nodes, GHashTable *arcs)
{
  gchar *nodes_part = nodes_as_json(nodes);
  gchar *arcs_part = arcs_as_json(arcs);
  gchar *json = g_strdup_printf("{\"nodes\" : %s, \"arcs\" : %s}", nodes_part, arcs_part);
  GString *result = g_string_new(json);
  g_free(json);
  g_free(nodes_part);
  g_free(arcs_part);

  return result;
}

static void
export_config_graph(ControlConnection *cc, GString *command, gpointer user_data)
{
  GHashTable *nodes;
  GHashTable *arcs;

  MainLoop *main_loop = (MainLoop *) user_data;
  GlobalConfig *cfg = main_loop_lock_current_config(main_loop);
  if (!cfg)
    {
      main_loop_unlock_current_config(main_loop);
      control_connection_send_reply(cc, g_string_new("FAIL {\"fail\" : \"syslog-ng is being reloaded\"}"));
      return;
    }
  cfg_walker_get_graph(cfg->tree.initialized_pipes, &nodes, &arcs);
  GString *result = generate_json(nodes, arcs);
  main_loop_unlock_current_config(main_loop);
  g_hash_table_destroy(nodes);
  g_hash_table_destroy(arcs);

  control_connection_send_reply(cc, result);
}

ControlCommand default_commands_sync[] =
{
  { "LOG", control_connection_message_log },
  { "STOP", control_connection_stop_process },
  { "CONFIG", control_connection_config },
  { "LICENSE", show_ose_license_info },
  { "PWD", process_credentials },
  { "LISTFILES", control_connection_list_files },
  { "EXPORT_CONFIG_GRAPH", export_config_graph },
  { NULL, NULL },
};


ControlCommandAsync default_commands_async[] =
{
  { { "RELOAD", control_connection_async_runner }, _control_connection_reload },
  { { "REOPEN", control_connection_async_runner }, _control_connection_reopen },
  { { NULL, NULL }, NULL },
};

static void
_register_sync_commands(MainLoop *main_loop)
{
  ControlCommand *cmd;

  for (gint i = 0; default_commands_sync[i].command_name != NULL; i++)
    {
      cmd = &default_commands_sync[i];
      control_register_command(cmd->command_name, cmd->func, main_loop);
    }
}

static void
_register_async_commands(MainLoop *main_loop)
{
  ControlCommandAsync *cmd;
  for (gint i = 0; default_commands_async[i].super.command_name != NULL; i++)
    {
      cmd = &default_commands_async[i];
      IV_EVENT_INIT(&cmd->command_requested);
      g_mutex_init(&cmd->args.lock);
      cmd->args.commands = NULL;
      cmd->args.connections = NULL;
      cmd->command_requested.handler = cmd->handler;
      cmd->command_requested.cookie = cmd;
      cmd->main_loop = main_loop;
      iv_event_register(&cmd->command_requested);
      control_register_command(cmd->super.command_name, cmd->super.func, cmd);
    }
}

void
main_loop_register_control_commands(MainLoop *main_loop)
{
  _register_sync_commands(main_loop);
  _register_async_commands(main_loop);
}
