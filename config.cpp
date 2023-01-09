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
            mavenRepos.emplace_back(_T("https://repo1.maven.org/maven2"), _T("192.168.170.2"), 808);
            nodejsRepos.emplace_back(_T("https://registry.npmjs.org"), _T("192.168.170.2"), 808);

            mavenRepos.emplace_back(_T("http://143.94.16.36/repository/maven-central"));
            mavenRepos.emplace_back(_T("http://143.94.16.36/repository/maven-local"));
            mavenRepos.emplace_back(_T("http://143.94.16.36/repository/maven-public"));
            mavenRepos.emplace_back(_T("http://143.94.16.36/repository/maven-releases"));
            mavenRepos.emplace_back(_T("http://143.94.16.36/repository/maven-snapshots"));
            mavenRepos.emplace_back(_T("http://143.94.16.36/repository/AWMS-New"));
            mavenRepos.emplace_back(_T("http://143.94.16.36/repository/DAITO"));
            mavenExclude.emplace_back(_T("/com/fujifilm/fb/cod/service-arrangement-api/"));
            mavenExclude.emplace_back(_T("/com/fujifilm/fb/cod/cod-backend/"));
            mavenExclude.emplace_back(_T("/com/fujifilm/fb/cod/operation-core/"));
            mavenExclude.emplace_back(_T("/com/fujifilm/fb/cod/common/cod-common/"));
            mavenExclude.emplace_back(_T("/com/fujifilm/fb/cod/cms/cms/"));
            mavenExclude.emplace_back(_T("/com/fujifilm/fb/cod/cms/cod-cms/"));
            mavenExclude.emplace_back(_T("/com/fujifilm/fb/cod/cms/cms-api/"));
            mavenExclude.emplace_back(_T("/com/fujifilm/fb/cod/conversion/cod-conversion-service"));
            mavenExclude.emplace_back(_T("/com/fujifilm/fb/cod/conversion/cod-conversion/"));
            mavenExclude.emplace_back(_T("/com/fujifilm/fb/cod/cod-anonymous-web/"));
            mavenExclude.emplace_back(_T("/com/fujifilm/fb/cod/cod-frontend/"));
            mavenExclude.emplace_back(_T("/com/fujifilm/fb/cod/cod-box-web/"));
            mavenExclude.emplace_back(_T("/com/fujifilm/fb/cod/cod-core/"));
            mavenExclude.emplace_back(_T("/com/fujifilm/fb/cod/cod-core-api/"));
            mavenExclude.emplace_back(_T("/com/fujifilm/fb/cod/cod-mfp-api/"));
            mavenExclude.emplace_back(_T("/com/fujifilm/fb/cod/cod-onedrive-web/"));
            mavenExclude.emplace_back(_T("/com/fujifilm/fb/cod/cod-rest-api/"));
            mavenExclude.emplace_back(_T("/com/fujifilm/fb/cod/cod-user-web/"));
            mavenExclude.emplace_back(_T("/com/fujifilm/fb/cod/cod-mup/"));
            mavenExclude.emplace_back(_T("/com/fujifilm/fb/cod/cod-universal-print/"));
            mavenExclude.emplace_back(_T("/com/fujifilm/fb/cod/admin-core-api/"));
            mavenExclude.emplace_back(_T("/com/fujifilm/fb/cod/cod-web/"));
            mavenExclude.emplace_back(_T("/com/fujifilm/fb/cod/cod-common/"));
            mavenExclude.emplace_back(_T("/com/fujifilm/fb/cod/cms/"));
            mavenExclude.emplace_back(_T("/com/fujifilm/fb/cod/cms-api/"));

            mavenIgnorePrefix.emplace_back(_T("/nexus/content/repositories/ans-dependencies"));
            mavenIgnorePrefix.emplace_back(_T("/nexus/content/repositories/codehaus-snapshots"));
            mavenIgnorePrefix.emplace_back(_T("/maven2"));
            nodejsIgnorePrefix.emplace_back(_T("/nexus/content/groups/npm-all"));
        }
    }
}