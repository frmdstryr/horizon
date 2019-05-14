#include "schematic.hpp"
#include "project/project.hpp"
#include "pool/pool_manager.hpp"
#include "block/block.hpp"
#include "schematic/schematic.hpp"
#include "pool/pool_cached.hpp"
#include "nlohmann/json.hpp"
#include "export_pdf/export_pdf.hpp"
#include "export_bom/export_bom.hpp"
#include "util.hpp"
#include <podofo/podofo.h>

SchematicWrapper::SchematicWrapper(const horizon::Project &prj, const horizon::UUID &block_uuid)
    : pool(horizon::PoolManager::get().get_by_uuid(prj.pool_uuid)->base_path, prj.pool_cache_directory),
      block(horizon::Block::new_from_file(prj.blocks.at(block_uuid).block_filename, pool)),
      schematic(horizon::Schematic::new_from_file(prj.blocks.at(block_uuid).schematic_filename, block, pool))
{
    schematic.expand();
}


static PyObject *PySchematic_new(PyTypeObject *type, PyObject *args, PyObject *kwds)
{
    PySchematic *self;
    self = (PySchematic *)type->tp_alloc(type, 0);
    if (self != NULL) {
        self->schematic = nullptr;
    }
    return (PyObject *)self;
}

static void PySchematic_dealloc(PyObject *pself)
{
    auto self = reinterpret_cast<PySchematic *>(pself);
    delete self->schematic;
    Py_TYPE(self)->tp_free((PyObject *)self);
}


static PyObject *PySchematic_get_pdf_export_settings(PyObject *pself)
{
    auto self = reinterpret_cast<PySchematic *>(pself);
    auto settings = self->schematic->schematic.pdf_export_settings.serialize();
    return py_from_json(settings);
}

static PyObject *PySchematic_export_pdf(PyObject *pself, PyObject *args)
{
    auto self = reinterpret_cast<PySchematic *>(pself);
    PyObject *py_export_settings = nullptr;
    if (!PyArg_ParseTuple(args, "O!", &PyDict_Type, &py_export_settings))
        return NULL;
    try {
        auto settings_json = json_from_py(py_export_settings);
        horizon::PDFExportSettings settings(settings_json);
        horizon::export_pdf(self->schematic->schematic, settings, nullptr);
    }
    catch (const std::exception &e) {
        PyErr_SetString(PyExc_IOError, e.what());
        return NULL;
    }
    catch (const PoDoFo::PdfError &e) {
        PyErr_SetString(PyExc_IOError, e.what());
        return NULL;
    }
    catch (...) {
        PyErr_SetString(PyExc_IOError, "unknown exception");
        return NULL;
    }
    Py_RETURN_NONE;
}


static PyObject *PySchematic_get_bom_export_settings(PyObject *pself)
{
    auto self = reinterpret_cast<PySchematic *>(pself);
    auto settings = self->schematic->block.bom_export_settings.serialize();
    return py_from_json(settings);
}

static PyObject *PySchematic_export_bom(PyObject *pself, PyObject *args)
{
    auto self = reinterpret_cast<PySchematic *>(pself);
    PyObject *py_export_settings = nullptr;
    if (!PyArg_ParseTuple(args, "O!", &PyDict_Type, &py_export_settings))
        return NULL;
    try {
        auto settings_json = json_from_py(py_export_settings);
        horizon::BOMExportSettings settings(settings_json);
        horizon::export_BOM(settings.output_filename, self->schematic->block, settings);
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


static PyMethodDef PySchematic_methods[] = {
        {"get_pdf_export_settings", (PyCFunction)PySchematic_get_pdf_export_settings, METH_NOARGS,
         "Return pdf export settings"},
        {"get_bom_export_settings", (PyCFunction)PySchematic_get_bom_export_settings, METH_NOARGS,
         "Return bom export settings"},
        {"export_pdf", (PyCFunction)PySchematic_export_pdf, METH_VARARGS, "Export as pdf"},
        {"export_bom", (PyCFunction)PySchematic_export_bom, METH_VARARGS, "Export BOM"},
        {NULL} /* Sentinel */
};

PyTypeObject SchematicType = {
    PyVarObject_HEAD_INIT( &PyType_Type, 0 )
    "horizon.Schematic",                        /* tp_name */
    sizeof( PySchematic ),                      /* tp_basicsize */
    0,                                          /* tp_itemsize */
    (destructor)PySchematic_dealloc,            /* tp_dealloc */
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
    "Schematic",                                /* Documentation string */
    (traverseproc)0,                            /* tp_traverse */
    (inquiry)0,                                 /* tp_clear */
    (richcmpfunc)0,                             /* tp_richcompare */
    0,                                          /* tp_weaklistoffset */
    (getiterfunc)0,                             /* tp_iter */
    (iternextfunc)0,                            /* tp_iternext */
    (struct PyMethodDef*)PySchematic_methods,   /* tp_methods */
    (struct PyMemberDef*)0,                     /* tp_members */
    0,                                          /* tp_getset */
    0,                                          /* tp_base */
    0,                                          /* tp_dict */
    (descrgetfunc)0,                            /* tp_descr_get */
    (descrsetfunc)0,                            /* tp_descr_set */
    0,                                          /* tp_dictoffset */
    (initproc)0,                                /* tp_init */
    (allocfunc)0,                               /* tp_alloc */
    (newfunc)PySchematic_new,                   /* tp_new */
    (freefunc)0,                                /* tp_free */
    (inquiry)0,                                 /* tp_is_gc */
    0,                                          /* tp_bases */
    0,                                          /* tp_mro */
    0,                                          /* tp_cache */
    0,                                          /* tp_subclasses */
    0,                                          /* tp_weaklist */
    (destructor)0                               /* tp_del */
};
