/* GObject introspection: scanner
 *
 * Copyright (C) 2008  Johan Dahlin <johan@gnome.org>
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

#ifdef HAVE_CONFIG_H
#  include "config.h"
#endif
#include <Python.h>
#include "sourcescanner.h"

#ifdef G_OS_WIN32
#define USE_WINDOWS
#endif
#include "grealpath.h"

#ifdef _WIN32
#include <stdint.h>
#include <fcntl.h>
#include <io.h>
#define WIN32_LEAN_AND_MEAN
#define STRICT
#include <windows.h>
#endif

#include <glib-object.h>

DL_EXPORT(void) init_giscanner(void);

#define NEW_CLASS(ctype, name, cname)	      \
static const PyMethodDef _Py##cname##_methods[];    \
PyTypeObject Py##cname##_Type = {             \
    PyObject_HEAD_INIT(NULL)                  \
    0,			                      \
    "scanner." name,                          \
    sizeof(ctype),     	              \
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0,	      \
    0, 0, 0, 0,	0, 0,			      \
    Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE, \
    NULL, 0, 0, 0,	                      \
    0,	      \
    0, 0,                                     \
    0,                                        \
    0, 0, NULL, NULL, 0, 0,	              \
    0             \
}

#define REGISTER_TYPE(d, name, type)	      \
    type.ob_type = &PyType_Type;              \
    type.tp_alloc = PyType_GenericAlloc;      \
    type.tp_new = PyType_GenericNew;          \
    if (PyType_Ready (&type))                 \
	return;                               \
    PyDict_SetItemString (d, name, (PyObject *)&type); \
    Py_INCREF (&type);

typedef struct {
  PyObject_HEAD
  GISourceType *type;
} PyGISourceType;

static PyObject * pygi_source_type_new (GISourceType *type);

typedef struct {
  PyObject_HEAD
  GISourceSymbol *symbol;
} PyGISourceSymbol;

typedef struct {
  PyObject_HEAD
  GISourceScanner *scanner;
} PyGISourceScanner;

NEW_CLASS (PyGISourceSymbol, "SourceSymbol", GISourceSymbol);
NEW_CLASS (PyGISourceType, "SourceType", GISourceType);
NEW_CLASS (PyGISourceScanner, "SourceScanner", GISourceScanner);


/* Symbol */

static PyObject *
pygi_source_symbol_new (GISourceSymbol *symbol)
{
  PyGISourceSymbol *self;

  if (symbol == NULL)
    {
      Py_INCREF (Py_None);
      return Py_None;
    }

  self = (PyGISourceSymbol *)PyObject_New (PyGISourceSymbol,
					   &PyGISourceSymbol_Type);
  self->symbol = symbol;
  return (PyObject*)self;
}

static PyObject *
symbol_get_type (PyGISourceSymbol *self,
		 void             *context)
{
  return PyInt_FromLong (self->symbol->type);
}

static PyObject *
symbol_get_line (PyGISourceSymbol *self,
		 void             *context)
{
  return PyInt_FromLong (self->symbol->line);
}

static PyObject *
symbol_get_private (PyGISourceSymbol *self,
                    void             *context)
{
  return PyBool_FromLong (self->symbol->private);
}

static PyObject *
symbol_get_ident (PyGISourceSymbol *self,
		  void            *context)
{

  if (!self->symbol->ident)
    {
      Py_INCREF(Py_None);
      return Py_None;
    }

  return PyString_FromString (self->symbol->ident);
}

static PyObject *
symbol_get_base_type (PyGISourceSymbol *self,
		      void             *context)
{
  return pygi_source_type_new (self->symbol->base_type);
}

static PyObject *
symbol_get_const_int (PyGISourceSymbol *self,
		      void             *context)
{
  if (!self->symbol->const_int_set)
    {
      Py_INCREF(Py_None);
      return Py_None;
    }

  return PyLong_FromLongLong ((long long)self->symbol->const_int);
}

static PyObject *
symbol_get_const_double (PyGISourceSymbol *self,
                         void             *context)
{
  if (!self->symbol->const_double_set)
    {
      Py_INCREF(Py_None);
      return Py_None;
    }
  return PyFloat_FromDouble (self->symbol->const_double);
}

static PyObject *
symbol_get_const_string (PyGISourceSymbol *self,
			 void             *context)
{
  if (!self->symbol->const_string)
    {
      Py_INCREF(Py_None);
      return Py_None;
    }

  return PyString_FromString (self->symbol->const_string);
}

