/**
 * Appcelerator Kroll - licensed under the Apache Public License 2
 * see LICENSE in the root folder for details on the license.
 * Copyright (c) 2008 Appcelerator, Inc. All Rights Reserved.
 */
#include "javascript_module.h"
#include <iostream>
#include <fstream>
#include <string>

namespace kroll
{
	// TODO: Implement real method metadata and lifecycle events for
	// scripting language-based modules
	JavascriptModuleInstance::JavascriptModuleInstance(Host *host, std::string path, std::string dir, std::string name) :
		Module(host, dir.c_str(), name.c_str(), "0.1"), path(path)
	{
		try
		{
			this->Load();
			this->Run();
		}
		catch (ValueException& e)
		{
			SharedString ss = e.GetValue()->DisplayString();
			Logger *logger = Logger::Get("Javascript");
			logger->Error("Could not execute %s because %s", path.c_str(), (*ss).c_str());
		}

	}
	void JavascriptModuleInstance::Initialize () {}
	void JavascriptModuleInstance::Destroy () {}

	void JavascriptModuleInstance::Load()
	{
		this->code = "";
		std::ifstream js_file(this->path.c_str());
		if (!js_file.is_open())
		{
			throw ValueException::FromString("Could not read Javascript file");
		}

		std::string line;
		while (!js_file.eof() )
		{
			std::getline(js_file, line);
			this->code.append(line);
			this->code.append("\n");
		}
		js_file.close();
	}

	void JavascriptModuleInstance::Run()
	{

		JSValueRef exception;
		JSGlobalContextRef context = JSGlobalContextCreate(NULL);
		JSObjectRef global_object = JSContextGetGlobalObject(context);
		KJSUtil::RegisterGlobalContext(global_object, context);

		/* Take some steps to insert the API into the Javascript context */
		/* Create a crazy, crunktown delegate hybrid object for Javascript */
		SharedValue global_value = Value::NewObject(host->GetGlobalObject());

		/* convert JS API to a KJS object */
		JSValueRef js_api = KJSUtil::ToJSValue(global_value, context);

		/* set the API as a property of the global object */
		JSStringRef prop_name = JSStringCreateWithUTF8CString(PRODUCT_NAME);
		JSObjectSetProperty(context, global_object, prop_name,
		                    js_api, kJSPropertyAttributeNone, NULL);

		/* Try to run the script */
		JSStringRef js_code = JSStringCreateWithUTF8CString(this->code.c_str());

		/* check script syntax */
		bool syntax = JSCheckScriptSyntax(context, js_code, NULL, 0, &exception);
		if (!syntax)
		{
			SharedValue e = KJSUtil::ToKrollValue(exception, context, NULL);
			throw ValueException(e);
		}

		/* evaluate the script */
		JSValueRef ret = JSEvaluateScript(context, js_code,
		                                  NULL, NULL,
		                                  1, &exception);
		if (ret == NULL)
		{
			SharedValue e = KJSUtil::ToKrollValue(exception, context, NULL);
			throw ValueException(e);
		}
		
		// null it out so we don't hold a bunch of this in memory
		this->code = "";
	}

}

