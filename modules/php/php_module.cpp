/**
 * Appcelerator Kroll - licensed under the Apache Public License 2
 * see LICENSE in the root folder for details on the license.
 * Copyright (c) 2008 Appcelerator, Inc. All Rights Reserved.
 */
#include <signal.h>
#include "php_module.h"
#include <Poco/Path.h>

namespace kroll
{
    KROLL_MODULE(PhpModule, STRING(MODULE_NAME), STRING(MODULE_VERSION));

    PhpModule* PhpModule::instance_ = NULL;

    void PhpModule::Initialize()
    {
        PhpModule::instance_ = this;
    }

    void PhpModule::Stop()
    {
        PhpModule::instance_ = NULL;
    }

    void PhpModule::InitializeBinding()
    {
    }

    const static std::string php_suffix = "module.php";

    bool PhpModule::IsModule(std::string& path)
    {
        return (path.substr(path.length()-php_suffix.length()) == php_suffix);
    }

    Module* PhpModule::CreateModule(std::string& path)
    {
        // TODO: Open and load the file path with PHP

        Poco::Path p(path);
        std::string basename = p.getBaseName();
        std::string name = basename.substr(0,basename.length()-php_suffix.length()+4);
        std::string moduledir = path.substr(0,path.length()-basename.length()-4);

        Logger *logger = Logger::Get("PHP");
        logger->Info("Loading PHP path=%s", path.c_str());

        return new PhpModuleInstance(host, path, moduledir, name);
    }
}