static PyObject *
symbol_get_source_filename (PyGISourceSymbol *self,
                            void             *context)
{
  if (!self->symbol->source_filename)
    {
      Py_INCREF(Py_None);
      return Py_None;
    }

  return PyString_FromString (self->symbol->source_filename);
}

static const PyGetSetDef _PyGISourceSymbol_getsets[] = {
  /* int ref_count; */
  { "type", (getter)symbol_get_type, NULL, NULL},
  /* int id; */
  { "ident", (getter)symbol_get_ident, NULL, NULL},
  { "base_type", (getter)symbol_get_base_type, NULL, NULL},
  /* gboolean const_int_set; */
  { "const_int", (getter)symbol_get_const_int, NULL, NULL},
  /* gboolean const_double_set; */
  { "const_double", (getter)symbol_get_const_double, NULL, NULL},
  { "const_string", (getter)symbol_get_const_string, NULL, NULL},
  { "source_filename", (getter)symbol_get_source_filename, NULL, NULL},
  { "line", (getter)symbol_get_line, NULL, NULL},
  { "private", (getter)symbol_get_private, NULL, NULL},
  { 0 }
};



/* Type */

static PyObject *
pygi_source_type_new (GISourceType *type)
{
  PyGISourceType *self;

  if (type == NULL)
    {
      Py_INCREF (Py_None);
      return Py_None;
    }

  self = (PyGISourceType *)PyObject_New (PyGISourceType,
					 &PyGISourceType_Type);
  self->type = type;
  return (PyObject*)self;
}

static PyObject *
type_get_type (PyGISourceType *self,
	       void           *context)
{
  return PyInt_FromLong (self->type->type);
}

static PyObject *
type_get_storage_class_specifier (PyGISourceType *self,
				  void           *context)
{
  return PyInt_FromLong (self->type->storage_class_specifier);
}

static PyObject *
type_get_type_qualifier (PyGISourceType *self,
			 void           *context)
{
  return PyInt_FromLong (self->type->type_qualifier);
}

static PyObject *
type_get_function_specifier (PyGISourceType *self,
			     void           *context)
{
  return PyInt_FromLong (self->type->function_specifier);
}

static PyObject *
type_get_name (PyGISourceType *self,
	       void           *context)
{
  if (!self->type->name)
    {
      Py_INCREF (Py_None);
      return Py_None;
    }

  return PyString_FromString (self->type->name);
}

static PyObject *
type_get_base_type (PyGISourceType *self,
		    void           *context)
{
  return pygi_source_type_new (self->type->base_type);
}

static PyObject *
type_get_child_list (PyGISourceType *self,
		     void           *context)
{
  GList *l;
  PyObject *list;
  int i = 0;

  if (!self->type)
    return Py_BuildValue("[]");

  list = PyList_New (g_list_length (self->type->child_list));

  for (l = self->type->child_list; l; l = l->next)
    {
      PyObject *item = pygi_source_symbol_new (l->data);
      PyList_SetItem (list, i++, item);
    }

  Py_INCREF (list);
  return list;
}

static PyObject *
type_get_is_bitfield (PyGISourceType *self,
			     void           *context)
{
  return PyInt_FromLong (self->type->is_bitfield);
}

static const PyGetSetDef _PyGISourceType_getsets[] = {
  { "type", (getter)type_get_type, NULL, NULL},
  { "storage_class_specifier", (getter)type_get_storage_class_specifier, NULL, NULL},
  { "type_qualifier", (getter)type_get_type_qualifier, NULL, NULL},
  { "function_specifier", (getter)type_get_function_specifier, NULL, NULL},
  { "name", (getter)type_get_name, NULL, NULL},
  { "base_type", (getter)type_get_base_type, NULL, NULL},
  { "child_list", (getter)type_get_child_list, NULL, NULL},
  { "is_bitfield", (getter)type_get_is_bitfield, NULL, NULL},
  { 0 }
};



/* Scanner */

static int
pygi_source_scanner_init (PyGISourceScanner *self,
			  PyObject  	    *args,
			  PyObject 	    *kwargs)
{
  if (!PyArg_ParseTuple (args, ":SourceScanner.__init__"))
    return -1;

  self->scanner = gi_source_scanner_new ();

  return 0;
}

