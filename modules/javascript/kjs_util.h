/**
 * Appcelerator Kroll - licensed under the Apache Public License 2
 * see LICENSE in the root folder for details on the license.
 * Copyright (c) 2008 Appcelerator, Inc. All Rights Reserved.
 */

#ifndef _KJS_UTIL_H_
#define _KJS_UTIL_H_

namespace kroll
{

	class KROLL_JAVASCRIPT_API KJSUtil
	{

	public:
		static SharedValue ToKrollValue(JSValueRef, JSContextRef, JSObjectRef);
		static JSValueRef ToJSValue(SharedValue, JSContextRef);
		static JSValueRef KObjectToJSValue(SharedValue, JSContextRef);
		static JSValueRef KMethodToJSValue(SharedValue, JSContextRef);
		static JSValueRef KListToJSValue(SharedValue, JSContextRef);
		static char* ToChars(JSStringRef);
		static bool IsArrayLike(JSObjectRef, JSContextRef);
		static AutoPtr<KKJSObject> ToBoundObject(JSContextRef, JSObjectRef);
		static AutoPtr<KKJSObject> ToBoundMethod(JSContextRef, JSObjectRef, JSObjectRef);
		static AutoPtr<KKJSList> ToBoundList(JSContextRef, JSObjectRef);

		static void RegisterGlobalContext(JSObjectRef, JSGlobalContextRef);
		static JSGlobalContextRef GetGlobalContext(JSObjectRef);

		static void ProtectGlobalContext(JSGlobalContextRef);
		static void UnprotectGlobalContext(JSGlobalContextRef);

		static SharedValue Evaluate(JSContextRef context, char *script);
		static SharedValue EvaluateFile(JSContextRef context, char *full_path);

		static JSValueRef GetFunctionPrototype(JSContextRef jsContext, JSValueRef* exception);
		static JSValueRef GetArrayPrototype(JSContextRef jsContext, JSValueRef* exception);

	private:
		static std::map<JSObjectRef, JSGlobalContextRef> contextMap;
		static std::map<JSGlobalContextRef, int> contextRefCounts;

	};

}

#endif
