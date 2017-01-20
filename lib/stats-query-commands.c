/*
 * Copyright (c) 2017 Balabit
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

#include "stats-query-commands.h"
#include "stats/stats-query.h"
#include "messages.h"

enum QueryCommands
{
  QUERY_GET = 0,
  QUERY_GET_AGGREGATE,
  QUERY_LIST,
  QUERY_CMD_MAX
};

typedef GString *(*QUERY_CMD)(const gchar *hds_path);

static GString *
_query_get(const gchar *hds_path)
{
  return stats_query_get(hds_path);
}

static GString *
_query_list(const gchar *hds_path)
{
  return stats_query_list(hds_path);
}

static GString *
_query_get_aggregated(const gchar *hds_path)
{
  return stats_query_get_aggregated(hds_path);
}

static gint
_command_str_to_id(const gchar *cmd)
{
  if (g_str_equal(cmd, "GET_AGGREGATED"))
    return QUERY_GET_AGGREGATE;

  if (g_str_equal(cmd, "GET"))
    return QUERY_GET;

  if (g_str_equal(cmd, "LIST"))
    return QUERY_LIST;

  msg_error("syslog-ng-ctl query got unknown command", evt_tag_str("command", cmd));

  return QUERY_CMD_MAX;
}

static QUERY_CMD QUERY_CMDS[] =
{
  _query_get,
  _query_get_aggregated,
  _query_list
};

static GString *
_dispatch_query(gint cmd_id, const gchar *hds_path)
{
  if (cmd_id < QUERY_GET || cmd_id >= QUERY_CMD_MAX)
    {
      msg_error("syslog-ng-ctl got invalid query command", 
                evt_tag_int("cmd_id", cmd_id),
                evt_tag_str("query", hds_path));
      return g_string_new("");
    }

  return QUERY_CMDS[cmd_id](hds_path);
}

GString *
process_query_command(GString *command)
{
  GString *result;
  gchar **cmds = g_strsplit(command->str, " ", 3);

  g_assert(g_str_equal(cmds[0], "QUERY"));

  result = _dispatch_query(_command_str_to_id(cmds[1]), cmds[2]);

  g_strfreev(cmds);
  return result;
}

