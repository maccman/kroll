/*
 * Appcelerator Kroll - licensed under the Apache Public License 2
 * see LICENSE in the root folder for details on the license.
 * Copyright (c) 2008 Appcelerator, Inc. All Rights Reserved.
 */
/**
 * This file is the easiest way to gain access to the full Kroll API
 * To use it, just do the following:
 * \code
 * #include <kroll/kroll.h>
 * \endcode
 */

#ifndef _KROLL_H_
#define _KROLL_H_

#include "base.h"
#include <Poco/SharedPtr.h>
#include <Poco/AutoPtr.h>
#include <vector>

using Poco::SharedPtr;
using Poco::AutoPtr;

#ifndef OS_WIN32
	// this is important which essentially marks all of
	// these classes below and the typedef/templates to be
	// visible outside of the library.  if you don't do this
	// you won't be able to catch exceptions of SharedValue for
	// example
	#pragma GCC visibility push(default)
#endif

namespace kroll
{
	class Value;
	class KObject;
	class KMethod;
	class KList;

	class StaticBoundObject;
	class StaticBoundMethod;
	class StaticBoundList;

	class DelegateStaticBoundObject;
	class ScopeMethodDelegate;
	class Blob;
	class ValueReleasePolicy;
	class Logger;
	class ArgList;

	typedef AutoPtr<Value> AutoValue;
	typedef AutoPtr<KObject> AutoKObject;
	typedef AutoPtr<KMethod> AutoKMethod;
	typedef AutoPtr<KList> AutoKList;
	typedef AutoPtr<Value> SharedValue;
	typedef AutoPtr<KObject> SharedKObject;
	typedef AutoPtr<KMethod> SharedKMethod;
	typedef AutoPtr<KList> SharedKList;
	typedef AutoPtr<Blob> AutoBlob;

	typedef SharedPtr<std::string> SharedString;
	typedef std::vector<SharedString> StringList;
	typedef SharedPtr<StringList> SharedStringList;

	typedef ArgList ValueList;

	class Module;
	class Application;
	class KComponent;
	class Dependency;

	typedef SharedPtr<KComponent> SharedComponent;
	typedef SharedPtr<Application> SharedApplication;
	typedef SharedPtr<Dependency> SharedDependency;
}

#ifndef OS_WIN32
	#pragma GCC visibility pop
#endif

#include "net/net.h"
#include "reference_counted.h"
#include "utils/utils.h"
#include "logger.h"
#include "mutex.h"
#include "scoped_lock.h"

#include "binding/binding.h"
#include "module_provider.h"
#include "module.h"
#include "async_job.h"
#include "host.h"

#ifdef OS_OSX
#include "osx/osx.h"
#elif defined(OS_WIN32)
#include "win32/win32.h"
#endif

#endif
