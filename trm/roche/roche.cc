//
// Python/C++ interface file for roche routines.
//

#include <Python.h>
#include "numpy/arrayobject.h"
#include "trm_roche.h"

//----------------------------------------------------------------------------------------
// Finds mass ratio corresponding to a given angle and white dwarf eclipse phase width

static PyObject* 
roche_findq(PyObject *self, PyObject *args)
{
    
    double iangle, pwidth, acc=1.e-4, dq=1.e-5, qlo=0.001, qhi=2.0;
    if(!PyArg_ParseTuple(args, "dd|dddd", &iangle, &pwidth, &acc, &dq, &qlo, &qhi))
	return NULL;
    if(iangle <= 0. || iangle > 90.){
	PyErr_SetString(PyExc_ValueError, "roche.findq: iangle out of range 0 to 90");
	return NULL;
    }
    if(pwidth <= 0. || pwidth > 0.25){
	PyErr_SetString(PyExc_ValueError, "roche.findq: pwidth out of range 0 to 0.25");
	return NULL;
    }
    if(acc <= 0. || acc > 0.1){
	PyErr_SetString(PyExc_ValueError, "roche.findq: acc <= 0 or acc > 0.1");
	return NULL;
    }
    if(dq <= 0. || dq > 0.1){
	PyErr_SetString(PyExc_ValueError, "roche.findq: dq <= 0 or dq > 0.1");
	return NULL;
    }

    Subs::Vec3 r;
    double phi = pwidth/2.;
    bool elo = Roche::fblink(qlo, iangle, phi, r, Roche::SECONDARY, 1.0, acc); 
    bool ehi = Roche::fblink(qhi, iangle, phi, r, Roche::SECONDARY, 1.0, acc); 

    double q;
    if(elo && ehi){
	q = -2.;
    }else if(!elo && !ehi){
	q = -1.;
    }else{
	while(qhi - qlo > dq){
	    q = (qlo+qhi)/2.;
	    if(Roche::fblink(q, iangle, phi, r, Roche::SECONDARY, 1.0, acc))
		qhi = q;
	    else
		qlo = q;
	}
	q = (qlo+qhi)/2.;
    }
    return Py_BuildValue("d", q);
};

//----------------------------------------------------------------------------------------
// Computes whether a given point is eclipsed

static PyObject* 
roche_fblink(PyObject *self, PyObject *args)
{
    
    double q, iangle, x, y, z, phi;
    double ffac=1., acc=1.e-4;
    int star = 2;
    if(!PyArg_ParseTuple(args, "dddddd|ddi", &q, &iangle, &phi, &x, &y, &z, &ffac, &acc, &star))
	return NULL;
    if(q <= 0.){
	PyErr_SetString(PyExc_ValueError, "roche.fblink: q <= 0");
	return NULL;
    }
    if(iangle <= 0. || iangle > 90.){
	PyErr_SetString(PyExc_ValueError, "roche.fblink: iangle out of range 0 to 90");
	return NULL;
    }
    if(ffac <= 0. || ffac > 1.){
	PyErr_SetString(PyExc_ValueError, "roche.fblink: ffac out of range 0 to 1");
	return NULL;
    }
    if(acc <= 0. || acc > 0.1){
	PyErr_SetString(PyExc_ValueError, "roche.fblink: acc <= 0 or acc > 0.1");
	return NULL;
    }
    if(star < 1 || star > 2){
	PyErr_SetString(PyExc_ValueError, "roche.fblink: star must be either 1 or 2");
	return NULL;
    }

    // Compute Roche lobe
    Subs::Vec3 r(x,y,z);
    int eclipse;
    if(Roche::fblink(q, iangle, phi, r, star == 1 ? Roche::PRIMARY : Roche::SECONDARY, ffac, acc))
	eclipse = 1;
    else
	eclipse = 0;
    return Py_BuildValue("i", eclipse);
};

//----------------------------------------------------------------------------------------
// Computes inegress and egress phases of a point