static PyObject *
pygi_source_scanner_append_filename (PyGISourceScanner *self,
				     PyObject          *args)
{
  char *filename;

  if (!PyArg_ParseTuple (args, "s:SourceScanner.append_filename", &filename))
    return NULL;

  self->scanner->filenames = g_list_append (self->scanner->filenames,
					    g_realpath (filename));

  Py_INCREF (Py_None);
  return Py_None;
}

static PyObject *
pygi_source_scanner_parse_macros (PyGISourceScanner *self,
                                  PyObject          *args)
{
  GList *filenames;
  int i;
  PyObject *list;

  list = PyTuple_GET_ITEM (args, 0);

  if (!PyList_Check (list))
    {
      PyErr_SetString (PyExc_RuntimeError, "parse macro takes a list of filenames");
      return NULL;
    }

  filenames = NULL;
  for (i = 0; i < PyList_Size (list); ++i)
    {
      PyObject *obj;
      char *filename;

      obj = PyList_GetItem (list, i);
      filename = PyString_AsString (obj);

      filenames = g_list_append (filenames, filename);
    }

  gi_source_scanner_parse_macros (self->scanner, filenames);
  g_list_free (filenames);

  Py_INCREF (Py_None);
  return Py_None;
}

static PyObject *
pygi_source_scanner_parse_file (PyGISourceScanner *self,
				PyObject          *args)
{
  int fd;
  FILE *fp;

  if (!PyArg_ParseTuple (args, "i:SourceScanner.parse_file", &fd))
    return NULL;

#ifdef _WIN32
  /* The file descriptor passed to us is from the C library Python
   * uses. That is msvcr71.dll for Python 2.5 and msvcr90.dll for
   * Python 2.6, 2.7, etc. This code, at least if compiled with mingw, uses
   * msvcrt.dll, so we cannot use the file descriptor directly. So
   * perform appropriate magic.
   */
  {
#if defined(PY_MAJOR_VERSION) && PY_MAJOR_VERSION==2 && PY_MINOR_VERSION==5
#define PYTHON_MSVCRXX_DLL "msvcr71.dll"
#elif defined(PY_MAJOR_VERSION) && PY_MAJOR_VERSION==2 && PY_MINOR_VERSION==6
#define PYTHON_MSVCRXX_DLL "msvcr90.dll"
#elif defined(PY_MAJOR_VERSION) && PY_MAJOR_VERSION==2 && PY_MINOR_VERSION==7
#define PYTHON_MSVCRXX_DLL "msvcr90.dll"
#elif defined(PY_MAJOR_VERSION) && PY_MAJOR_VERSION==3 && PY_MINOR_VERSION==2
#define PYTHON_MSVCRXX_DLL "msvcr90.dll"
#else
#error This Python version not handled
#endif
    HMODULE msvcrxx;
    intptr_t (*p__get_osfhandle) (int);
    HANDLE handle;

    msvcrxx = GetModuleHandle (PYTHON_MSVCRXX_DLL);
    if (!msvcrxx)
      {
        g_print ("No " PYTHON_MSVCRXX_DLL " loaded.\n");
	return NULL;
      }

    p__get_osfhandle = (intptr_t (*) (int)) GetProcAddress (msvcrxx, "_get_osfhandle");
    if (!p__get_osfhandle)
      {
        g_print ("No _get_osfhandle found in " PYTHON_MSVCRXX_DLL ".\n");
	return NULL;
      }

    handle = (HANDLE) p__get_osfhandle (fd);
    if (!p__get_osfhandle)
      {
        g_print ("Could not get OS handle from " PYTHON_MSVCRXX_DLL " fd.\n");
	return NULL;
      }

    fd = _open_osfhandle ((intptr_t) handle, _O_RDONLY);
    if (fd == -1)
      {
	g_print ("Could not open C fd from OS handle.\n");
	return NULL;
      }
  }
#endif

  fp = fdopen (fd, "r");
  if (!fp)
    {
      PyErr_SetFromErrno (PyExc_OSError);
      return NULL;
    }

  if (!gi_source_scanner_parse_file (self->scanner, fp))
    {
      g_print ("Something went wrong during parsing.\n");
      return NULL;
    }

  Py_INCREF (Py_None);
  return Py_None;
}

