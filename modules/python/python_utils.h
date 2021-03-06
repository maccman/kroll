/**
 * Appcelerator Kroll - licensed under the Apache Public License 2
 * see LICENSE in the root folder for details on the license.
 * Copyright (c) 2008 Appcelerator, Inc. All Rights Reserved.
 */
#ifndef PYTHON_TYPES_H_
#define PYTHON_TYPES_H_

#include <typeinfo>
#include "python_module.h"

namespace kroll
{
	class PythonUtils
	{
	public:
		static SharedValue ToKrollValue(PyObject* value);
		static PyObject* ToPyObject(SharedValue value);
		static PyObject* ToPyObject(const ValueList& list);
		static const char* ToString(PyObject* value);
		static PyObject* KObjectToPyObject(SharedValue o);
		static PyObject* KMethodToPyObject(SharedValue o);
		static PyObject* KListToPyObject(SharedValue o);

	private:
		PythonUtils() {}
		~PythonUtils () {}
	};
}

#endif
