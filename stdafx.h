#pragma once

#define CPPHTTPLIB_HTTPLIB_SUPPORT
#include "../cpp-utils/utils.hpp"

constexpr auto respoMutexName = _T("cached-repository");

struct REPOSITORY {
    std::string uri;
    utils::httplib::PROXY proxy;

    REPOSITORY(std::string uri) {
        this->uri = uri;
    }

    REPOSITORY(std::string uri, std::string proxyHost, int proxyPort) {
        this->uri = uri;
        this->proxy.host = proxyHost;
        this->proxy.port = proxyPort;
    }

    REPOSITORY(std::string uri, std::string proxy) {
        this->uri = uri;
        this->proxy.setProxy(proxy);
    }
};

inline void println(const httplib::Request& req, std::string text, WORD color, bool ssl) {
    std::vector<utils::console::CONSOLE_TEXT> v;
    if (ssl) {
        v.push_back(utils::console::CONSOLE_TEXT("["));
        v.push_back(utils::console::CONSOLE_TEXT("SSL", FOREGROUND_GREEN | FOREGROUND_INTENSITY));
        v.push_back(utils::console::CONSOLE_TEXT("]"));
    }
    v.push_back(utils::console::CONSOLE_TEXT(req.remote_addr.c_str(), FOREGROUND_GREEN | FOREGROUND_INTENSITY));
    v.push_back(utils::console::CONSOLE_TEXT(": "));
    v.push_back(utils::console::CONSOLE_TEXT(text, color));
    println(v);
}

struct PARAM {
    string_t url;
    string_t path_;
    string_t* resolvedUri;
    std::string* body;
    int* result;
    utils::httplib::PROXY* proxy;

    PARAM() {
        this->resolvedUri = nullptr;
        this->body = nullptr;
        this->result = nullptr;
        this->proxy = nullptr;
    }
};

inline DWORD WINAPI downloadThread(LPVOID param) {
    auto p = (struct PARAM*)param;
    if (p) {
        std::string debug = "TRY -> " + p->url;
        int err = 0;
        auto res = utils::httplib::Get(p->url, p->proxy, &err);
        if (res.empty() || err != utils::httplib::status::OK) {
            debug += " 404\n";
        } else {
            utils::threading::lock(respoMutexName);
            if (p->body->empty()) {
                *p->resolvedUri = p->url;
                *p->body = res;
                *p->result = utils::httplib::status::OK;
            }
            utils::threading::unlock(respoMutexName);
            debug += " 200\n";
        }
        OutputDebugString(debug.c_str());
        delete p;
        p = nullptr;
    }
    return 1;
}

inline int GetCacheFile(string_t path, string_t* resolvedUri, std::string* cacheFileBytes, string_t localRepositoryDirectory, std::vector<REPOSITORY>& repoServersList) {
    auto result = utils::httplib::status::NOT_FOUND;
    if (resolvedUri && cacheFileBytes) {
        auto localFile = localRepositoryDirectory + utils::strings::replace(path, "/", PATH_SEPARATOR);
        if (utils::io::directory::is_directory(localFile)) {
            localFile = utils::io::path::combine(localFile, utils::io::path::GetFileName(localFile));
        }
        if (utils::io::file::exists(localFile)) {
            *resolvedUri = localFile;
            auto fe = fopen(localFile.c_str(), "rb+");
            if (fe) {
                fseek(fe, 0, SEEK_END);
                auto size = ftell(fe);
                fseek(fe, 0, SEEK_SET);
                auto buffer = new char[size];
                fread(buffer, sizeof(buffer[0]), size, fe);
                *cacheFileBytes = std::string(buffer, size);
                delete[] buffer;
                fclose(fe);
            }
        } else {
            std::vector<HANDLE> threads;
            for (auto& e : repoServersList) {
                struct PARAM* param = new PARAM();
                param->url = e.uri + path;
                param->resolvedUri = resolvedUri;
                param->body = cacheFileBytes;
                param->result = &result;
                param->proxy = &e.proxy;
                threads.push_back(CreateThread(nullptr, 0, downloadThread, param, 0, nullptr));
                if (threads.size() == 1) {
                    WaitForSingleObject(threads.at(0), INFINITE);
                    utils::threading::lock(respoMutexName);
                    if (*param->result == utils::httplib::status::OK) {
                        break;
                    }
                    utils::threading::unlock(respoMutexName);
                }
            }
            for (auto& thread : threads) {
                WaitForSingleObject(thread, INFINITE);
                CloseHandle(thread);
            }
        }
    }
    return result;
}