static PyObject *
pygi_source_scanner_lex_filename (PyGISourceScanner *self,
				  PyObject          *args)
{
  char *filename;

  if (!PyArg_ParseTuple (args, "s:SourceScanner.lex_filename", &filename))
    return NULL;

  self->scanner->current_filename = g_strdup (filename);
  if (!gi_source_scanner_lex_filename (self->scanner, filename))
    {
      g_print ("Something went wrong during lexing.\n");
      return NULL;
    }
  self->scanner->filenames =
    g_list_append (self->scanner->filenames, g_strdup (filename));

  Py_INCREF (Py_None);
  return Py_None;
}

static PyObject *
pygi_source_scanner_set_macro_scan (PyGISourceScanner *self,
				    PyObject          *args)
{
  int macro_scan;

  if (!PyArg_ParseTuple (args, "b:SourceScanner.set_macro_scan", &macro_scan))
    return NULL;

  gi_source_scanner_set_macro_scan (self->scanner, macro_scan);

  Py_INCREF (Py_None);
  return Py_None;
}

static PyObject *
pygi_source_scanner_get_symbols (PyGISourceScanner *self)
{
  GSList *l, *symbols;
  PyObject *list;
  int i = 0;

  symbols = gi_source_scanner_get_symbols (self->scanner);
  list = PyList_New (g_slist_length (symbols));

  for (l = symbols; l; l = l->next)
    {
      PyObject *item = pygi_source_symbol_new (l->data);
      PyList_SetItem (list, i++, item);
    }

  Py_INCREF (list);
  return list;
}

static PyObject *
pygi_source_scanner_get_comments (PyGISourceScanner *self)
{
  GSList *l, *comments;
  PyObject *list;
  int i = 0;

  comments = gi_source_scanner_get_comments (self->scanner);
  list = PyList_New (g_slist_length (comments));

  for (l = comments; l; l = l->next)
    {
      GISourceComment *comment = l->data;
      PyObject *item = Py_BuildValue ("(ssi)", comment->comment,
                                      comment->filename,
                                      comment->line);
      PyList_SetItem (list, i++, item);
    }

  Py_INCREF (list);
  return list;
}

static const PyMethodDef _PyGISourceScanner_methods[] = {
  { "get_comments", (PyCFunction) pygi_source_scanner_get_comments, METH_NOARGS },
  { "get_symbols", (PyCFunction) pygi_source_scanner_get_symbols, METH_NOARGS },
  { "append_filename", (PyCFunction) pygi_source_scanner_append_filename, METH_VARARGS },
  { "parse_file", (PyCFunction) pygi_source_scanner_parse_file, METH_VARARGS },
  { "parse_macros", (PyCFunction) pygi_source_scanner_parse_macros, METH_VARARGS },
  { "lex_filename", (PyCFunction) pygi_source_scanner_lex_filename, METH_VARARGS },
  { "set_macro_scan", (PyCFunction) pygi_source_scanner_set_macro_scan, METH_VARARGS },
  { NULL, NULL, 0 }
};


static int calc_attrs_length(PyObject *attributes, int indent,
			     int self_indent)
{
  int attr_length = 0;
  int i;

  if (indent == -1)
    return -1;

  for (i = 0; i < PyList_Size (attributes); ++i)
    {
      PyObject *tuple, *pyvalue;
      PyObject *s = NULL;
      char *attr, *value;
      char *escaped;

      tuple = PyList_GetItem (attributes, i);
      if (PyTuple_GetItem(tuple, 1) == Py_None)
	continue;

      if (!PyArg_ParseTuple(tuple, "sO", &attr, &pyvalue))
        return -1;

      if (PyUnicode_Check(pyvalue)) {
        s = PyUnicode_AsUTF8String(pyvalue);
        if (!s) {
          return -1;
        }
        value = PyString_AsString(s);
      } else if (PyString_Check(pyvalue)) {
        value = PyString_AsString(pyvalue);
      } else {
        PyErr_SetString(PyExc_TypeError,
                        "value must be string or unicode");
        return -1;
      }

      escaped = g_markup_escape_text (value, -1);
      attr_length += 2 + strlen(attr) + strlen(escaped) + 2;
      g_free(escaped);
      Py_XDECREF(s);
    }

  return attr_length + indent + self_indent;
}

/* Hall of shame, wasted time debugging the code below
 * 20min - Johan 2009-02-19
 */
