/**
 * Appcelerator Kroll - licensed under the Apache Public License 2
 * see LICENSE in the root folder for details on the license.
 * Copyright (c) 2008-2009 Appcelerator, Inc. All Rights Reserved.
 */

#include "boot.h"
#include "popup_dialog_win32.h"
#include <process.h>
#include <windows.h>
using std::wstring;

#ifndef MAX_PATH
#define MAX_PATH 512
#endif

#ifdef USE_BREAKPAD
#include "client/windows/handler/exception_handler.h"
#include "common/windows/http_upload.h"
#endif

namespace KrollBoot
{
	extern string applicationHome;
	extern string updateFile;
	extern SharedApplication app;
	extern int argc;
	extern const char** argv;
	const char *preload[] = { "zlib1.dll", "libeay32.dll", "ssleay32.dll", "libpng13.dll", "libxml2.dll", "libxslt.dll" };
	const int preloadSize = sizeof(preload)/sizeof(preload[0]);
	
	inline void ShowError(string msg, bool fatal)
	{
		std::cerr << "Error: " << msg << std::endl;
		MessageBoxA(NULL, msg.c_str(), GetApplicationName().c_str(), MB_OK|MB_ICONERROR|MB_SYSTEMMODAL);
		if (fatal)
			exit(1);
	}

	std::string GetApplicationHomePath()
	{
		char path[MAX_PATH];
		int size = GetModuleFileNameA(GetModuleHandle(NULL), (char*)path, MAX_PATH);
		if (size>0)
		{
			path[size] = '\0';
		}
		return FileUtils::Dirname(string(path));
	}

	bool IsWindowsXP()
	{
		OSVERSIONINFO osVersion;
		osVersion.dwOSVersionInfoSize = sizeof(OSVERSIONINFO);
		::GetVersionEx(&osVersion);
		return osVersion.dwMajorVersion == 5;
	}

	void BootstrapPlatformSpecific(string path)
	{
		// Add runtime path and all module paths to PATH
		path = app->runtime->path + ";" + path;
		string currentPath = EnvironmentUtils::Get("PATH");
		if (!currentPath.empty())
		{
			path = path + ";" + currentPath;
		}
		EnvironmentUtils::Set("PATH", path);
	}

	string Blastoff()
	{
		// Windows boot does not normally need to restart itself,  so just
		// launch the host here and exit with the appropriate return value.

		// This may have been an install, so ensure that KR_HOME is correct
		EnvironmentUtils::Set("KR_HOME", app->path);
		exit(KrollBoot::StartHost());
	}
	
	bool SafeLoadRuntimeDLL(const char *name, HMODULE *module)
	{
		string runtimePath = EnvironmentUtils::Get("KR_RUNTIME");
		std::string dll = FileUtils::Join(runtimePath.c_str(), name, NULL);
		if (!FileUtils::IsFile(dll))
		{
			ShowError(string("Couldn't find required file: ") + dll);
			return false;
		}
		
		*module = LoadLibraryA(dll.c_str());
		if (!(*module))
		{
			char msg[MAX_PATH];
			sprintf_s(msg,"Couldn't load file: %s, error: %s",
				dll.c_str(), KrollUtils::Win32Utils::QuickFormatMessage(GetLastError()).c_str());

			ShowError(msg);
			return false;
		}
		
		return true;
	}
	
	typedef int Executor(HINSTANCE, int, const char **);
	int StartHost()
	{	
		// preload some of the troublesome common runtime DLLs
		for (int i = 0; i < preloadSize; i++)
		{
			HMODULE module;
			if (!SafeLoadRuntimeDLL(preload[i], &module)) return __LINE__;
		}
		
		HMODULE khost;	
		if (!SafeLoadRuntimeDLL("khost.dll", &khost)) return __LINE__;
		
		Executor *executor = (Executor*)GetProcAddress(khost, "Execute");
		if (!executor)
		{
			ShowError(string("Invalid entry point 'Execute' in khost.dll"));
			return __LINE__;
		}

		return executor(::GetModuleHandle(NULL), argc,(const char**)argv);
	}

	bool RunInstaller(vector<SharedDependency> missing, bool forceInstall)
	{
		string exec = FileUtils::Join(
			app->path.c_str(), "installer", "Installer.exe", NULL);
		if (!FileUtils::IsFile(exec))
		{
			ShowError("Missing installer and application has additional modules that are needed.");
			return false;
		}
		bool result = BootUtils::RunInstaller(missing, app, updateFile, "", false, forceInstall);

		// Ugh. Now we need to figure out where the app installer installed
		// to. We would normally use stdout, but we had to execute with
		// an expensive call to ShellExecuteEx, so we're just going to read
		// the information from a file.
		if (!app->IsInstalled())
		{
			string installedToFile = FileUtils::Join(app->GetDataPath().c_str(), ".installedto", NULL);
			// The user probably cancelled -- don't show an error
			if (!FileUtils::IsFile(installedToFile))
				return true;

			std::ifstream file(installedToFile.c_str());
			if (file.bad() || file.fail() || file.eof())
			{
				DeleteFileA(installedToFile.c_str());
				ShowError("Could not determine where installer installed application.");
				return false; // Don't show further errors
			}
			string appInstallPath;
			std::getline(file, appInstallPath);
			appInstallPath = FileUtils::Trim(appInstallPath);

			SharedApplication newapp = Application::NewApplication(appInstallPath);
			if (newapp.isNull())
			{
				return false; // Don't show further errors
			}
			else
			{
				app = newapp;
			}
			DeleteFileA(installedToFile.c_str());
		}
		return result;
	}

