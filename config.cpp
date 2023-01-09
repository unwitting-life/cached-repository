#pragma once
#include "config.h"

auto mavenDirectory = utils::io::path::GetWorkingDirectory() + "\\repos\\.m2";
std::vector<REPOSITORY> mavenRepos;
std::vector<string_t> mavenExclude;
std::vector<string_t> mavenIgnorePrefix;

auto nodejsDirectory = utils::io::path::GetWorkingDirectory() + "\\repos\\nodejs";
std::vector<REPOSITORY> nodejsRepos;
std::vector<string_t> nodejsExclude;
std::vector<string_t> nodejsIgnorePrefix;

namespace config {
    void init() {
        static auto b = false;
        if (!b) {
            b = true;
            mavenRepos.emplace_back(_T("https://repo1.maven.org/maven2"));
            nodejsRepos.emplace_back(_T("https://registry.npmjs.org"));
        }
    }
}