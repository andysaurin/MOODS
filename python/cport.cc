// MOODS.xs - algorithm for finding PWM matches from sequences
// Copyright (C) 2009 Petri Martinmäki
//
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version, or under the terms of the Biopython
// License.

#include <Python.h>
#include "pssm_algorithms.hpp"

#if PY_MAJOR_VERSION >= 3
/* see http://python3porting.com/cextensions.html */
    #define MOD_INIT(name) PyMODINIT_FUNC PyInit_##name(void)
#else
    #define MOD_INIT(name) PyMODINIT_FUNC init##name(void)
#endif

charArray convertSequence(const char *sequence) {
	charArray c_seq;
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
			c_seq.push_back(toput);
		}
	}
	return c_seq;
}



PyObject *atoPyArray(scoreArray a) {
	PyObject *new_row = PyList_New(a.size());
	for(unsigned int i=0; i < a.size(); i++) {
		PyList_SET_ITEM(new_row, i, PyFloat_FromDouble(a[i]));
	}
	return new_row;
}

PyObject *atoPyMatrix(scoreMatrix m) {
	PyObject *py_matrix = PyList_New(m.size());
	for(unsigned int i = 0; i < m.size(); i++) {
		PyList_SET_ITEM(py_matrix, i, atoPyArray(m[i]));
	}
	return py_matrix;
}

scoreArray atoDoubleArray(PyObject *o) {
	scoreArray t;
	if(!PyList_Check(o)) {
		return t;
	}
	Py_ssize_t length = PyList_Size(o);
	PyObject *tmp;
	for(int i=0; i< length; i++) {
		tmp = PyList_GET_ITEM(o, i);
		t.push_back(PyFloat_AsDouble(tmp));
	}
	return t;
}

scoreMatrix atoDoubleMatrix(PyObject *o) {
	scoreMatrix t;
	if(!PyList_Check(o)) {
		return t;
	}
	Py_ssize_t length = PyList_Size(o);
	PyObject *tmp;
	for(int i=0; i< length; i++) {
		tmp = PyList_GET_ITEM(o, i);
		t.push_back(atoDoubleArray(tmp));
	}
	return t;
}

static PyObject *_countLogOdds(PyObject *self, PyObject *args)
{
	PyObject *py_matrix;
	PyObject *py_bg;
	double ps;

	if (!PyArg_ParseTuple(args, "OOd", &py_matrix, &py_bg, &ps))
	    return NULL;

	scoreMatrix m = atoDoubleMatrix(py_matrix);
	scoreArray bg = atoDoubleArray(py_bg);
	if(PyErr_Occurred()) {
		return NULL;
	}
	return atoPyMatrix(counts2LogOdds(m, bg, ps));
}

static PyObject *_thresholdFromP(PyObject *self, PyObject *args) {
	PyObject *py_matrix;
	PyObject *py_bg;
	double p;

	if (!PyArg_ParseTuple(args, "OOd", &py_matrix, &py_bg, &p))
		return NULL;

	scoreMatrix m = atoDoubleMatrix(py_matrix);
	scoreArray bg = atoDoubleArray(py_bg);
	if(PyErr_Occurred()) {
		return NULL;
	}
	return PyFloat_FromDouble(tresholdFromP(m, bg, p));
}
static PyObject *_bgFromSequence(PyObject *self, PyObject *args) {
	const char *seq;
	double ps;

	if (!PyArg_ParseTuple(args, "sd", &seq, &ps))
		return NULL;
	return atoPyArray(bgFromSequence(convertSequence(seq), 4, ps));
}