	string GetApplicationName()
	{
		if (!app.isNull())
		{
			return app->name.c_str();
		}
		return PRODUCT_NAME;
	}

#ifdef USE_BREAKPAD
	static google_breakpad::ExceptionHandler* breakpad;
	extern string dumpFilePath;

	char breakpadCallBuffer[MAX_PATH];
	bool HandleCrash(
		const wchar_t* dumpPath,
		const wchar_t* id,
		void* context,
		EXCEPTION_POINTERS* exinfo,
		MDRawAssertionInfo* assertion,
		bool succeeded)
	{
		if (succeeded)
		{
			STARTUPINFOA startupInfo = {0};
			startupInfo.cb = sizeof(startupInfo);
			PROCESS_INFORMATION processInformation;
			
			_snprintf_s(breakpadCallBuffer, MAX_PATH, MAX_PATH - 1,
				 "\"%s\" \"%s\" %S %S", argv[0], CRASH_REPORT_OPT, dumpPath, id);

			CreateProcessA(
				NULL,
				breakpadCallBuffer,
				NULL,
				NULL,
				FALSE,
				0,
				NULL,
				NULL,
				&startupInfo,
				&processInformation);
		}

		// We would not normally need to do this, but on Windows XP it
		// seems that this callback is called multiple times for a crash.
		// We should probably try to remove the following line the next
		// time we update breakpad.
		exit(__LINE__);
		return true;
	}

	wstring StringToWString(string in)
	{
		wstring out(in.length(), L' ');
		copy(in.begin(), in.end(), out.begin());
		return out; 
	}

	map<wstring, wstring> GetCrashReportParametersW()
	{
		map<wstring, wstring> paramsW;
		map<string, string> params = GetCrashReportParameters();
		map<string, string>::iterator i = params.begin();
		while (i != params.end())
		{
			wstring key = StringToWString(i->first);
			wstring val = StringToWString(i->second);
			i++;

			paramsW[key] = val;
		}
		return paramsW;
	}
	
	int SendCrashReport()
	{
		InitCrashDetection();
		std::string title = GetCrashDetectionTitle();
		std::string msg = GetCrashDetectionHeader();
		msg.append("\n\n");
		msg.append(GetCrashDetectionMessage());

		Win32PopupDialog popupDialog(NULL);
		popupDialog.SetTitle(title);
		popupDialog.SetMessage(msg);
		popupDialog.SetShowCancelButton(true);
		if (popupDialog.Show() != IDYES)
		{
			return __LINE__;
		}

		wstring url = L"https://";
		url += StringToWString(CRASH_REPORT_URL);

		const std::map<wstring, wstring> parameters = GetCrashReportParametersW();
		wstring dumpFilePathW = StringToWString(dumpFilePath);
		wstring responseBody;
		int responseCode;
		
		bool success = google_breakpad::HTTPUpload::SendRequest(
			url,
			parameters,
			dumpFilePathW.c_str(),
			L"dump",
			NULL,
			&responseBody,
			&responseCode);
	
		if (!success)
		{
#ifdef DEBUG		
			ShowError("Error uploading crash dump.");
#endif
			return __LINE__;
		}
#ifdef DEBUG
		else
		{
			MessageBoxW(NULL,L"Your crash report has been submitted. Thank You!",L"Error Reporting Status",MB_OK | MB_ICONINFORMATION);
		}
#endif
		return 0;
	}
#endif
}

#if defined(OS_WIN32) && !defined(WIN32_CONSOLE)
int WINAPI WinMain(HINSTANCE, HINSTANCE, LPSTR command_line, int)
#else
int main(int __argc, const char* __argv[])
#endif
{
	KrollBoot::argc = __argc;
	KrollBoot::argv = (const char**) __argv;

#ifdef USE_BREAKPAD
	// Don't install a handler if we are just handling an error.
	if (__argc > 2 && !strcmp(CRASH_REPORT_OPT, __argv[1]))
	{
		return KrollBoot::SendCrashReport();
	}

	wchar_t tempPath[MAX_PATH];
	GetTempPathW(MAX_PATH, tempPath);
	KrollBoot::breakpad = new google_breakpad::ExceptionHandler(
		tempPath,
		NULL,
		KrollBoot::HandleCrash,
		NULL,
		google_breakpad::ExceptionHandler::HANDLER_ALL);
#endif

	return KrollBoot::Bootstrap();
}
