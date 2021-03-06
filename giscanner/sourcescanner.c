/* GObject introspection: public scanner api
 *
 * Copyright (C) 2007 Jürg Billeter
 * Copyright (C) 2008 Johan Dahlin
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 *
 */

#include "sourcescanner.h"
#include <string.h>
#include <gio/gio.h>

GISourceSymbol *
gi_source_symbol_new (GISourceSymbolType type, const gchar *filename, int line)
{
  GISourceSymbol *s = g_slice_new0 (GISourceSymbol);
  s->ref_count = 1;
  s->source_filename = g_strdup (filename);
  s->type = type;
  s->line = line;
  return s;
}

void
ctype_free (GISourceType * type)
{
  g_free (type->name);
  g_list_foreach (type->child_list, (GFunc)gi_source_symbol_unref, NULL);
  g_list_free (type->child_list);
  g_slice_free (GISourceType, type);
}

GISourceSymbol *
gi_source_symbol_ref (GISourceSymbol * symbol)
{
  symbol->ref_count++;
  return symbol;
}

void
gi_source_symbol_unref (GISourceSymbol * symbol)
{
  if (!symbol)
    return;
  symbol->ref_count--;
  if (symbol->ref_count == 0)
    {
      g_free (symbol->ident);
      if (symbol->base_type)
        ctype_free (symbol->base_type);
      g_free (symbol->const_string);
      g_free (symbol->source_filename);
      g_slice_free (GISourceSymbol, symbol);
    }
}
 
gboolean
gi_source_symbol_get_const_boolean (GISourceSymbol * symbol)
{
  return (symbol->const_int_set && symbol->const_int) || symbol->const_string;
}

/* use specified type as base type of symbol */
void
gi_source_symbol_merge_type (GISourceSymbol *symbol,
			     GISourceType   *type)
{
  GISourceType **foundation_type = &(symbol->base_type);

  while (*foundation_type != NULL)
    {
      foundation_type = &((*foundation_type)->base_type);
    }
  *foundation_type = type;
}


GISourceType *
gi_source_type_new (GISourceTypeType type)
{
  GISourceType *t = g_slice_new0 (GISourceType);
  t->type = type;
  return t;
}

GISourceType *
gi_source_type_copy (GISourceType * type)
{
  GList *l;
  GISourceType *result = g_slice_new0 (GISourceType);
  result->type = type->type;
  result->storage_class_specifier = type->storage_class_specifier;
  result->type_qualifier = type->type_qualifier;
  result->function_specifier = type->function_specifier;
  if (type->name)
    result->name = g_strdup (type->name);
  if (type->base_type)
    result->base_type = gi_source_type_copy (type->base_type);
  for (l = type->child_list; l; l = l->next)
    result->child_list = g_list_append (result->child_list, gi_source_symbol_ref (l->data));
  result->is_bitfield = type->is_bitfield;
  return result;
}

GISourceType *
gi_source_basic_type_new (const char *name)
{
  GISourceType *basic_type = gi_source_type_new (CTYPE_BASIC_TYPE);
  basic_type->name = g_strdup (name);
  return basic_type;
}

GISourceType *
gi_source_typedef_new (const char *name)
{
  GISourceType *typedef_ = gi_source_type_new (CTYPE_TYPEDEF);
  typedef_->name = g_strdup (name);
  return typedef_;
}

GISourceType *
gi_source_struct_new (const char *name)
{
  GISourceType *struct_ = gi_source_type_new (CTYPE_STRUCT);
  struct_->name = g_strdup (name);
  return struct_;
}

GISourceType *
gi_source_union_new (const char *name)
{
  GISourceType *union_ = gi_source_type_new (CTYPE_UNION);
  union_->name = g_strdup (name);
  return union_;
}

GISourceType *
gi_source_enum_new (const char *name)
{
  GISourceType *enum_ = gi_source_type_new (CTYPE_ENUM);
  enum_->name = g_strdup (name);
  return enum_;
}

