#include <Python.h>
#include "structmember.h"
#include "string.h"             /* for NULL pointers */
#include "pssm_algorithms.h"
// #define NPY_NO_DEPRECATED_API NPY_1_7_API_VERSION
// #include    <numpy/arrayobject.h>

#if PY_MAJOR_VERSION >= 3
/* see http://python3porting.com/cextensions.html */
    #define MOD_INIT(name) PyMODINIT_FUNC PyInit_##name(void)
#else
    #define MOD_INIT(name) PyMODINIT_FUNC init##name(void)
#endif

/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~ C API functions ~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

PyDoc_STRVAR(moody__doc__,
              "Do Position Weight Matrix stuff\n");

charArray convertSequence(const char *sequence) {
    char_vec_t c_seq;
    int lenght = strlen(sequence);
    for(int i = 0; i < lenght; i++) {
        char toput = 5;
        switch (sequence[i])
        {
            case 'a':
            case 'A': toput = 0; break;
            case 'c':
            case 'C': toput = 1; break;
            case 'g':
            case 'G': toput = 2; break;
            case 't':
            case 'T': toput = 3; break;
            case 'n':
            case 'N': toput = (int) (4 * rand() / (1.0 + RAND_MAX)); break;
            default:
                break;
        }
        if(toput != 5) {
            kv_push(uint8_t, c_seq, toput); 
        }
    }
    return c_seq;
}

int atoDoubleArray(PyObject *o, score_vec_t *out) {
    if(!PyList_Check(o)) {
        return -1;
    }

    Py_ssize_t length = PyList_Size(o);
    score_vec_t ret;
    kv_init(ret);
    kv_resize(score_t, ret, length);
    PyObject *tmp;
    int i;
    for(i=0; i < length;) {
        tmp = PyList_GET_ITEM(o, i++);
        kv_push(ret, PyFloat_AsDouble(tmp));
    }
    *out = ret;
    return 0;
}

int atoDoubleMatrix(PyObject *o, score_matrix_t *out) {
    if(!PyList_Check(o)) {
        return -1;
    }
    Py_ssize_t length = PyList_Size(o);
    score_matrix_vec_t ret;
    kv_init(ret);
    kv_resize(score_vec_t, ret, length);
    
    PyObject *tmp1;
    score_vec_t tmp2;
    for(int i=0; i < length;) {
        tmp1 = PyList_GET_ITEM(o, i++);
        atoDoubleArray(tmp1, tmp2);
        kv_push(ret, tmp2);
    }
    *out = ret;
    return 0;
}

typedef struct {
    PyObject_HEAD
    moods_mlf_t *mlf;
    // int q; 
    // std::vector<scoreMatrix> *matrices;
    // std::vector<std::vector< OutputListElementMulti> > *output; 
    // intArray *window_positions;
    // intArray *m; 
    // intMatrix *orders; 
    // scoreMatrix *L;
    // scoreArray *thresholds;
    int both_strands;
    int num_matrices;
} MOODSSearch;

static void
MOODSSearch_dealloc(MOODSSearch* self) {
    delete self->matrices;
    delete self->output;
    delete self->window_positions;
    delete self->m;
    delete self->orders;
    delete self->L;
    delete self->thresholds;
    free(self->mlf);
    Py_TYPE(self)->tp_free((PyObject*)self);
}

static PyObject *
MOODSSearch_new(PyTypeObject *type, PyObject *args, PyObject *kwds) {
    MOODSSearch *self;
    self = (MOODSSearch *)type->tp_alloc(type, 0);
    self->mlf = NULL;
    self->mlf = (moods_mlf_t *) calloc(1, sizeof(moods_mlf_t));
    /* allocate other fields later.
    */
    return (PyObject *)self;
}

