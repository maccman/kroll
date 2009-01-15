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
	JavascriptModuleInstance::JavascriptModuleInstance(Host *host, std::string& path)
		: Module(host,FileUtils::GetDirectory(path)), path(path)
	{
		try
		{
			this->Load();
			this->Run();
		}
		catch (Value *e)
		{
			ScopedDereferencer s(e);
			std::cerr << "Could not execute " << path <<
			             " because: " << e->DisplayString() << std::endl;
		}
	
	}
	void JavascriptModuleInstance::Initialize () {}
	void JavascriptModuleInstance::Destroy () {}
	
	void JavascriptModuleInstance::Load()
	{
		this->code = "";
		std::ifstream js_file(this->path.c_str());
		if (!js_file.is_open())
			throw "Could not read Javascript file";
	
		std::string line;
		while (!js_file.eof() )
		{
			std::getline(js_file, line);
			this->code += line;
		}
		js_file.close();
	}
	
	void JavascriptModuleInstance::Run()
	{
	
		JSValueRef exception;
		JSGlobalContextRef context = JSGlobalContextCreate(NULL);
	
		/* Take some steps to insert the API into the Javascript context */
		/* Create a crazy, crunktown delegate hybrid object for Javascript */
		Value *api = host->GetGlobalObject()->Get("api");
		if (!api->IsObject())
			throw new Value("Could not find API module");
		ScopeMethodDelegate::CreateDelegate(host->GetGlobalObject(), api->ToObject());
	
		/* convert JS API to a KJS object */
		JSValueRef js_api = KJSUtil::ToJSValue(api, context);
	
		/* set the API as a property of the global object */
		JSObjectRef global_object = JSContextGetGlobalObject(context);
		JSStringRef prop_name = JSStringCreateWithUTF8CString(PRODUCT_NAME);
		JSObjectSetProperty(context, global_object, prop_name, 
		                    js_api, kJSPropertyAttributeNone, NULL);
	
		/* Try to run the script */
		JSStringRef js_code = JSStringCreateWithUTF8CString(this->code.c_str());
	
		/* check script syntax */
		bool syntax = JSCheckScriptSyntax(context, js_code, NULL, 0, &exception);
		if (!syntax)
			throw KJSUtil::ToKrollValue(exception, context, NULL);
	
		/* evaluate the script */
		JSValueRef ret = JSEvaluateScript(context, js_code,
		                                  NULL, NULL,
		                                  1, &exception);
		if (ret == NULL)
			throw KJSUtil::ToKrollValue(exception, context, NULL);
	}
	
	const char* JavascriptModuleInstance::GetName()
	{
		return path.c_str();
	}
}