static PyObject *_search(PyObject *self, PyObject *args)
{
    const char *sequence;
    charArray c_seq;
    PyObject *py_matrices;
    PyObject *py_thresholds;
    const char *algorithm;
    int q;
    PyObject *py_absolute_threshold;
    PyObject *py_bg;
    PyObject *py_combine;
    PyObject *py_both_strands;
    bool absolute_threshold;
    double ps = 0.1;
    bool combine = false;
    bool both_strands;
    std::vector<matchArray> matches;

    std::vector<scoreMatrix> matrices;
    scoreArray thresholds;
    scoreArray bg;

    if (!PyArg_ParseTuple(args, "sOOOsiOOO", &sequence, &py_matrices, &py_thresholds, &py_bg, &algorithm, &q, &py_absolute_threshold, &py_combine, &py_both_strands))
        return NULL;

    absolute_threshold = (bool) PyObject_IsTrue(py_absolute_threshold);
    combine = (bool) PyObject_IsTrue(py_combine);
    thresholds = atoDoubleArray(py_thresholds);
    both_strands = (bool) PyObject_IsTrue(py_both_strands);

    c_seq = convertSequence(sequence);
    if(py_bg != Py_None) {
    	bg = atoDoubleArray(py_bg);
    }
    else {
    	if(!absolute_threshold)
    		bg = bgFromSequence(c_seq, 4, ps);
    	else
    		bg = flatBG(4);
    }

    if(!PyList_Check(py_matrices)) {
		return NULL;
	}
	unsigned int num_matrices = (int) PyList_Size(py_matrices);
	if(num_matrices != thresholds.size()) {
		PyErr_SetString(PyExc_RuntimeError, "Thresholds should be as many as matrices");
		return NULL;
	}

	for(unsigned int i=0; i< num_matrices; i++) {
		matrices.push_back(atoDoubleMatrix(PyList_GET_ITEM(py_matrices, i)));
		if(matrices[i].size() != 4) {
			PyErr_SetString(PyExc_RuntimeError, "Matrix size must be 4");
			return NULL;
		}
	}

	//Check if parameter parsing has raised an exception
	if(PyErr_Occurred()) {
		return NULL;
	}

	if(both_strands) {
		for(unsigned int i=0; i < num_matrices; i++) {
			matrices.push_back(reverseComplement(matrices[i]));
			thresholds.push_back(thresholds[i]);
		}
	}
	if(!absolute_threshold) {
		for(unsigned int i=0; i < matrices.size(); i++) {
			matrices[i] = counts2LogOdds(matrices[i], bg, ps);
			thresholds[i] = tresholdFromP(matrices[i], bg, thresholds[i]);
		}
	}

	if(matrices.size() == 1)
		combine = false;

	if((strcmp(algorithm, "lf")== 0 && combine) || strcmp(algorithm, "mlf")==0) {
		matches = multipleMatrixLookaheadFiltrationDNA(q, c_seq, matrices, bg, thresholds);
	}
	else {
		for(unsigned int i = 0; i < matrices.size(); i++) {
			matchArray result;
			if(maxScore(matrices[i]) >= thresholds[i]) {
				if(strcmp(algorithm, "naive") == 0) {
					result = naiveAlgorithm(c_seq, matrices[i], thresholds[i]);
				}
				else if(strcmp(algorithm, "pla") == 0) {
					result = permutatedLookAhead(c_seq, matrices[i], bg, thresholds[i]);
				}
				else if(strcmp(algorithm, "supera") == 0 ||  ( ((int) matrices[0].size()) > (3*q)) ) {
					result = naiveSuperalphabetAlgorithmDNA(q, c_seq, matrices[i], thresholds[i]);
				}
				else if(strcmp(algorithm, "lf") == 0) {
					result = lookaheadFiltrationDNA(q, c_seq, matrices[i], bg, thresholds[i]);
				}
				else {
					//Raise unsupported algorithm exception
					PyErr_SetString(PyExc_RuntimeError, "Unsupported algorithm");
					return NULL;
				}
			}
			matches.push_back(result);
		}
	}
	if(both_strands) {
		if(matches.size() != (2 * num_matrices) ) {
			PyErr_SetString(PyExc_RuntimeError, "Unknown error");
			return NULL;
		}
		for(unsigned int i=0; i < num_matrices; ++i) {
			while(!matches[num_matrices + i].empty()) {
				matches[num_matrices + i].back().position = -matches[num_matrices + i].back().position;
				matches[i].push_back(matches[num_matrices + i].back());
				matches[num_matrices + i].pop_back();
			}
		}
	}
	PyObject *results = PyList_New(num_matrices);
	for(unsigned int i = 0; i < num_matrices; ++i) {
		PyObject *new_match_list = PyList_New(matches[i].size());
		for(unsigned int j=0; j < matches[i].size(); ++j) {
			PyList_SET_ITEM(new_match_list, j, Py_BuildValue("Ld", matches[i][j].position, matches[i][j].score));
		}
		PyList_SET_ITEM(results, i, new_match_list);
	}

    return results;
}

static PyMethodDef _cmodule__methods[] = {
    {"_search",  _search, METH_VARARGS,
     "Search for a pwm match."},
    {"_count_log_odds",  _countLogOdds, METH_VARARGS,
          ""},
    {"_threshold_from_p",  _thresholdFromP, METH_VARARGS,
			""},
	{"_bg_from_sequence",  _bgFromSequence, METH_VARARGS,
			  ""},
    {NULL, NULL, 0, NULL}        /* Sentinel */
};

PyDoc_STRVAR(_cmodule__doc__,
              "Simple filtering of DNA sequences\n");

extern "C" {
MOD_INIT(_cmodule) {
	#if PY_MAJOR_VERSION >= 3
	    static struct PyModuleDef moduledef = {
	        PyModuleDef_HEAD_INIT,
	        "_cmodule",           /* m_name */
	        _cmodule__doc__,      /* m_doc */
	        -1,                  /* m_size */
	        _cmodule__methods,	/* m_methods */
	        NULL,               /* m_reload */
	        NULL,               /* m_traverse */
	        NULL,               /* m_clear */
	        NULL,               /* m_free */
	    };
	    PyObject* m = PyModule_Create(&moduledef);
	    if (m == NULL) { return NULL; }
	    return m;
	#else
		(void) Py_InitModule3("_cmodule", _cmodule__methods, _cmodule__doc__);
	#endif
}

}