inline void fetch(const httplib::Request& req, httplib::Response& res, bool ssl,
    std::vector<string_t> ignorePrefix, string_t localDirectory, std::vector<REPOSITORY> repos,
    std::vector<string_t> exclude) {
    if (res.status == utils::httplib::status::NOT_FOUND) {
        auto file = utils::io::path::GetExeFileDirectory();
        file = utils::io::path::combine(file, _T("wwwroot"));
        file = utils::io::path::combine(file, utils::strings::t2t(utils::httplib::GetUrl(req.path)));
        if (utils::strings::upper(req.method) == utils::httplib::method::GET) {
            if (utils::io::file::exists(file)) {
                res.status = utils::httplib::status::OK;
                res.set_content(utils::io::file::read(file), utils::httplib::mime::GetMimeType(file));
            } else if (req.path == "/") {
                auto now = utils::datetime::now("%04d%02d%02d%02d%02d%02d%03d");
                res.status = utils::httplib::status::TEMPORARILY_MOVED;
                res.set_header(utils::httplib::header::Location,
                    utils::strings::formatA("/index/CACHED-REPOSITORY.html?%s", utils::datetime::now("%04d/%02d/%02d %02d:%02d:%02d.%03d").c_str()));
            } else {
                auto url = utils::strings::t2t(req.path);
                for (auto& e : ignorePrefix) {
                    url = utils::strings::replace(url, e, _T(""));
                }
                std::string fileBytes;
                string_t resolvedUri;
                string_t debug = req.method + " -> " + url + "\n";
                OutputDebugString(debug.c_str());

                auto result = utils::httplib::status::NOT_FOUND;
                bool isExclude = false;
                string_t lower = utils::strings::lower(url);
                for (auto& e : exclude) {
                    if (lower.find(e) != std::string::npos) {
                        isExclude = true;
                        break;
                    }
                }
                if (isExclude) {
                    println(req, (url + " -> skip").c_str(), FOREGROUND_INTENSITY, ssl);
                } else {
                    result = GetCacheFile(url, &resolvedUri, &fileBytes, localDirectory, repos);
                    if (fileBytes.empty()) {
                        println(req, url.c_str(), FOREGROUND_RED | FOREGROUND_INTENSITY, ssl);
                    } else {
                        auto relativeFilePath = localDirectory + utils::strings::replace(url, '/', PATH_SEPARATOR);
                        if (utils::io::path::GetDirectoryName(url).empty()) {
                            relativeFilePath = utils::io::path::combine(relativeFilePath, utils::io::path::GetFileName(relativeFilePath));
                        }
                        auto directoryPath = utils::io::path::GetDirectoryPath(relativeFilePath);
                        utils::threading::lock(respoMutexName);
                        if (utils::io::file::exists(directoryPath)) {
                            utils::io::remove(directoryPath);
                        }
                        utils::io::file::write(relativeFilePath, fileBytes);
                        utils::threading::unlock(respoMutexName);

                        auto mimeType = "application/octet-stream";
                        if (resolvedUri.find(".pom") != std::string::npos ||
                            resolvedUri.find(".xml") != std::string::npos) {
                            mimeType = "text/xml";
                        }
                        res.set_content(fileBytes, mimeType);
                        res.status = utils::httplib::status::OK;

                        char buffer[1000] = { 0 };
                        sprintf(buffer, " -> %d bytes", (int)fileBytes.size());
                        std::vector<utils::console::CONSOLE_TEXT> v;
                        if (ssl) {
                            v.push_back(utils::console::CONSOLE_TEXT("["));
                            v.push_back(utils::console::CONSOLE_TEXT("SSL", FOREGROUND_GREEN | FOREGROUND_INTENSITY));
                            v.push_back(utils::console::CONSOLE_TEXT("]"));
                        }
                        v.push_back(utils::console::CONSOLE_TEXT(req.remote_addr.c_str(), FOREGROUND_GREEN | FOREGROUND_INTENSITY));
                        v.push_back(utils::console::CONSOLE_TEXT(": "));
                        v.push_back(utils::console::CONSOLE_TEXT(resolvedUri, FOREGROUND_GREEN | FOREGROUND_INTENSITY));
                        v.push_back(utils::console::CONSOLE_TEXT(buffer));
                        println(v);
                    }
                }
            }
        } 
    }
}
