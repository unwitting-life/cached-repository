#pragma warning(disable : 4996)
#pragma warning(disable : 6387)

#include "stdafx.h"
#include "config.h"

CRITICAL_SECTION criticalSection = { 0 };

extern std::wstring maven_directory;
extern std::wstring nodejs_directory;
extern std::vector<repository> maven_repositories;
extern std::vector<repository> nodejs_repositories;

constexpr auto MAVEN_HTTP_PORT = (8081);
constexpr auto MAVEN_SSL_PORT = (443);
constexpr auto NODEJS_PORT = (8082);

int main() {
    InitializeCriticalSection(&criticalSection);
    for (auto& e : string_t(maven_directory).to_directory()->files()) {
        sha1_jar(e);
        md5_jar(e);
    }
    system("cls");
    printf("\033[0m****************************************************************\n");
    printf("\033[0m* URL    : https://github.com/unwitting-life/cached-repository *\n");
    printf("\033[0m* License: MIT                                                 *\n");
    printf("\033[0m* Author : Yuanlei Huang                                       *\n");
    printf("\033[0m* Latest : 2023/06/02                                          *\n");
    printf("\033[0m****************************************************************\n");
    CloseHandle(CreateThread(nullptr, 0, [](LPVOID) -> DWORD {
        utils::system::console::println(string_t(L"[MAVEN] Listen on 0.0.0.0:%d").format(MAVEN_HTTP_PORT), FOREGROUND_GREEN);
        httplib::Server server;
        server.Get(".*", [&](const httplib::Request& req, httplib::Response& res) {
            res.status = 404;
            invoke(req, res, maven_directory, maven_repositories, prohibit);
        });
        server.listen("0.0.0.0", MAVEN_HTTP_PORT);
        return 0;
    }, nullptr, 0, nullptr));
    Sleep(100);
    CloseHandle(CreateThread(nullptr, 0, [](LPVOID) -> DWORD {
        utils::system::console::println(string_t(L"[MAVEN] Listen on 0.0.0.0:%d").format(MAVEN_SSL_PORT), FOREGROUND_GREEN);
        auto certFilePath = string_t(utils::system::io::path::executable_file_directory()).to_path()->combine(L"certificate.crt");
        auto keyFilePath = string_t(utils::system::io::path::executable_file_directory()).to_path()->combine(L"private.pem");
        if (!string_t(certFilePath).to_file()->exists()) {
            utils::http::httplib::create_new_certificate(true);
            certFilePath = string_t(utils::system::io::path::executable_file_directory()).to_path()->combine(L"localhost.crt");
            keyFilePath = string_t(utils::system::io::path::executable_file_directory()).to_path()->combine(L"localhost.key");
        }
        httplib::SSLServer sslServer(string_t(certFilePath).s().c_str(), string_t(keyFilePath).s().c_str());
        sslServer.Get(".*", [&](const httplib::Request& req, httplib::Response& res) {
            res.status = 404;
            invoke(req, res, maven_directory, maven_repositories, prohibit);
        });
        sslServer.listen("0.0.0.0", MAVEN_SSL_PORT);
        return 0;
        }, nullptr, 0, nullptr));

    Sleep(100);
    CloseHandle(CreateThread(nullptr, 0, [](LPVOID) -> DWORD {
        utils::system::console::println(string_t(L"[NODEJS] Listen on 0.0.0.0:%d").format(NODEJS_PORT), FOREGROUND_GREEN);
        httplib::Server server;
        server.Get(".*", [&](const httplib::Request& req, httplib::Response& res) {
            res.status = 404;
            invoke(req, res, nodejs_directory, nodejs_repositories, prohibit);
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