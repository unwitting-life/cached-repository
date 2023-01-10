#pragma warning(disable : 4996)

#include "stdafx.h"
#include "config.h"

CRITICAL_SECTION criticalSection = { 0 };

extern string_t mavenDirectory;
extern std::vector<REPOSITORY> mavenRepos;
extern std::vector<std::string> mavenExclude;
extern std::vector<std::string> mavenIgnorePrefix;

extern string_t nodejsDirectory;
extern std::vector<REPOSITORY> nodejsRepos;
extern std::vector<std::string> nodejsExclude;
extern std::vector<std::string> nodejsIgnorePrefix;

#define MAVEN_HTTP_PORT (80)
#define MAVEN_TLS_PORT (443)
#define NODEJS_PORT (8081)

int main() {
    utils::httplib::CreateCertificate();
    config::init();
    InitializeCriticalSection(&criticalSection);

    CloseHandle(CreateThread(nullptr, 0, [](LPVOID) -> DWORD {
        utils::console::println(utils::strings::format(_T("[MAVEN] Listen on 0.0.0.0:%d"), MAVEN_HTTP_PORT), FOREGROUND_GREEN);
        httplib::Server server;
        server.Get(".*", [&](const httplib::Request& req, httplib::Response& res) {
            res.status = 404;
            fetch(req, res, false, mavenIgnorePrefix, mavenDirectory, mavenRepos, mavenExclude);
        });
        server.listen("0.0.0.0", MAVEN_HTTP_PORT);
        return 0;
    }, nullptr, 0, nullptr));

    // https://stackoverflow.com/questions/3810058/read-certificate-files-from-memory-instead-of-a-file-using-openssl
    Sleep(100);
    CloseHandle(CreateThread(nullptr, 0, [](LPVOID) -> DWORD {
        utils::console::println(utils::strings::format(_T("[MAVEN] Listen on 0.0.0.0:%d"), MAVEN_TLS_PORT), FOREGROUND_GREEN);
        httplib::SSLServer sslServer(
            utils::io::path::combine(utils::io::path::GetExeFileDirectory(), "localhost.crt").c_str(),
            utils::io::path::combine(utils::io::path::GetExeFileDirectory(), "localhost.key").c_str());
        sslServer.Get(".*", [&](const httplib::Request& req, httplib::Response& res) {
            res.status = 404;
            fetch(req, res, true, mavenIgnorePrefix, mavenDirectory, mavenRepos, mavenExclude);
        });
        sslServer.listen("0.0.0.0", MAVEN_TLS_PORT);
        return 0;
        }, nullptr, 0, nullptr));

    Sleep(100);
    CloseHandle(CreateThread(nullptr, 0, [](LPVOID) -> DWORD {
        utils::console::println(utils::strings::format(_T("[MAVEN] Listen on 0.0.0.0:%d"), NODEJS_PORT), FOREGROUND_GREEN);
        httplib::Server server;
        server.Get(".*", [&](const httplib::Request& req, httplib::Response& res) {
            res.status = 404;
            fetch(req, res, false, nodejsIgnorePrefix, nodejsDirectory, nodejsRepos, nodejsExclude);
        });
        server.listen("0.0.0.0", NODEJS_PORT);
        return 0;
    }, nullptr, 0, nullptr));
    
    while (true) {
        auto _ = _getch();
    }

    return 0;
}

LPCRITICAL_SECTION GetCriticalSection() {
    return &criticalSection;
}