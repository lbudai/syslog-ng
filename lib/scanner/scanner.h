/*
 * Copyright (c) 2016 Balabit
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

typedef struct _Scanner
{
  GString *current_value;

  void (*scan_input)(Scanner *pstate, const gchar *input);
  gboolean (*scan_next)(Scanner *pstate);
  gboolean (*is_scan_finished)(Scanner *pstate);

} Scanner;

void scanner_input(Scanner *pstate, const gchar *input);
gboolean csv_scanner_scan_next(CSVScanner *pstate);
gboolean csv_scanner_is_scan_finished(CSVScanner *pstate);
const gchar *csv_scanner_get_current_value(CSVScanner *pstate);
gint csv_scanner_get_current_value_len(CSVScanner *self);

static inline const gchar *
scanner_get_current_value(CSVScanner *self)
{
  return self->current_value->str;
}

static inline gint
scanner_get_current_value_len(CSVScanner *self)
{
  return self->current_value->len;
}
