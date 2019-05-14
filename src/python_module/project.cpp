#include "project.hpp"
#include "schematic.hpp"
#include "board.hpp"
#include "pool/pool_manager.hpp"

ProjectWrapper::ProjectWrapper(const std::string &path) : project(horizon::Project::new_from_file(path))
{
}

static PyObject *PyProject_get_name(PyObject *pself)
{
    auto self = reinterpret_cast<PyProject *>(pself);
    return PyUnicode_FromString(self->project->project.name.c_str());
}

static PyObject *PyProject_get_title(PyObject *pself)
{
    auto self = reinterpret_cast<PyProject *>(pself);
    return PyUnicode_FromString(self->project->project.title.c_str());
}

static PyObject *PyProject_open_top_schematic(PyObject *pself)
{
    auto self = reinterpret_cast<PyProject *>(pself);
    if (horizon::PoolManager::get().get_by_uuid(self->project->project.pool_uuid) == nullptr) {
        PyErr_SetString(PyExc_FileNotFoundError, "pool not found");
        return NULL;
    }
    SchematicWrapper *schematic = nullptr;
    try {
        auto top_block = self->project->project.get_top_block();
        schematic = new SchematicWrapper(self->project->project, top_block.uuid);
    }
    catch (const std::exception &e) {
        PyErr_SetString(PyExc_IOError, e.what());
        return NULL;
    }
    catch (...) {
        PyErr_SetString(PyExc_IOError, "unknown exception");
        return NULL;
    }

    PySchematic *sch = PyObject_New(PySchematic, &SchematicType);
    sch->schematic = schematic;
    return reinterpret_cast<PyObject *>(sch);
}

static PyObject *PyProject_open_board(PyObject *pself)
{
    auto self = reinterpret_cast<PyProject *>(pself);
    if (horizon::PoolManager::get().get_by_uuid(self->project->project.pool_uuid) == nullptr) {
        PyErr_SetString(PyExc_FileNotFoundError, "pool not found");
        return NULL;
    }
    BoardWrapper *board = nullptr;
    try {
        board = new BoardWrapper(self->project->project);
    }
    catch (const std::exception &e) {
        PyErr_SetString(PyExc_IOError, e.what());
        return NULL;
    }
    catch (...) {
        PyErr_SetString(PyExc_IOError, "unknown exception");
        return NULL;
    }

    PyBoard *brd = PyObject_New(PyBoard, &BoardType);
    brd->board = board;
    return reinterpret_cast<PyObject *>(brd);
}


static PyObject *PyProject_new(PyTypeObject *type, PyObject *args, PyObject *kwds)
{
    PyProject *self;
    self = (PyProject *)type->tp_alloc(type, 0);
    if (self != NULL) {
        self->project = nullptr;
    }
    return (PyObject *)self;
}

static void PyProject_dealloc(PyObject *pself)
{
    auto self = reinterpret_cast<PyProject *>(pself);
    delete self->project;
    Py_TYPE(self)->tp_free((PyObject *)self);
}

static int PyProject_init(PyObject *pself, PyObject *args, PyObject *kwds)
{
    auto self = reinterpret_cast<PyProject *>(pself);
    const char *path;
    if (!PyArg_ParseTuple(args, "s", &path))
        return -1;
    ProjectWrapper *new_project = nullptr;
    try {
        new_project = new ProjectWrapper(path);
    }
    catch (const std::exception &e) {
        PyErr_SetString(PyExc_IOError, e.what());
        return -1;
    }
    catch (...) {
        PyErr_SetString(PyExc_IOError, "unknown exception");
        return -1;
    }
    delete self->project;
    self->project = new_project;
    return 0;
}

static PyMethodDef PyProject_methods[] = {
        {"get_name", (PyCFunction)PyProject_get_name, METH_NOARGS, "Return project name"},
        {"get_title", (PyCFunction)PyProject_get_title, METH_NOARGS, "Return project title"},
        {"open_top_schematic", (PyCFunction)PyProject_open_top_schematic, METH_NOARGS, "Open top block"},
        {"open_board", (PyCFunction)PyProject_open_board, METH_NOARGS, "Open board"},
        {NULL} /* Sentinel */
};


PyTypeObject ProjectType = {
    PyVarObject_HEAD_INIT( &PyType_Type, 0 )
    "horizon.Project",                          /* tp_name */
    sizeof( PyProject ),                        /* tp_basicsize */
    0,                                          /* tp_itemsize */
    (destructor)PyProject_dealloc,              /* tp_dealloc */
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
    "Project",                                  /* Documentation string */
    (traverseproc)0,                            /* tp_traverse */
    (inquiry)0,                                 /* tp_clear */
    (richcmpfunc)0,                             /* tp_richcompare */
    0,                                          /* tp_weaklistoffset */
    (getiterfunc)0,                             /* tp_iter */
    (iternextfunc)0,                            /* tp_iternext */
    (struct PyMethodDef*)PyProject_methods,     /* tp_methods */
    (struct PyMemberDef*)0,                     /* tp_members */
    0,                                          /* tp_getset */
    0,                                          /* tp_base */
    0,                                          /* tp_dict */
    (descrgetfunc)0,                            /* tp_descr_get */
    (descrsetfunc)0,                            /* tp_descr_set */
    0,                                          /* tp_dictoffset */
    (initproc)0,                                /* tp_init */
    (allocfunc)0,                               /* tp_alloc */
    (newfunc)PyProject_new,                     /* tp_new */
    (freefunc)0,                                /* tp_free */
    (inquiry)0,                                 /* tp_is_gc */
    0,                                          /* tp_bases */
    0,                                          /* tp_mro */
    0,                                          /* tp_cache */
    0,                                          /* tp_subclasses */
    0,                                          /* tp_weaklist */
    (destructor)0                               /* tp_del */
};
