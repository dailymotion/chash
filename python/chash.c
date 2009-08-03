#include <Python.h>
#include "libchash.h"

/*** Types ***/

typedef struct
{
  PyObject_HEAD
  CHASH_CONTEXT context;
} CHashObject;

static PyObject *CHashError;

static PyObject *
chash_return(int status, int zero_is_none)
{
  if (status == 0 && zero_is_none)
    {
      Py_INCREF(Py_None);
      return  Py_None;
    }
  
  if (status >= 0)
    return Py_BuildValue("l", status);

  switch (status)
    {
    case CHASH_ERROR_MEMORY:
      PyErr_NoMemory();
      break;
      
    case CHASH_ERROR_NOT_FOUND:
      PyErr_SetString(CHashError, "No element found");
      break;
      
    default:
      PyErr_SetString(CHashError, "Unknown exception");
    }
    return NULL;
}

//----------------------------------------------------------------------------------------                                                                                                                                       
//                                                                                                                                                                                                                               
static void
chash_dealloc(CHashObject* self)
{
  chash_terminate(&(self->context), 0);
}

//----------------------------------------------------------------------------------------                                                                                                                                       
//                                                                                                                                                                                                                               
static void
chash_init(CHashObject* self, PyObject* args, PyObject* kwds)
{
  chash_initialize(&(self->context), 0);
}

//----------------------------------------------------------------------------------------                                                                                                                                       
//                                                                                                                                                                                                                               
static PyObject *
do_add_target(PyObject *pyself, PyObject *args)
{
  const char*	target;
  CHashObject*	self = (CHashObject*)pyself;
  u_int32_t	weight = 1;

  if (!PyArg_ParseTuple(args, "s|l", &target, &weight))
    return NULL;

  return chash_return(chash_add_target(&(self->context), target, weight), 1);
}

//----------------------------------------------------------------------------------------                                                                                                                                       
//                                                                                                                                                                                                                               
static PyObject *
do_remove_target(PyObject *pyself, PyObject *args)
{
  const char*	target;
  CHashObject*	self = (CHashObject*)pyself;

  if (!PyArg_ParseTuple(args, "s", &target))
    return NULL;

  return  chash_return(chash_remove_target(&(self->context), target), 1);
}

//----------------------------------------------------------------------------------------                                                                                                                                       
//                                                                                                                                                                                                                               
static PyObject *
do_count_targets(PyObject *pyself, PyObject *args)
{
  CHashObject* self = (CHashObject*)pyself;

  return chash_return(chash_targets_count(&(self->context)), 0);
}

//----------------------------------------------------------------------------------------                                                                                                                                       
//                                                                                                                                                                                                                               
static PyObject *
do_clear_targets(PyObject *pyself, PyObject *args)
{
  CHashObject* self = (CHashObject*)pyself;

  return chash_return(chash_clear_targets(&(self->context)), 1);
}

//----------------------------------------------------------------------------------------                                                                                                                                       
//                                                                                                                                                                                                                               
static PyObject *
do_serialize(PyObject *pyself, PyObject *args)
{
  CHashObject* self = (CHashObject*)pyself;
  u_char*      serialized;
  int          size;
  PyObject*    retval;

  size = chash_serialize(&(self->context), &serialized);

  if (size < 0)
  {
    return chash_return(size, 1);
  } 

  retval = PyString_FromStringAndSize((char *)serialized, size);

  if (serialized)
    free(serialized);

  return retval;
}

//----------------------------------------------------------------------------------------                                                                                                                                       
//                                                                                                                                                                                                                               
static PyObject *
do_unserialize(PyObject *pyself, PyObject *args)
{
  CHashObject* self = (CHashObject*)pyself;
  u_char*      serialized;
  u_int32_t    length;

  if (!PyArg_ParseTuple(args, "s#", &serialized, &length))
    return NULL;

  return chash_return(chash_unserialize(&(self->context), serialized, length), 1);
}

//----------------------------------------------------------------------------------------                                                                                                                                       
//                                                                                                                                                                                                                               
static PyObject *
do_serialize_to_file(PyObject *pyself, PyObject *args)
{
  CHashObject* self = (CHashObject*)pyself;
  char*        path;

  if (!PyArg_ParseTuple(args, "s", &path))
    return NULL;

  return chash_return(chash_file_serialize(&(self->context), path), 1);
}

//----------------------------------------------------------------------------------------
//
static PyObject *
do_unserialize_from_file(PyObject *pyself, PyObject *args)
{
  CHashObject* self = (CHashObject*)pyself;
  char*        path;

  if (!PyArg_ParseTuple(args, "s", &path))
    return NULL;

  return chash_return(chash_file_unserialize(&(self->context), path), 1);
}

