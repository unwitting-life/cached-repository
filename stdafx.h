#pragma once

#include "../cpp-utils/utils.hpp"

constexpr auto mutex = _T("cached-repository");

struct repository {
    std::wstring uri;
    utils::http::Proxy proxy;

    repository(std::wstring uri) {
        this->uri = uri;
    }

    repository(std::wstring uri, std::wstring host, int port) {
        this->uri = uri;
        this->proxy.host = string_t(host).s();
        this->proxy.port = port;
    }

    repository(std::wstring uri, std::wstring proxy) {
        this->uri = uri;
        this->proxy.parse(proxy);
    }
};

inline void println(const httplib::Request req, std::wstring text, WORD color) {
    std::vector<utils::system::console::text> v;
    v.push_back(utils::system::console::text(string_t(req.remote_addr), FOREGROUND_GREEN | FOREGROUND_INTENSITY));
    v.push_back(utils::system::console::text(L": "));
    v.push_back(utils::system::console::text(text, color));
    utils::system::console::println(v);
}

struct PARAM {
    std::wstring url;
    std::wstring path;
    std::wstring* src;
    std::string* body;
    std::string* content_type;
    std::wstring* proxy_host;
    int* result;
    utils::http::Proxy proxy;
    PARAM() :src(nullptr), body(nullptr), content_type(nullptr), proxy_host(nullptr), result(nullptr) {}
};

void sha1_jar(std::wstring file) {
    auto jar = string_t(file).to_path();
    if (string_t(jar->extension()).lower() == L".jar" ||
        string_t(jar->extension()).lower() == L".pom") {
        auto sha1_file = string_t(string_t(jar->directory()).to_path()->combine(string_t(string_t(L"%s.sha1").format(jar->name().c_str())))).to_ofstream(true);
        if (sha1_file->size() != 40) {
            auto sha1 = string_t(string_t(jar->c_str()).to_file()->sha1()).lower();
            sha1_file->write(string_t(sha1).s());
            std::vector<utils::system::console::text> v;
            v.push_back(utils::system::console::text(L"["));
            v.push_back(utils::system::console::text(L"SHA1", FOREGROUND_GREEN | FOREGROUND_BLUE));
            v.push_back(utils::system::console::text(L"]"));
            v.push_back(utils::system::console::text(sha1_file->c_str(), FOREGROUND_GREEN | FOREGROUND_INTENSITY));
            v.push_back(utils::system::console::text(L" -> "));
            v.push_back(utils::system::console::text(sha1, FOREGROUND_GREEN | FOREGROUND_INTENSITY));
            utils::system::console::println(v);
        }
    }
}

void md5_jar(std::wstring file) {
    auto jar = string_t(file).to_path();
    if (string_t(jar->extension()).lower() == L".jar" ||
        string_t(jar->extension()).lower() == L".pom") {
        auto md5_file = string_t(string_t(jar->directory()).to_path()->combine(string_t(string_t(L"%s.md5").format(jar->name().c_str())))).to_ofstream(true);
        if (md5_file->size() != 32) {
            auto md5 = string_t(string_t(jar->c_str()).to_file()->md5()).lower();
            md5_file->write(string_t(md5).s());
            std::vector<utils::system::console::text> v;
            v.push_back(utils::system::console::text(L"["));
            v.push_back(utils::system::console::text(L"MD5", FOREGROUND_GREEN | FOREGROUND_BLUE));
            v.push_back(utils::system::console::text(L"]"));
            v.push_back(utils::system::console::text(md5_file->c_str(), FOREGROUND_GREEN | FOREGROUND_INTENSITY));
            v.push_back(utils::system::console::text(L" -> "));
            v.push_back(utils::system::console::text(md5, FOREGROUND_GREEN | FOREGROUND_INTENSITY));
            utils::system::console::println(v);
        }
    }
}

inline DWORD WINAPI get_thread(LPVOID param) {
    auto p = (struct PARAM*)param;
    if (p && p->result && p->src && p->body && p->content_type && p->proxy_host && p->result) {
        if (p->proxy.host.empty()) {
            OutputDebugString(string_t(L"TRY -> %s ...\n").format(p->url).c_str());
        } else {
            OutputDebugString(string_t(L"TRY -> %s via %s ...\n").format(p->url, string_t(p->proxy.host)).c_str());
        }
        auto debug = string_t(L"TRY -> ") + string_t( p->url);
        int err = 0;
        auto res = string_t(p->url).to_curl(string_t(L"%s:%d").format(string_t(p->proxy.host).c_str(), p->proxy.port))->get();
        if (res.status_code == utils::http::status::NotFound) {
            debug += L" 404\n";
        } else if (res.body.empty()) {
            debug += L" NO_CONTENT\n";
        } else {
            debug += L" 200\n";
            synchronsize(mutex);
            if (*p->result != utils::http::status::Ok) {
                *p->src = p->url;
                *p->body = res.body;
                for (auto& e : res.headers) {
                    if (e.starts_with(utils::http::header::ContentType)) {
                        *p->content_type = e.substr(strlen(utils::http::header::ContentType) + 1);
                        break;
                    }
                }
                *p->proxy_host = string_t(L"%s:%d").format(string_t(p->proxy.host), p->proxy.port);
                *p->result = utils::http::status::Ok;
            }
        }
        OutputDebugString(debug.c_str());
        delete p;
        p = nullptr;
    }
    return 1;
}

