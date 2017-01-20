/*
 * Copyright (c) 2016-2017 Noémi Ványi
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

#ifndef CLI_H_INCLUDED
#define CLI_H_INCLUDED

#include "cfg.h"

#include <glib.h>

typedef struct _CliParam
{
  gchar *name;
  gchar *cfg;
  gchar *type;
} CliParam;

CliParam *cli_param_new(gchar *type, gchar *cfg);

typedef struct _Cli
{
  gchar **raw_params;
  GList *params;
  gboolean is_cli;
  gboolean is_command_line_drivers;
  gchar *generated_config;
} Cli;

Cli *cli_new(gchar **params, gboolean is_cli_param);
gboolean cli_setup_params(Cli *cli);
void cli_initialize_configuration(Cli *cli, GlobalConfig *global_config);
gboolean cli_write_generated_config_to_file(Cli *cli, gchar* filename);

#endif
