#pragma once
#include "config.h"

auto mavenDirectory = utils::io::path::GetWorkingDirectory() + "\\repos\\.m2";
std::vector<std::string> mavenRepos;
std::vector<std::string> mavenExclude;
std::vector<std::string> mavenIgnorePrefix;

auto nodejsDirectory = utils::io::path::GetWorkingDirectory() + "\\repos\\nodejs";
std::vector<std::string> nodejsRepos;
std::vector<std::string> nodejsExclude;
std::vector<std::string> nodejsIgnorePrefix;

namespace config {
    void init() {
        static auto b = false;
        if (!b) {
            b = true;
            mavenRepos.push_back("https://repo1.maven.org");
            nodejsRepos.push_back("https://registry.npmjs.org");
        }
    }
}