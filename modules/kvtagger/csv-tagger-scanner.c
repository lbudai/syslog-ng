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

#include "csv-tagger-scanner.h"
#include "scanner/csv-scanner/csv-scanner.h"


gboolean
_get_next_record(CSVTaggerScanner *self, gchar *input, database_record *last_record)
{
  csv_scanner_input(&self->scanner, input);
  if (!csv_scanner_scan_next(&self->scanner))
      return FALSE;
  last_record->selector = csv_scanner_dup_current_value(&self->scanner);
  if (!csv_scanner_scan_next(&self->scanner))
      return FALSE;
  last_record->name = csv_scanner_dup_current_value(&self->scanner);
  if (!csv_scanner_scan_next(&self->scanner))
      return FALSE;
  last_record->value = csv_scanner_dup_current_value(&self->scanner);

  return TRUE;
}

static GArray* csv_tagger_scanner_get_parsed_records(TaggerScanner *s, FILE *file)
{
  CSVTaggerScanner *self = (CSVTaggerScanner *) s;
  gchar line[3072];
  database_record last_record;
  GArray *nv_array = g_array_new(FALSE, FALSE, sizeof(database_record));
  while(fscanf(file,"%3071s\n", line) == 1)
    {
      if (_get_next_record(self, line, &last_record))
        {
          g_array_append_val(nv_array, last_record);
        }
      else
        {
          g_array_free(nv_array, TRUE);
          return NULL;
        }
    }
  return nv_array;
}

void
_get_columns(GList* columns)
{
  columns = g_list_append(columns, "selector");
  columns = g_list_append(columns, "name");
  columns = g_list_append(columns, "value");
}

CSVTaggerScanner*
csv_tagger_scanner_new()
{
  CSVTaggerScanner *self = g_new0(CSVTaggerScanner, 1);
  csv_scanner_options_set_delimiters(&self->options, ",");
  csv_scanner_options_set_quote_pairs(&self->options, "\"\"''");
  GList *columns = NULL;
  _get_columns(columns);
  csv_scanner_options_set_columns(&self->options, columns);
  csv_scanner_options_set_flags(&self->options, CSV_SCANNER_STRIP_WHITESPACE);
  csv_scanner_options_set_dialect(&self->options, CSV_SCANNER_ESCAPE_NONE);
  csv_scanner_state_init(&self->scanner, &self->options);
  self->super.get_parsed_records = csv_tagger_scanner_get_parsed_records;
  return self;
}