static int
MOODSSearch_init(MOODSSearch *self, PyObject *args, PyObject *kwds) {
    PyObject *py_matrices;
    PyObject *py_thresholds;
    int q;
    PyObject *py_absolute_threshold;
    PyObject *py_bg;
    PyObject *py_both_strands;
    int absolute_threshold;
    double ps = 0.1;
    int both_strands;

    moods_mlf_t *mlf = self->mlf;

    score_matrix_vec_t *matrices = self->matrices;

    if (self == NULL) {
        return -1;
    }

    if (!PyArg_ParseTuple(args, "OOOiOO", &py_matrices, &py_thresholds, &py_bg, &q, &py_absolute_threshold, &py_both_strands)) {
        return -1;
    }

    absolute_threshold = PyObject_IsTrue(py_absolute_threshold);
    mlf->thresholds = atoDoubleArray(py_thresholds);
    scoreArray &thresholds = *(self->thresholds);  // Hopefully this works
    
    self->both_strands = PyObject_IsTrue(py_both_strands);

    scoreArray bg = *(atoDoubleArray(py_bg));

    if(!PyList_Check(py_matrices)) {
        return -1;
    }
    int num_matrices = (int) PyList_Size(py_matrices);
    if(num_matrices != thresholds.size()) {
        PyErr_SetString(PyExc_RuntimeError, "Thresholds should be as many as matrices");
        return -1;
    }
    self->num_matrices = num_matrices;

    for(int i=0; i < num_matrices; i++) {
        matrices.push_back(atoDoubleMatrix(PyList_GET_ITEM(py_matrices, i)));
        if(matrices[i].size() != 4) {
            PyErr_SetString(PyExc_RuntimeError, "Matrix size must be 4");
            return -1;
        }
    }

    //Check if parameter parsing has raised an exception
    if(PyErr_Occurred()) {
        return -1;
    }

    if(both_strands) {
        for(int i=0; i < num_matrices; i++) {
            matrices.push_back(reverseComplement(matrices[i]));
            thresholds.push_back(thresholds[i]);
        }
    }
    if(!absolute_threshold) {
        for(int i=0; i < matrices.size(); i++) {
            matrices[i] = counts2LogOdds(matrices[i], bg, ps);
            thresholds[i] = tresholdFromP(matrices[i], bg, thresholds[i]);
        }
    }

    const int BITSHIFT = 2;
    const bits_t size = 1 << (BITSHIFT * q); // numA^q
    self->output = new std::vector<std::vector< OutputListElementMulti> >;
    self->output->reserve(size);
    self->window_positions = new intArray;
    self->window_positions->reserve(matrices.size());
    self->m = new intArray(matrices.size(), 0);
    self->orders = new intMatrix;
    self->orders->reserve(matrices.size());
    self->L = new scoreMatrix;
    self->L->reserve(matrices.size());
    if (multipleMatrixLookaheadFiltrationDNASetup(q, 
        self->matrices, self->output, self->window_positions, 
        self->m, self->orders, self->L,
        bg, self->thresholds) < 0) {
        return -1;
    };

    self->q = q;

    return 0;
};

static PyObject *
MOODSSearch_search(MOODSSearch* self, PyObject *args) {
    const char *sequence;
    std::vector<matchArray> matches;
    charArray c_seq;
    if (!PyArg_ParseTuple(args, "s", &sequence)) {
        return NULL;
    }
    c_seq = convertSequence(sequence);
    matches = doScan(c_seq, self->q, self->matrices, self->output, 
        self->window_positions, self->m, 
        self->orders, self->L, self->thresholds);

    int num_matrices = self->num_matrices;
    if(self->both_strands) {
        if(matches.size() != 2 * num_matrices) {
            PyErr_SetString(PyExc_RuntimeError, "Unknown error");
            return NULL;
        }
        for(int i=0; i< num_matrices; i++) {
            while(!matches[num_matrices + i].empty()) {
                matches[num_matrices + i].back().position = -matches[num_matrices + i].back().position;
                matches[i].push_back(matches[num_matrices + i].back());
                matches[num_matrices + i].pop_back();
            }
        }
    }
    PyObject *results = PyList_New(matches.size());
    for(int i = 0; i < matches.size(); i++) {
        PyObject *new_match_list = PyList_New(matches[i].size());
        for(int j=0; j < matches[i].size(); j++) {
            PyList_SET_ITEM(new_match_list, j, Py_BuildValue("Ld", matches[i][j].position, matches[i][j].score));
        }
        PyList_SET_ITEM(results, i, new_match_list);
    }

    return results;
}