//----------------------------------------------------------------------------------------
//
static PyObject *
do_lookup_list(PyObject *pyself, PyObject *args)
{
  CHashObject* self = (CHashObject*)pyself;
  char*        candidate;
  uint         count = 1;
  int          status;
  uint         index;
  char**       targets;
  PyObject*    retval = PyList_New(0);

  if (!PyArg_ParseTuple(args, "s|l", &candidate, &count))
    return NULL;

  status = chash_lookup(&(self->context), candidate, count, &targets);
  if (status <= 0)
    return chash_return(status, 1);
  
  for (index = 0; index < status; index ++)
  {
    PyList_Append(retval, PyString_FromString(targets[index]));
  }

  return retval;
}

//----------------------------------------------------------------------------------------
//
static PyObject *
do_lookup_balance(PyObject *pyself, PyObject *args)
{
  CHashObject* self = (CHashObject*)pyself;
  char*        candidate;
  uint         count = 1;
  int          status;
  char*        target;

  if (!PyArg_ParseTuple(args, "s|l", &candidate, &count))
    return NULL;
  
  status = chash_lookup_balance(&(self->context), candidate, count, &target);
  if (status < 0)
    return chash_return(status, 1);

  return PyString_FromString(target);
}

//----------------------------------------------------------------------------------------
//
static PyMethodDef chash_methods[] = {
    {
      "add_target",  do_add_target, METH_VARARGS, 
      "add_target(target, weight=1) -- FIXME"
    },
    {
      "remove_target",  do_remove_target, METH_VARARGS, 
      "remove_target(target)"
    },
    {
      "count_targets", do_count_targets, METH_NOARGS,
      "count_targets()"
      "@return: Number of targets.\n@rtype: int\n"
    },
    {
      "clear_targets", do_clear_targets, METH_NOARGS,
      "clear_targets()"
    },
    {
      "serialize", do_serialize, METH_NOARGS,
      "serialize()"
    },
    {
      "unserialize", do_unserialize, METH_VARARGS,
      "unserialize()"
    },
    {
      "serialize_to_file", do_serialize_to_file, METH_VARARGS,
      "serialize_to_file(path)"
    },
    {
      "unserialize_from_file", do_unserialize_from_file, METH_VARARGS,
      "unserialize_from_file(path)"
    },
    {
      "lookup_list", do_lookup_list, METH_VARARGS,
      "lookup_list(candidate, count=1)"
      "@return: List of targets.\n@rtype: list\n"
    },
    {
      "lookup_balance", do_lookup_balance, METH_VARARGS,
      "lookup_balance(name, count=1)"
      "@return: A target.\n@rtype: string\n"
    },
    {NULL, NULL, 0, NULL}        /* Sentinel */
};

static PyTypeObject chash_CHashType = {
  PyObject_HEAD_INIT(NULL)
  0,                         /*ob_size*/
  "CHash",                   /*tp_name*/
  sizeof(CHashObject),       /*tp_basicsize*/
  0,                         /*tp_itemsize*/
  (destructor)chash_dealloc, /*tp_dealloc*/
  0,                         /*tp_print*/
  0,                         /*tp_getattr*/
  0,                         /*tp_setattr*/
  0,                         /*tp_compare*/
  0,                         /*tp_repr*/
  0,                         /*tp_as_number*/
  0,                         /*tp_as_sequence*/
  0,                         /*tp_as_mapping*/
  0,                         /*tp_hash */
  0,                         /*tp_call*/
  0,                         /*tp_str*/
  0,                         /*tp_getattro*/
  0,                         /*tp_setattro*/
  0,                         /*tp_as_buffer*/
  Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE,        /*tp_flags*/
  "CHash object",            /* tp_doc */
  0,                         /* tp_traverse */
  0,                         /* tp_clear */
  0,                         /* tp_richcompare */
  0,                         /* tp_weaklistoffset */
  0,                         /* tp_iter */
  0,                         /* tp_iternext */
  chash_methods,             /* tp_methods */
  0,                         /* tp_members */
  0,                         /* tp_getset */
  0,                         /* tp_base */
  0,                         /* tp_dict */
  0,                         /* tp_descr_get */
  0,                         /* tp_descr_set */
  0,                         /* tp_dictoffset */
  (initproc)chash_init,      /* tp_init */
  0,                         /* tp_alloc */
  PyType_GenericNew,         /* tp_new */
};

static PyMethodDef chash_module_methods[] = {
  {NULL}  /* Sentinel */
};

#ifndef PyMODINIT_FUNC  /* declarations for DLL import/export */
#define PyMODINIT_FUNC void
#endif
PyMODINIT_FUNC
initchash(void)
{
  PyObject *m;

  chash_CHashType.tp_new = PyType_GenericNew;
  if (PyType_Ready(&chash_CHashType) < 0)
    return;

  m = Py_InitModule3("chash", chash_module_methods,
		     "Extension to chash using libchash.");
  if (m == NULL)
    return;

  CHashError = PyErr_NewException("chash.CHashError", NULL, NULL);
  Py_INCREF(CHashError);
  PyModule_AddObject(m, "CHashError", CHashError);

  Py_INCREF(&chash_CHashType);
  PyModule_AddObject(m, "CHash", (PyObject *)&chash_CHashType);
}
