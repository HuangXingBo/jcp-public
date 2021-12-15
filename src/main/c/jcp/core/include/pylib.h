// Licensed to the Apache Software Foundation (ASF) under one
// or more contributor license agreements.  See the NOTICE file
// distributed with this work for additional information
// regarding copyright ownership.  The ASF licenses this file
// to you under the Apache License, Version 2.0 (the
// "License"); you may not use this file except in compliance
// with the License.  You may obtain a copy of the License at
//
//   http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing,
// software distributed under the License is distributed on an
// "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
// KIND, either express or implied.  See the License for the
// specific language governing permissions and limitations
// under the License.

#include <jni.h>
#include <Python.h>

#include <pyutils.h>
#include <pyexceptions.h>
#include <java_class/JavaClass.h>
#include <jcpport.h>

#ifndef _Included_pylib
#define _Included_pylib

#define DICT_KEY "jcp"

struct __JcpThread {
    char          *callable_name;
    PyObject      *callable;
    PyObject      *globals;
    PyThreadState *tstate;
    JNIEnv        *env;
};
typedef struct __JcpThread JcpThread;


#define Jcp_BEGIN_ALLOW_THREADS { \
                        JcpThread* jcp_thread; \
                        jcp_thread = (JcpThread*) ptr; \
                        PyEval_AcquireThread(jcp_thread->tstate);

#define Jcp_END_ALLOW_THREADS    PyEval_ReleaseThread(jcp_thread->tstate); \
                 }


/* Initialization and finalization */
JcpAPI_FUNC(void) JcpPy_Initialize(JNIEnv *);
JcpAPI_FUNC(void) JcpPy_Finalize(JavaVM *);
JcpAPI_FUNC(intptr_t) JcpPy_InitThread(JNIEnv *);
JcpAPI_FUNC(void) JcpPy_FinalizeThread(intptr_t);

/* Add path to search path of Main Interpreter */
JcpAPI_FUNC(void) JcpPy_AddSearchPath(JNIEnv *, jstring);

/* Import module to the Main Interpreter so that the module can be shared by all interpreters */
JcpAPI_FUNC(void) JcpPy_ImportModule(JNIEnv *, jstring);

// ------------------------------ set/get parameters methods----------------------

/* Function to set a Java boolean value in the specified PyThreadState */
JcpAPI_FUNC(void) JcpPyObject_SetJBoolean(JNIEnv *, intptr_t, const char *, jboolean);

/* Function to set a Java int value in the specified PyThreadState */
JcpAPI_FUNC(void) JcpPyObject_SetJInt(JNIEnv *, intptr_t, const char *, jint);

/* Function to set a Java long value in the specified PyThreadState */
JcpAPI_FUNC(void) JcpPyObject_SetJLong(JNIEnv *, intptr_t, const char *, jlong);

/* Function to set a Java double value in the specified PyThreadState */
JcpAPI_FUNC(void) JcpPyObject_SetJDouble(JNIEnv *, intptr_t, const char *, jdouble);

/* Function to set a Java String in the specified PyThreadState */
JcpAPI_FUNC(void) JcpPyObject_SetJString(JNIEnv *, intptr_t, const char *, jstring);

/* Function to set a Java Object in the specified PyThreadState */
JcpAPI_FUNC(void) JcpPyObject_SetJObject(JNIEnv *, intptr_t, const char *, jobject);

static inline void
_JcpPyObject_SetPyObject(PyObject* globals, const char *name, PyObject* value)
{
    if (value) {
        PyDict_SetItemString(globals, name, value);
        Py_DECREF(value);
    }
}

/* Function to get the Java Object by the name in the specified PyThreadState */
JcpAPI_FUNC(jobject) JcpPyObject_GetJObject(JNIEnv *, intptr_t, const char *, jclass);