inline void invoke(const httplib::Request& req, 
    httplib::Response& res,
    std::wstring physics_repository_directory_path, 
    std::vector<repository>& repos, 
    std::vector<std::wstring>& prohibit) {
    if (res.status == utils::http::status::NotFound) {
        auto req_path = string_t(req.path);
        auto file = utils::system::io::path::executable_file_directory();
        file = string_t(file).to_path()->combine(_T("wwwroot"));
        file = string_t(file).to_path()->combine(req_path.to_http()->trunk());
        if (string_t(string_t(req.method).upper()).s() == utils::http::method::GET ||
            string_t(string_t(req.method).upper()).s() == utils::http::method::HEAD) {
            if (string_t(file).to_file()->exists()) {
                res.status = utils::http::status::Ok;
                res.set_content(string_t(file).to_file()->content(), utils::http::mime::get(file));
            } else if (req.path == "/") {
                auto now = utils::system::datetime::now(L"%04d%02d%02d%02d%02d%02d%03d");
                res.status = utils::http::status::TemporarilyMoved;

                auto location = string_t(L"/index/CACHED-Repository.html?%s").format(utils::system::datetime::now(L"%04d%02d%02d%02d%02d%02d%03d").c_str());
                res.set_header(utils::http::header::Location, string_t(location).s());
            } else {
                file = physics_repository_directory_path + string_t(req_path).replace(L'/', PATH_SEPARATOR);
                if (string_t(file).to_path()->extension().empty()) {
                    file = string_t(file).to_path()->combine(string_t(".%s").format(string_t(file).to_path()->name()));
                }
                OutputDebugString(string_t(L"%s -> %s\n").format(string_t(req.method), req_path).c_str());

                auto file_info = file + L".info";
                auto src = std::wstring();
                auto body = std::string();
                auto content_type = std::string();
                auto result = int(0);
                auto proxy_host = std::wstring();
                if (string_t(file).to_file()->size() > 0) {
                    body = string_t(file).to_file()->content();
                } else {
                    std::vector<HANDLE> threads;
                    for (auto& e : repos) {
                        struct PARAM* param = nullptr;
                        auto uri = std::wstring(req_path);
                        auto init_param = [&]() {
                            param = new PARAM();
                            param->url = e.uri + uri;
                            param->src = &src;
                            param->body = &body;
                            param->content_type = &content_type;
                            param->result = &result;
                            param->proxy = e.proxy;
                            param->proxy_host = &proxy_host;
                        };
                        for (auto& p : prohibit) {
                            uri = string_t(uri).replace(p, std::wstring());
                        }
                        if (!uri.empty()) {
                            if (uri.front() != L'/') {
                                uri += L'/';
                            }
                            init_param();
                            threads.push_back(CreateThread(nullptr, 0, get_thread, param, 0, nullptr));
                        }
                        if (threads.size() == 1) {
                            auto ready = false;
                            auto retry = 3;
                            while (!ready && retry-- > 0) {
                                WaitForSingleObject(threads[0], INFINITE);
                                if (result == utils::http::status::Ok && !body.empty()) {
                                    ready = true;
                                } else if (result == utils::http::status::None && body.empty()) {
                                    threads.clear();
                                    init_param();
                                    threads.push_back(CreateThread(nullptr, 0, get_thread, param, 0, nullptr));
                                } else {
                                    retry = 0;
                                }
                            }
                            if (ready) {
                                break;
                            }
                        }
                    }
                    for (auto& thread : threads) {
                        WaitForSingleObject(thread, INFINITE);
                        CloseHandle(thread);
                    }
                    if (!body.empty()) {
                        string_t(string_t(file).to_path()->directory()).to_directory()->mkdir(true);
                        string_t(file).to_ofstream()->write(body);
                        string_t(file_info).to_ofstream()->write(content_type);
                        sha1_jar(file);
                        md5_jar(file);
                    }
                } 
                if (body.empty()) {
                    println(req, req_path, FOREGROUND_RED | FOREGROUND_INTENSITY);
                } else {
                    if (content_type.empty()) {
                        content_type = string_t(file_info).to_file()->content();
                    }
                    auto mime_type = content_type.empty() ? "application/octet-stream" : content_type.c_str();
                    if (file.ends_with(L".pom")||
                        file.ends_with(L".xml")) {
                        mime_type = "text/xml";
                    }
                    res.set_content(body, mime_type);
                    res.status = utils::http::status::Ok;

                    char buffer[1000] = { 0 };
                    sprintf(buffer, "%d bytes", (int)body.size());
                    std::vector<utils::system::console::text> v;
                    v.push_back(utils::system::console::text(L"["));
                    v.push_back(utils::system::console::text(req.remote_addr, FOREGROUND_GREEN | FOREGROUND_BLUE));
                    v.push_back(utils::system::console::text(L"] "));
                    if (src.empty()) {
                        v.push_back(utils::system::console::text(file, FOREGROUND_GRAY));
                    } else {
                        v.push_back(utils::system::console::text(src, FOREGROUND_GREEN | FOREGROUND_INTENSITY));
                        if (!proxy_host.empty()) {
                            v.push_back(utils::system::console::text(L" via ", FOREGROUND_GREEN | FOREGROUND_BLUE));
                            v.push_back(utils::system::console::text(proxy_host, FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE));
                        }
                    }
                    v.push_back(utils::system::console::text(L" -> "));
                    v.push_back(utils::system::console::text(buffer, FOREGROUND_GREEN | FOREGROUND_INTENSITY));
                    utils::system::console::println(v);
                }
            }
        } else {
            auto w = string_t(L"%s -> %s").format(string_t(req.method).c_str(), req_path.c_str());
            println(req, w, FOREGROUND_RED | FOREGROUND_INTENSITY);
        }
    }
}
