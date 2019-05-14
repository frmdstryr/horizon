#include "board.hpp"
#include "pool/pool_manager.hpp"
#include "nlohmann/json.hpp"
#include "util.hpp"
#include "export_gerber/gerber_export.hpp"

BoardWrapper::BoardWrapper(const horizon::Project &prj)
    : pool(horizon::PoolManager::get().get_by_uuid(prj.pool_uuid)->base_path, prj.pool_cache_directory),
      block(horizon::Block::new_from_file(prj.get_top_block().block_filename, pool)), vpp(prj.vias_directory, pool),
      board(horizon::Board::new_from_file(prj.board_filename, block, pool, vpp))
{
    board.expand();
    board.update_planes();
}


static PyObject *PyBoard_new(PyTypeObject *type, PyObject *args, PyObject *kwds)
{
    PyBoard *self;
    self = (PyBoard *)type->tp_alloc(type, 0);
    if (self != NULL) {
        self->board = nullptr;
    }
    return (PyObject *)self;
}

static void PyBoard_dealloc(PyObject *pself)
{
    auto self = reinterpret_cast<PyBoard *>(pself);
    delete self->board;
    Py_TYPE(self)->tp_free((PyObject *)self);
}


static PyObject *PyBoard_get_gerber_export_settings(PyObject *pself)
{
    auto self = reinterpret_cast<PyBoard *>(pself);
    auto settings = self->board->board.fab_output_settings.serialize();
    return py_from_json(settings);
}

static PyObject *PyBoard_export_gerber(PyObject *pself, PyObject *args)
{
    auto self = reinterpret_cast<PyBoard *>(pself);
    PyObject *py_export_settings = nullptr;
    if (!PyArg_ParseTuple(args, "O!", &PyDict_Type, &py_export_settings))
        return NULL;
    try {
        auto settings_json = json_from_py(py_export_settings);
        horizon::FabOutputSettings settings(settings_json);
        horizon::GerberExporter ex(&self->board->board, &settings);
        ex.generate();
    }
    catch (const std::exception &e) {
        PyErr_SetString(PyExc_IOError, e.what());
        return NULL;
    }
    catch (...) {
        PyErr_SetString(PyExc_IOError, "unknown exception");
        return NULL;
    }
    Py_RETURN_NONE;
}


static PyMethodDef PyBoard_methods[] = {
        {"get_gerber_export_settings", (PyCFunction)PyBoard_get_gerber_export_settings, METH_NOARGS,
         "Return gerber export settings"},
        {"export_gerber", (PyCFunction)PyBoard_export_gerber, METH_VARARGS, "Export gerber"},
        {NULL} /* Sentinel */
};

PyTypeObject BoardType = {
    PyVarObject_HEAD_INIT( &PyType_Type, 0 )
    "horizon.Board",                            /* tp_name */
    sizeof( PyBoard ),                          /* tp_basicsize */
    0,                                          /* tp_itemsize */
    (destructor)PyBoard_dealloc,                /* tp_dealloc */
    (printfunc)0,                               /* tp_print */
    (getattrfunc)0,                             /* tp_getattr */
    (setattrfunc)0,                             /* tp_setattr */
#if PY_VERSION_HEX >= 0x03050000
	( PyAsyncMethods* )0,                       /* tp_as_async */
#elif PY_VERSION_HEX >= 0x03000000
	( void* ) 0,                                /* tp_reserved */
#else
	( cmpfunc )0,                               /* tp_compare */
#endif
    (reprfunc)0,                                /* tp_repr */
    (PyNumberMethods*)0,                        /* tp_as_number */
    (PySequenceMethods*)0,                      /* tp_as_sequence */
    (PyMappingMethods*)0,                       /* tp_as_mapping */
    (hashfunc)0,                                /* tp_hash */
    (ternaryfunc)0,                             /* tp_call */
    (reprfunc)0,                                /* tp_str */
    (getattrofunc)0,                            /* tp_getattro */
    (setattrofunc)0,                            /* tp_setattro */
    (PyBufferProcs*)0,                          /* tp_as_buffer */
    Py_TPFLAGS_DEFAULT,                         /* tp_flags */
    "Board",                                    /* Documentation string */
    (traverseproc)0,                            /* tp_traverse */
    (inquiry)0,                                 /* tp_clear */
    (richcmpfunc)0,                             /* tp_richcompare */
    0,                                          /* tp_weaklistoffset */
    (getiterfunc)0,                             /* tp_iter */
    (iternextfunc)0,                            /* tp_iternext */
    (struct PyMethodDef*)PyBoard_methods,       /* tp_methods */
    (struct PyMemberDef*)0,                     /* tp_members */
    0,                                          /* tp_getset */
    0,                                          /* tp_base */
    0,                                          /* tp_dict */
    (descrgetfunc)0,                            /* tp_descr_get */
    (descrsetfunc)0,                            /* tp_descr_set */
    0,                                          /* tp_dictoffset */
    (initproc)0,                                /* tp_init */
    (allocfunc)0,                               /* tp_alloc */
    (newfunc)PyBoard_new,                       /* tp_new */
    (freefunc)0,                                /* tp_free */
    (inquiry)0,                                 /* tp_is_gc */
    0,                                          /* tp_bases */
    0,                                          /* tp_mro */
    0,                                          /* tp_cache */
    0,                                          /* tp_subclasses */
    0,                                          /* tp_weaklist */
    (destructor)0                               /* tp_del */
};