static PyMemberDef MOODSSearch_members[] = {
    {NULL}  /* Sentinel */
};

static PyGetSetDef MOODSSearch_getsetters[] = {
    {NULL}  /* Sentinel */
};

static PyMethodDef MOODSSearch_methods[] = {
    {"search", (PyCFunction)MOODSSearch_search, METH_VARARGS,
     "do a Lookahead Filtration Search\n"
    },
    {NULL}  /* Sentinel */
};

static PyTypeObject MOODSSearchType = {
    PyVarObject_HEAD_INIT(NULL, 0)
    "moody.MOODSSearch",        /*tp_name*/
    sizeof(MOODSSearch),            /*tp_basicsize*/
    0,                              /*tp_itemsize*/
    (destructor)MOODSSearch_dealloc,/*tp_dealloc*/
    0,                              /*tp_print*/
    0,                              /*tp_getattr*/
    0,                              /*tp_setattr*/
    0,                              /*tp_compare*/
    0,                              /*tp_repr*/
    0,                              /*tp_as_number*/
    0,                              /*tp_as_sequence*/
    0,                              /*tp_as_mapping*/
    0,                              /*tp_hash */
    0,                              /*tp_call*/
    0,                              /*tp_str*/
    0,                              /*tp_getattro*/
    0,                              /*tp_setattro*/
    0,                              /*tp_as_buffer*/
    Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE, /*tp_flags*/
    "MOODSSearch objects",          /* tp_doc */
    0,                              /* tp_traverse */
    0,                              /* tp_clear */
    0,                              /* tp_richcompare */
    0,                              /* tp_weaklistoffset */
    0,                              /* tp_iter */
    0,                              /* tp_iternext */
    MOODSSearch_methods,            /* tp_methods */
    MOODSSearch_members,            /* tp_members */
    MOODSSearch_getsetters,          /* tp_getset */
    0,                              /* tp_base */
    0,                              /* tp_dict */
    0,                              /* tp_descr_get */
    0,                              /* tp_descr_set */
    0,                              /* tp_dictoffset */
    (initproc)MOODSSearch_init,     /* tp_init */
    0,                              /* tp_alloc */
    MOODSSearch_new,                /* tp_new */
};

static PyMethodDef moody_mod_methods[] = {
    {NULL}
};

MOD_INIT(moody) {
    if (PyType_Ready(&MOODSSearchType) < 0) {
        return NULL;
    }
    #if PY_MAJOR_VERSION >= 3
        static struct PyModuleDef moduledef = {
            PyModuleDef_HEAD_INIT,
            "moody",                /* m_name */
            moody__doc__,           /* m_doc */
            -1,                     /* m_size */
            moody_mod_methods,      /* m_methods */
            NULL,                   /* m_reload */
            NULL,                   /* m_traverse */
            NULL,                   /* m_clear */
            NULL,                   /* m_free */
        };
        PyObject* m = PyModule_Create(&moduledef);
        // import_array();
        if (m == NULL) { return NULL; }

        Py_INCREF(&MOODSSearchType);
        PyModule_AddObject(m, "MOODSSearch", (PyObject *)&MOODSSearchType);
        return m;
    #else
        PyObject* m = Py_InitModule3("moody", moody_methods, moody__doc__);
        if (m == NULL) { return; }
        // import_array();
        Py_INCREF(&MOODSSearchType);
        PyModule_AddObject(m, "MOODSSearch", (PyObject *)&MOODSSearchType);
    #endif
};