GISourceType *
gi_source_pointer_new (GISourceType * base_type)
{
  GISourceType *pointer = gi_source_type_new (CTYPE_POINTER);
  if (base_type != NULL)
    pointer->base_type = gi_source_type_copy (base_type);
  return pointer;
}

GISourceType *
gi_source_array_new (GISourceSymbol *size)
{
  GISourceType *array = gi_source_type_new (CTYPE_ARRAY);
  if (size != NULL && size->type == CSYMBOL_TYPE_CONST && size->const_int_set)
      array->child_list = g_list_append (array->child_list, size);
  return array;
}

GISourceType *
gi_source_function_new (void)
{
  GISourceType *func = gi_source_type_new (CTYPE_FUNCTION);
  return func;
}

GISourceScanner *
gi_source_scanner_new (void)
{
  GISourceScanner * scanner;

  scanner = g_slice_new0 (GISourceScanner);
  scanner->typedef_table = g_hash_table_new_full (g_str_hash, g_str_equal,
						  g_free, NULL);
  scanner->struct_or_union_or_enum_table =
    g_hash_table_new_full (g_str_hash, g_str_equal,
			   g_free, (GDestroyNotify)gi_source_symbol_unref);

  return scanner;
}

static void
gi_source_comment_free (GISourceComment *comment)
{
  g_free (comment->comment);
  g_free (comment->filename);
  g_slice_free (GISourceComment, comment);
}

void
gi_source_scanner_free (GISourceScanner *scanner)
{
  g_free (scanner->current_filename);

  g_hash_table_destroy (scanner->typedef_table);
  g_hash_table_destroy (scanner->struct_or_union_or_enum_table);

  g_slist_foreach (scanner->comments, (GFunc)gi_source_comment_free, NULL);
  g_slist_free (scanner->comments);
  g_slist_foreach (scanner->symbols, (GFunc)gi_source_symbol_unref, NULL);
  g_slist_free (scanner->symbols);

  g_list_foreach (scanner->filenames, (GFunc)g_free, NULL);
  g_list_free (scanner->filenames);

}

gboolean
gi_source_scanner_is_typedef (GISourceScanner *scanner,
			      const char      *name)
{
  gboolean b = g_hash_table_lookup (scanner->typedef_table, name) != NULL;
  return b;
}

void
gi_source_scanner_set_macro_scan (GISourceScanner  *scanner,
				  gboolean          macro_scan)
{
  scanner->macro_scan = macro_scan;
}

void
gi_source_scanner_add_symbol (GISourceScanner  *scanner,
			      GISourceSymbol   *symbol)
{
  gboolean found_filename = FALSE;
  GList *l;
  GFile *current_file;

  g_assert (scanner->current_filename);
  current_file = g_file_new_for_path (scanner->current_filename);

  for (l = scanner->filenames; l != NULL; l = l->next)
    {
      GFile *file = g_file_new_for_path (l->data);

      if (g_file_equal (file, current_file))
	{
	  found_filename = TRUE;
          g_object_unref (file);
	  break;
	}
      g_object_unref (file);
    }

  if (found_filename || scanner->macro_scan)
    scanner->symbols = g_slist_prepend (scanner->symbols,
					gi_source_symbol_ref (symbol));
  g_assert (symbol->source_filename != NULL);

  switch (symbol->type)
    {
    case CSYMBOL_TYPE_TYPEDEF:
      g_hash_table_insert (scanner->typedef_table,
			   g_strdup (symbol->ident),
			   GINT_TO_POINTER (TRUE));
      break;
    case CSYMBOL_TYPE_STRUCT:
    case CSYMBOL_TYPE_UNION:
    case CSYMBOL_TYPE_ENUM:
      g_hash_table_insert (scanner->struct_or_union_or_enum_table,
			   g_strdup (symbol->ident),
			   gi_source_symbol_ref (symbol));
      break;
    default:
      break;
    }

    g_object_unref (current_file);
}

GSList *
gi_source_scanner_get_symbols (GISourceScanner  *scanner)
{
  return g_slist_reverse (scanner->symbols);
}

GSList *
gi_source_scanner_get_comments(GISourceScanner  *scanner)
{
  return g_slist_reverse (scanner->comments);
}