// ------------------------------ Core JcpPyObject call functions----------------------
static inline PyObject *
_get_callable(JNIEnv* env, JcpThread* jcp_thread, const char *name)
{

    int size;
    char* dot;
    char* global_name;
    char* error_buf;

    PyObject* globals;
    PyObject* callable;
    PyObject* obj;

    if (jcp_thread->callable_name != NULL && strcmp(jcp_thread->callable_name, name) == 0) {
        return jcp_thread->callable;
    } else {
        globals = jcp_thread->globals;
        callable = PyObject_GetAttrString(globals, name);
        if (!callable) {
            dot = strchr(name, '.');
            if (dot) {
                global_name = malloc((dot - name + 1) * sizeof(char));
                strncpy(global_name, name, dot - name);
                global_name[dot - name] = '\0';

                obj = PyDict_GetItemString(globals, global_name);
                if (obj) {
                    callable = PyObject_GetAttrString(obj, dot + 1);
                    if (!callable) {
                        JcpPyErr_Throw(env);
                    }
                } else {
                    error_buf = malloc((strlen(global_name) + 64) * sizeof(char));
                    error_buf[strlen(error_buf) + 1] = '\0';
                    snprintf(error_buf, strlen(error_buf) + 1, "Unable to find object with name: %s", global_name);
                    (*env)->ThrowNew(env, JPYTHONEXCE_TYPE, error_buf);
                    free(error_buf);
                }

                free(global_name);
            } else {
                error_buf = malloc((strlen(name) + 64) * sizeof(char));
                error_buf[strlen(error_buf) + 1] = '\0';
                snprintf(error_buf, strlen(error_buf) + 1, "Unable to find object with name: %s", name);
                (*env)->ThrowNew(env, JPYTHONEXCE_TYPE, error_buf);
                free(error_buf);
            }
        }
        if (callable) {
            if (jcp_thread->callable_name) {
                free(jcp_thread->callable_name);
            }
            size = sizeof(char) * (strlen(name) + 1);
            jcp_thread->callable_name = malloc(size);
            memset(jcp_thread->callable_name, '\0', size);
            strcpy(jcp_thread->callable_name, name);
            jcp_thread->callable = callable;
        }
        return callable;
    }
}

static inline jobject
_JcpPyObject_CallOneArg(JNIEnv *env, JcpThread* jcp_thread, const char *name, PyObject* arg)
{

    PyObject* callable = NULL;
    PyObject* py_ret = NULL;

    jobject result = NULL;

    if (arg) {
        callable = _get_callable(env, jcp_thread, name);

        if (callable) {
#if PY_MINOR_VERSION >= 9
            py_ret = PyObject_CallOneArg(callable, arg);
#else
            py_ret = PyObject_CallFunctionObjArgs(callable, arg, NULL);
#endif

            if (!JcpPyErr_Throw(env)) {
                result = JcpPyObject_AsJObject(env, py_ret, JOBJECT_TYPE);
                Py_DECREF(py_ret);
            }

        }
        Py_DECREF(arg);
    }

    return result;
}

/* Call a callable Python object without any arguments */
JcpAPI_FUNC(jobject) JcpPyObject_CallNoArgs(JNIEnv *, intptr_t, const char *);

/* Call a callable Python object with only one jboolean argument */
JcpAPI_FUNC(jobject) JcpPyObject_CallOneJBooleanArg(JNIEnv *, intptr_t, const char *, jboolean);

/* Call a callable Python object with only one jint argument */
JcpAPI_FUNC(jobject) JcpPyObject_CallOneJIntArg(JNIEnv *, intptr_t, const char *, jint);

/* Call a callable Python object with only one jlong argument */
JcpAPI_FUNC(jobject) JcpPyObject_CallOneJLongArg(JNIEnv *, intptr_t, const char *, jlong);

/* Call a callable Python object with only one jdouble argument */
JcpAPI_FUNC(jobject) JcpPyObject_CallOneJDoubleArg(JNIEnv *, intptr_t, const char *, jdouble);