static PyObject* 
roche_ineg(PyObject *self, PyObject *args)
{
    
    double q, iangle, x, y, z;
    double ffac=1., delta=1.e-7;
    int star = 2;
    if(!PyArg_ParseTuple(args, "ddddd|ddi", &q, &iangle, &x, &y, &z, &ffac, &delta, &star))
	return NULL;
    if(q <= 0.){
	PyErr_SetString(PyExc_ValueError, "roche.ineg: q <= 0");
	return NULL;
    }
    if(iangle <= 0. || iangle > 90.){
	PyErr_SetString(PyExc_ValueError, "roche.ineg: iangle out of range 0 to 90");
	return NULL;
    }
    if(ffac <= 0. || ffac > 1.){
	PyErr_SetString(PyExc_ValueError, "roche.ineg: ffac out of range 0 to 1");
	return NULL;
    }
    if(delta <= 0.){
	PyErr_SetString(PyExc_ValueError, "roche.ineg: delta <= 0");
	return NULL;
    }
    if(star < 1 || star > 2){
	PyErr_SetString(PyExc_ValueError, "roche.ineg: star must be either 1 or 2");
	return NULL;
    }

    // Compute Roche lobe
    Subs::Vec3 r(x,y,z);
    double ingress, egress;
    if(!Roche::ingress_egress(q, ffac, iangle, r, ingress, egress, delta, star == 1 ? Roche::PRIMARY : Roche::SECONDARY)){
	PyErr_SetString(PyExc_ValueError, "roche.ineg: point is not eclipsed");
	return NULL;
    }
    return Py_BuildValue("dd", ingress, egress);
};

//----------------------------------------------------------------------------------------
// Returns tuple of x, y arrays representing the secondary star's Roche lobe

static PyObject* 
roche_lobe1(PyObject *self, PyObject *args)
{

    double q;
    int n = 200;
    if(!PyArg_ParseTuple(args, "d|i:roche.lobe1", &q, &n))
	return NULL;
    if(q <= 0.){
	PyErr_SetString(PyExc_ValueError, "roche.lobe1: q <= 0");
	return NULL;
    }
    if(n < 2){
	PyErr_SetString(PyExc_ValueError, "roche.lobe1: n < 2");
	return NULL;
    }

    // Create output array containing both x and y arrays.
    npy_intp dim[2] = {2, n};

    PyArrayObject* r = NULL;
    r = (PyArrayObject*) PyArray_SimpleNew(2, dim, PyArray_FLOAT);
    if(r == NULL) return NULL;

    float* x = (float*)r->data;
    float* y = x + n;

    // Compute Roche lobe
    Roche::lobe1(q, x, y, n);

    return PyArray_Return(r);

};

//----------------------------------------------------------------------------------------
// Returns tuple of x, y arrays representing the secondary star's Roche lobe

static PyObject* 
roche_lobe2(PyObject *self, PyObject *args)
{

    double q;
    int n = 200;
    if(!PyArg_ParseTuple(args, "d|i:roche.lobe2", &q, &n))
	return NULL;
    if(q <= 0.){
	PyErr_SetString(PyExc_ValueError, "roche.lobe2: q <= 0");
	return NULL;
    }
    if(n < 2){
	PyErr_SetString(PyExc_ValueError, "roche.lobe2: n < 2 in");
	return NULL;
    }

    // Create output array containing both x and y arrays.
    npy_intp dim[2] = {2, n};

    PyArrayObject* r = NULL;
    r = (PyArrayObject*) PyArray_SimpleNew(2, dim, PyArray_FLOAT);
    if(r == NULL) return NULL;

    float* x = (float*)r->data;
    float* y = x + n;

    // Compute Roche lobe
    Roche::lobe2(q, x, y, n);

    return PyArray_Return(r);

};

//----------------------------------------------------------------------------------------
// Returns tuple of x, y arrays representing the gas stream

static PyObject* 
roche_stream(PyObject *self, PyObject *args)
{

    double q, rad;
    int n = 200;
    if(!PyArg_ParseTuple(args, "dd|i:roche.stream", &q, &rad, &n))
	return NULL;
    if(q <= 0.){
	PyErr_SetString(PyExc_ValueError, "roche.stream: q <= 0");
	return NULL;
    }
    if(rad < 0. || rad > 1.){
	PyErr_SetString(PyExc_ValueError, "roche.stream: rad < 0 or > 1.");
	return NULL;
    }
    if(n < 2){
	PyErr_SetString(PyExc_ValueError, "roche.stream: n < 2");
	return NULL;
    }

    // Create output array containing both x and y arrays.
    npy_intp dim[2] = {2, n};

    PyArrayObject* r = NULL;
    r = (PyArrayObject*) PyArray_SimpleNew(2, dim, PyArray_FLOAT);
    if(r == NULL) return NULL;

    float* x = (float*)r->data;
    float* y = x + n;

    // Carry out the stream integration

    Roche::streamr(q, rad, x, y, n);

    return PyArray_Return(r);

};

//----------------------------------------------------------------------------------------
// strmnx

