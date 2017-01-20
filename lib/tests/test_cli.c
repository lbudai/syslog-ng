/*
 * Copyright (c) 2016-2017 Noémi Ványi
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
 */

#include "cli.h"
#include "testutils.h"
#include "messages.h"

#include <criterion/criterion.h>


Test(cli, test_is_cli_or_command_line_drivers)
{
  Cli *cli = cli_new(NULL, FALSE);
  cr_assert_not(cli->is_cli, "Not command line mode: --cli is not set");
  cr_assert_not(cli->is_command_line_drivers, "Not command line drivers: raw params are NULL");

  cli = cli_new(NULL, TRUE);
  cr_assert(cli->is_cli, "Command line mode: --cli is set");
  cr_assert_not(cli->is_command_line_drivers, "Not command line drivers: raw params are NULL");

  gchar *parser_raw_param[] = {"parser csvparser();"};
  cli = cli_new(parser_raw_param, FALSE);
  cr_assert(cli->is_cli, "Command line mode: command line drivers are passed");
  cr_assert(cli->is_command_line_drivers, "Command line drivers: raw param string is passed");

  cli = cli_new(parser_raw_param, TRUE);
  cr_assert(cli->is_cli, "Command line mode: --cli is set && command line drivers are passed");
  cr_assert(cli->is_command_line_drivers, "Command line drivers: raw param string is passed");
}

Test(cli, test_cli_setup_params)
{
  gboolean command_line_mode = TRUE;

  gchar **parser_raw_param = (char *[])
  { "parser csvparser()", NULL
  };
  Cli *cli = cli_new(parser_raw_param, command_line_mode);
  cr_assert_not(cli_setup_params(cli), "Invalid raw params: missing driver separator");

  parser_raw_param = (char *[])
  { "csvparser();", NULL
  };
  cli = cli_new(parser_raw_param, command_line_mode);
  cr_assert_not(cli_setup_params(cli), "Invalid raw params: missing driver type");

  parser_raw_param = (char *[])
  { "parser;", NULL
  };
  cli = cli_new(parser_raw_param, command_line_mode);
  cr_assert_not(cli_setup_params(cli), "Invalid raw params: missing driver config");

  parser_raw_param = (char *[])
  { "parser json-parser();", "parser json-parser()", NULL
  };
  cli = cli_new(parser_raw_param, command_line_mode);
  cr_assert_not(cli_setup_params(cli), "Invalid raw params: missing driver separator in 2nd item");

  parser_raw_param = (char *[])
  { "parser csvparser();", "parser;", NULL
  };
  cli = cli_new(parser_raw_param, command_line_mode);
  cr_assert_not(cli_setup_params(cli), "Invalid raw params: missing driver config in 2nd item");

  parser_raw_param = (char *[])
  { "parser csvparser();", "csvparser();", NULL
  };
  cli = cli_new(parser_raw_param, command_line_mode);
  cr_assert_not(cli_setup_params(cli), "Invalid raw params: missing driver config in 2nd item");

  parser_raw_param = (char *[])
  { "parser csvparser();", NULL
  };
  cli = cli_new(parser_raw_param, command_line_mode);
  cr_assert(cli_setup_params(cli), "Valid raw params: csvparser");
  cr_assert(g_list_length(cli->params) == 1, "1 parsed driver: csvparser");

  parser_raw_param = (char *[])
  { "parser csvparser(columns(column));", NULL
  };
  cli = cli_new(parser_raw_param, command_line_mode);
  cr_assert(cli_setup_params(cli), "Valid raw params: csvparser");
  cr_assert(g_list_length(cli->params) == 1, "1 parsed driver: csvparser");

  parser_raw_param = (char *[])
  { "parser csvparser(columns(column)); parser json-parser();", NULL
  };
  cli = cli_new(parser_raw_param, command_line_mode);
  cr_assert(cli_setup_params(cli), "Valid raw params: csvparser, json-parser");
  cr_assert(g_list_length(cli->params) == 2, "2 parsed driver: csvparser, json-parser");
}

Test(cli, test_cli_initialize_config_with_one_param)
{
  GlobalConfig *global_config = cfg_new(0);
  GList *params = NULL;
  CliParam *cli_param = cli_param_new("parser", "csvparser();");
  cli_param->name = "my-name";
  params = g_list_append(params, cli_param);
  Cli *cli = cli_new(NULL, TRUE);
  cli->params = params;

  gchar expected_cfg[200];
  gchar *expected_cfg_template = "@version: %s\n"
                                 "@include scl.conf\n"
                                 "source s_stdin { stdin(); };\n"
                                 "destination d_stdout { stdout(); };\n"
                                 " parser my-name { csvparser();};\n"
                                 "log {source(s_stdin); parser(my-name); destination(d_stdout);};";
  g_snprintf(expected_cfg, sizeof(expected_cfg), expected_cfg_template, SYSLOG_NG_VERSION);

  cli_initialize_configuration(cli, global_config);
  cr_assert_eq(strcmp(cli->generated_config, expected_cfg), 0, "Generated: \n%s\nExpected:\n%s", cli->generated_config,
               expected_cfg);
}

Test(cli, test_cli_initialize_config_with_two_params_param)
{
  GlobalConfig *global_config = cfg_new(0);
  GList *params = NULL;
  CliParam *cli_param = cli_param_new("parser", "csvparser();");
  cli_param->name = "my-csvparser-name";
  params = g_list_append(params, cli_param);
  cli_param = cli_param_new("parser", "jsonparser();");
  cli_param->name = "my-jsonparser-name";
  params = g_list_append(params, cli_param);
  Cli *cli = cli_new(NULL, TRUE);
  cli->params = params;

  gchar expected_cfg[300];
  gchar *expected_cfg_template = "@version: %s\n"
                                 "@include scl.conf\n"
                                 "source s_stdin { stdin(); };\n"
                                 "destination d_stdout { stdout(); };\n"
                                 " parser my-csvparser-name { csvparser();};\n"
                                 " parser my-jsonparser-name { jsonparser();};\n"
                                 "log {source(s_stdin); parser(my-csvparser-name); parser(my-jsonparser-name); destination(d_stdout);};";
  g_snprintf(expected_cfg, sizeof(expected_cfg), expected_cfg_template, SYSLOG_NG_VERSION);

  cli_initialize_configuration(cli, global_config);
  cr_assert_eq(strcmp(cli->generated_config, expected_cfg), 0, "Generated: \n%s\nExpected:\n%s", cli->generated_config,
               expected_cfg);
}