/* Call a callable Python object with only one jstring argument */
JcpAPI_FUNC(jobject) JcpPyObject_CallOneJStringArg(JNIEnv *, intptr_t, const char *, jstring);

/* Call a callable Python object with only one jobject argument */
JcpAPI_FUNC(jobject) JcpPyObject_CallOneJObjectArg(JNIEnv *, intptr_t, const char *, jobject);

/* Call a callable Python object */
JcpAPI_FUNC(jobject) JcpPyObject_Call(JNIEnv *, intptr_t, const char *, jobjectArray, jobject);


static inline jobject
_JcpPyObject_Call_MethodOneArg(JNIEnv *env, JcpThread* jcp_thread, const char *obj,
                               const char *name, PyObject* py_arg)
{
    PyObject* py_obj = NULL;
    PyObject* py_ret = NULL;
    jobject result = NULL;

    if (py_arg) {
        py_obj = PyDict_GetItemString(jcp_thread->globals, obj);

        if (py_obj) {
            PyObject* py_name = PyUnicode_FromString(name);

#if PY_MINOR_VERSION >= 9
            py_ret = PyObject_CallMethodOneArg(py_obj, py_name, py_arg);
#else
            py_ret = PyObject_CallMethodObjArgs(py_obj, py_name, py_arg, NULL);
#endif

            Py_DECREF(py_name);

            if (!JcpPyErr_Throw(env)) {
                result = JcpPyObject_AsJObject(env, py_ret, JOBJECT_TYPE);
                Py_DECREF(py_ret);
            }

        }
        Py_DECREF(py_arg);
    }

    return result;
}

/* Call the method named 'name' of object 'obj' without arguments */
JcpAPI_FUNC(jobject) JcpPyObject_CallMethodNoArgs(JNIEnv *, intptr_t, const char *, const char *);

/* Call the method named 'name' of object 'obj' with only one jboolean argument */
JcpAPI_FUNC(jobject) JcpPyObject_CallMethodOneJBooleanArg(JNIEnv *, intptr_t, const char *, const char *, jboolean);

/* Call the method named 'name' of object 'obj' with only one jint argument */
JcpAPI_FUNC(jobject) JcpPyObject_CallMethodOneJIntArg(JNIEnv *, intptr_t, const char *, const char *, jint);

/* Call the method named 'name' of object 'obj' with only one jlong argument */
JcpAPI_FUNC(jobject) JcpPyObject_CallMethodOneJLongArg(JNIEnv *, intptr_t, const char *, const char *, jlong);

/* Call the method named 'name' of object 'obj' with only one jdouble argument */
JcpAPI_FUNC(jobject) JcpPyObject_CallMethodOneJDoubleArg(JNIEnv *, intptr_t, const char *, const char *, jdouble);

/* Call the method named 'name' of object 'obj' with only one jstring argument */
JcpAPI_FUNC(jobject) JcpPyObject_CallMethodOneJStringArg(JNIEnv *, intptr_t, const char *, const char *, jstring);

/* Call the method named 'name' of object 'obj' with only one jobject argument */
JcpAPI_FUNC(jobject) JcpPyObject_CallMethodOneJObjectArg(JNIEnv *, intptr_t, const char *, const char *, jobject);

/* Call the method named 'name' of object 'obj' with a variable number of Java arguments */
JcpAPI_FUNC(jobject) JcpPyObject_CallMethod(JNIEnv *, intptr_t, const char *, const char *, jobjectArray);

// ----------------------------------------------------------------------------------------

/* Exec python code */
JcpAPI_FUNC(void) JcpExec(JNIEnv *, intptr_t, const char *);

/* Eval python code */
JcpAPI_FUNC(void) JcpEval(JNIEnv *, intptr_t, const char *);

/* Compile python code */
JcpAPI_FUNC(jint) JcpCompile(JNIEnv *, intptr_t, const char *);

#endif // ifndef _Included_pylib