static PyObject *
pygi_collect_attributes (PyObject *self,
			 PyObject *args)
{
  char *tag_name;
  PyObject *attributes;
  int indent, indent_len, i, j, self_indent;
  char *indent_char;
  gboolean first;
  GString *attr_value = NULL;
  int len;
  PyObject *result = NULL;

  if (!PyArg_ParseTuple(args, "sO!isi",
			&tag_name, &PyList_Type, &attributes,
			&self_indent, &indent_char,
			&indent))
    return NULL;

  if (attributes == Py_None || !PyList_Size(attributes))
    return PyUnicode_DecodeUTF8("", 0, "strict");

  len = calc_attrs_length(attributes, indent, self_indent);
  if (len < 0)
    return NULL;
  if (len > 79)
    indent_len = self_indent + strlen(tag_name) + 1;
  else
    indent_len = 0;

  first = TRUE;
  attr_value = g_string_new ("");

  for (i = 0; i < PyList_Size (attributes); ++i)
    {
      PyObject *tuple, *pyvalue;
      PyObject *s = NULL;
      char *attr, *value, *escaped;

      tuple = PyList_GetItem (attributes, i);

      if (!PyTuple_Check (tuple))
        {
          PyErr_SetString(PyExc_TypeError,
                          "attribute item must be a tuple");
	  goto out;
        }

      if (!PyTuple_Size (tuple) == 2)
        {
          PyErr_SetString(PyExc_IndexError,
                          "attribute item must be a tuple of length 2");
	  goto out;
        }

      if (PyTuple_GetItem(tuple, 1) == Py_None)
	continue;

      /* this leaks, but we exit after, so */
      if (!PyArg_ParseTuple(tuple, "sO", &attr, &pyvalue))
	goto out;

      if (PyUnicode_Check(pyvalue)) {
        s = PyUnicode_AsUTF8String(pyvalue);
        if (!s)
	  goto out;
        value = PyString_AsString(s);
      } else if (PyString_Check(pyvalue)) {
        value = PyString_AsString(pyvalue);
      } else {
        PyErr_SetString(PyExc_TypeError,
                        "value must be string or unicode");
	goto out;
      }

      if (indent_len && !first)
	{
	  g_string_append_c (attr_value, '\n');
	  for (j = 0; j < indent_len; j++)
	    g_string_append_c (attr_value, ' ');
	}
      g_string_append_c (attr_value, ' ');
      g_string_append (attr_value, attr);
      g_string_append_c (attr_value, '=');
      g_string_append_c (attr_value, '\"');
      escaped = g_markup_escape_text (value, -1);
      g_string_append (attr_value, escaped);
      g_string_append_c (attr_value, '\"');
      if (first)
	first = FALSE;
      Py_XDECREF(s);
  }

  result = PyUnicode_DecodeUTF8 (attr_value->str, attr_value->len, "strict");
 out:
  if (attr_value != NULL)
    g_string_free (attr_value, TRUE);
  return result;
}

/* Module */

static const PyMethodDef pyscanner_functions[] = {
  { "collect_attributes",
    (PyCFunction) pygi_collect_attributes, METH_VARARGS },
  { NULL, NULL, 0, NULL }
};

DL_EXPORT(void)
init_giscanner(void)
{
    PyObject *m, *d;
    gboolean is_uninstalled;

    g_type_init ();

    /* Hack to avoid having to create a fake directory structure; when
     * running uninstalled, the module will be in the top builddir,
     * with no _giscanner prefix.
     */
    is_uninstalled = g_getenv ("UNINSTALLED_INTROSPECTION_SRCDIR") != NULL;
    m = Py_InitModule (is_uninstalled ? "_giscanner" : "giscanner._giscanner",
		       (PyMethodDef*)pyscanner_functions);
    d = PyModule_GetDict (m);

    PyGISourceScanner_Type.tp_init = (initproc)pygi_source_scanner_init;
    PyGISourceScanner_Type.tp_methods = (PyMethodDef*)_PyGISourceScanner_methods;
    REGISTER_TYPE (d, "SourceScanner", PyGISourceScanner_Type);

    PyGISourceSymbol_Type.tp_getset = (PyGetSetDef*)_PyGISourceSymbol_getsets;
    REGISTER_TYPE (d, "SourceSymbol", PyGISourceSymbol_Type);

    PyGISourceType_Type.tp_getset = (PyGetSetDef*)_PyGISourceType_getsets;
    REGISTER_TYPE (d, "SourceType", PyGISourceType_Type);
}