static PyObject* 
roche_strmnx(PyObject *self, PyObject *args)
{
    double q, acc=1.e-7;
    int n = 1;
    if(!PyArg_ParseTuple(args, "d|id", &q, &n, &acc))
	return NULL;
    if(q < 0){
	PyErr_SetString(PyExc_ValueError, "roche.strmnx: q <= 0");
	return NULL;
    }
    if(n < 1){
	PyErr_SetString(PyExc_ValueError, "roche.strmnx: n < 1");
	return NULL;
    }
    if(acc <= 0.){
	PyErr_SetString(PyExc_ValueError, "roche.strmnx: acc <= 0");
	return NULL;
    }
    Subs::Vec3 r, v;
    Roche::strinit(q, r, v);
    for(int i=0; i<n; i++)
	Roche::strmnx(q, r, v, acc);
    double tvx1, tvy1, tvx2, tvy2;
    Roche::vtrans(q, 1, r.x(), r.y(), v.x(), v.y(), tvx1, tvy1);
    Roche::vtrans(q, 2, r.x(), r.y(), v.x(), v.y(), tvx2, tvy2);
    return Py_BuildValue("dddddd", r.x(), r.y(), tvx1, tvy1, tvx2, tvy2);
};


//----------------------------------------------------------------------------------------
// Returns tuple of x, y arrays representing the primary star's Roche lobe in velocity space

static PyObject* 
roche_vlobe1(PyObject *self, PyObject *args)
{

    double q;
    int n = 200;
    if(!PyArg_ParseTuple(args, "d|i:roche.vlobe1", &q, &n))
	return NULL;
    if(q <= 0.){
	PyErr_SetString(PyExc_ValueError, "roche.vlobe1: q <= 0");
	return NULL;
    }
    if(n < 2){
	PyErr_SetString(PyExc_ValueError, "roche.vlobe1: n < 2");
	return NULL;
    }

    // Create output array containing both x and y arrays.
    npy_intp dim[2] = {2, n};

    PyArrayObject* v = NULL;
    v = (PyArrayObject*) PyArray_SimpleNew(2, dim, PyArray_FLOAT);
    if(v == NULL) return NULL;

    float* vx = (float*)v->data;
    float* vy = vx + n;

    // Compute Roche lobe
    Roche::vlobe1(q, vx, vy, n);

    return PyArray_Return(v);

};


//----------------------------------------------------------------------------------------
// Returns tuple of vx, vy arrays representing the secondary star's Roche lobe in velocity space

static PyObject* 
roche_vlobe2(PyObject *self, PyObject *args)
{

    double q;
    int n = 200;
    if(!PyArg_ParseTuple(args, "d|i:roche.vlobe2", &q, &n))
	return NULL;
    if(q <= 0.){
	PyErr_SetString(PyExc_ValueError, "roche.vlobe2: q <= 0");
	return NULL;
    }
    if(n < 2){
	PyErr_SetString(PyExc_ValueError, "roche.vlobe2: n < 2");
	return NULL;
    }

    // Create output array containing both x and y arrays.
    npy_intp dim[2] = {2, n};

    PyArrayObject* v = NULL;
    v = (PyArrayObject*) PyArray_SimpleNew(2, dim, PyArray_FLOAT);
    if(v == NULL) return NULL;

    float* vx = (float*)v->data;
    float* vy = vx + n;

    // Compute Roche lobe
    Roche::vlobe2(q, vx, vy, n);

    return PyArray_Return(v);

};

//----------------------------------------------------------------------------------------
// Returns tuple of vx, vy arrays representing the gas stream

static PyObject* 
roche_vstream(PyObject *self, PyObject *args)
{

    double q, step = 0.01;
    int stype=1, n = 60;
    if(!PyArg_ParseTuple(args, "d|dii:roche.vstream", &q, &step, &stype, &n))
	return NULL;
    if(q <= 0.){
	PyErr_SetString(PyExc_ValueError, "roche.vstream: q <= 0");
	return NULL;
    }
    if(step <= 0. || step >= 1.){
	PyErr_SetString(PyExc_ValueError, "roche.vstream: step <= 0 or >= 1.");
	return NULL;
    }
    if(stype < 1 && stype > 2){
	PyErr_SetString(PyExc_ValueError, "roche.vstream: stype must be 1 or 2");
	return NULL;
    }
    if(n < 2){
	PyErr_SetString(PyExc_ValueError, "roche.vstream: n < 2");
	return NULL;
    }

    // Create output array containing both x and y arrays.
    npy_intp dim[2] = {2, n};

    PyArrayObject* v = NULL;
    v = (PyArrayObject*) PyArray_SimpleNew(2, dim, PyArray_FLOAT);
    if(v == NULL) return NULL;

    float* vx = (float*)v->data;
    float* vy = vx + n;

    // Carry out the stream integration
    try{
	Roche::vstrreg(q, step, vx, vy, n, stype);
    }
    catch(const Roche::Roche_Error &err){
	PyErr_SetString(PyExc_ValueError, ("roche.vstream: " + err).c_str());
	return NULL;
    }

    return PyArray_Return(v);

};

