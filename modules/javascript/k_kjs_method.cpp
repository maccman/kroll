/**
 * Appcelerator Kroll - licensed under the Apache Public License 2
 * see LICENSE in the root folder for details on the license.
 * Copyright (c) 2008 Appcelerator, Inc. All Rights Reserved.
 */
#include "javascript_module.h"

namespace kroll
{
	KKJSMethod::KKJSMethod(JSContextRef context, JSObjectRef js_object, JSObjectRef this_obj) :
		context(NULL),
		object(js_object),
		this_obj(this_obj)
	{

		/* KJS methods run in the global context that they originated from
		 * this seems to prevent nasty crashes from trying to access invalid
		 * contexts later. Global contexts need to be registered by all modules
		 * that use a KJS context. */
		JSObjectRef global_object = JSContextGetGlobalObject(context);
		JSGlobalContextRef global_context = KJSUtil::GetGlobalContext(global_object);

		// This context hasn't been registered. Something has gone pretty
		// terribly wrong and Kroll will likely crash soon. Nonetheless, keep
		// the user up-to-date to keep their hopes up.
		if (global_context == NULL)
			std::cerr << "Could not locate global context for a KJS method."  <<
			             " One of the modules is misbehaving." << std::endl;

		this->context = global_context;

		KJSUtil::ProtectGlobalContext(this->context);
		JSValueProtect(this->context, js_object);
		if (this_obj != NULL)
			JSValueProtect(this->context, this_obj);

		this->kjs_bound_object = new KKJSObject(this->context, js_object);
	}

	KKJSMethod::~KKJSMethod()
	{
		JSValueUnprotect(this->context, this->object);
		if (this->this_obj != NULL)
			JSValueUnprotect(this->context, this->this_obj);
		KJSUtil::UnprotectGlobalContext(this->context);
	}

	SharedValue KKJSMethod::Get(const char *name)
	{
		return kjs_bound_object->Get(name);
	}

	void KKJSMethod::Set(const char *name, SharedValue value)
	{
		return kjs_bound_object->Set(name, value);
	}

	bool KKJSMethod::Equals(SharedKObject other)
	{
		AutoPtr<KKJSMethod> kjsOther = other.cast<KKJSMethod>();
		if (kjsOther.isNull())
			return false;
		if (!kjsOther->SameContextGroup(this->context))
			return false;
		return JSValueIsStrictEqual(
			this->context, this->object, kjsOther->GetJSObject());
	}

	SharedStringList KKJSMethod::GetPropertyNames()
	{
		return kjs_bound_object->GetPropertyNames();
	}

	bool KKJSMethod::SameContextGroup(JSContextRef c)
	{
		return kjs_bound_object->SameContextGroup(c);
	}

	JSObjectRef KKJSMethod::GetJSObject()
	{
		return this->object;
	}

	SharedValue KKJSMethod::Call(const ValueList& args)
	{
		JSValueRef* js_args = new JSValueRef[args.size()];
		for (int i = 0; i < (int) args.size(); i++)
		{
			SharedValue arg = args.at(i);
			js_args[i] = KJSUtil::ToJSValue(arg, this->context);
		}

		JSValueRef exception = NULL;
		JSValueRef js_value = JSObjectCallAsFunction(
			this->context,
			this->object,
			this->this_obj,
			args.size(),
			js_args,
			&exception);

		delete [] js_args; // clean up args

		if (js_value == NULL && exception != NULL) //exception thrown
		{
			SharedValue tv_exp = KJSUtil::ToKrollValue(exception, this->context, NULL);
			throw ValueException(tv_exp);
		}

		return KJSUtil::ToKrollValue(js_value, this->context, NULL);
	}

}