//----------------------------------------------------------------------------------------
// xl1

static PyObject* 
roche_xl1(PyObject *self, PyObject *args)
{
    double q;
    if(!PyArg_ParseTuple(args, "d:roche.xl1", &q))
	return NULL;
    if(q < 0){
	PyErr_SetString(PyExc_ValueError, "roche.xl1: q <= 0");
	return NULL;
    }
    double x = Roche::xl1(q);
    return Py_BuildValue("f", x);
};

//----------------------------------------------------------------------------------------
// xl2

static PyObject* 
roche_xl2(PyObject *self, PyObject *args)
{
    double q;
    if(!PyArg_ParseTuple(args, "d:roche.xl2", &q))
	return NULL;
    if(q < 0){
	PyErr_SetString(PyExc_ValueError, "roche.xl2: q <= 0");
	return NULL;
    }
    double x = Roche::xl2(q);
    return Py_BuildValue("f", x);
};

//----------------------------------------------------------------------------------------
// xl3

static PyObject* 
roche_xl3(PyObject *self, PyObject *args)
{
    double q;
    if(!PyArg_ParseTuple(args, "d:roche.xl3", &q))
	return NULL;
    if(q < 0){
	PyErr_SetString(PyExc_ValueError, "roche.cl3: q <= 0");
	return NULL;
    }
    double x = Roche::xl3(q);
    return Py_BuildValue("f", x);
};


//----------------------------------------------------------------------------------------
// The methods

static PyMethodDef RocheMethods[] = {

    {"findq", roche_findq, METH_VARARGS, 
     "findq(i, pwidth, acc=1.e-4, dq=1.e-5, qlo=0.005, qhi=2.), computes mass ratio q for a given phase width and inclination"},

    {"fblink", roche_fblink, METH_VARARGS, 
     "fblink(q, i, phi, x, y, z, ffac=1., acc=1.e-4, star=2), computes whether a point is eclipsed or not"},

    {"ineg", roche_ineg, METH_VARARGS, 
     "(in,out) = ineg(q, i, x, y, z, ffac=1., delta=1.e-7, star=2), computes ingress and egress phase of a point"},

    {"lobe1", roche_lobe1, METH_VARARGS, 
     "r = lobe1(q, n=200), q = M2/M1. Returns 2xn array of primary star's Roche lobe."},

    {"lobe2", roche_lobe2, METH_VARARGS, 
     "r = lobe2(q, n=200), q = M2/M1. Returns 2xn array of secondary star's Roche lobe."},

    {"stream", roche_stream, METH_VARARGS, 
     "r = stream(q, rad, n=200), q = M2/M1. Returns 2xn array of the gas stream."},

    {"strmnx", roche_strmnx, METH_VARARGS, 
     "(x,y,vx1,vy1,vx2,vy2) = strmnx(q, n=1, acc=1.e-7), q = M2/M1. Calculates position & velocity of n-th turning point of stream."},

    {"vlobe1", roche_vlobe1, METH_VARARGS, 
     "v = vlobe1(q, n=200), q = M2/M1. Returns 2xn array of primary star's Roche lobe in velocity space."},

    {"vlobe2", roche_vlobe2, METH_VARARGS, 
     "v = vlobe2(q, n=200), q = M2/M1. Returns 2xn array of secondary star's Roche lobe in velocity space."},

    {"vstream", roche_vstream, METH_VARARGS, 
     "v = vstream(q, step=0.01, type=1, n=60), q = M2/M1. Returns 2xn array of positions of the gas stream in velocity space."},

    {"xl1", roche_xl1, METH_VARARGS, 
     "xl1(q), q = M2/M1. Calculate the inner Lagrangian point distance."},

    {"xl2", roche_xl2, METH_VARARGS, 
     "xl2(q), q = M2/M1. Calculate the L2 point distance."},

    {"xl3", roche_xl3, METH_VARARGS, 
     "xl3(q), q = M2/M1. Calculate the L3 point distance."},

    {NULL, NULL, 0, NULL} /* Sentinel */
};

PyMODINIT_FUNC
init_roche(void)
{
    (void) Py_InitModule("_roche", RocheMethods);
    import_array();
}
