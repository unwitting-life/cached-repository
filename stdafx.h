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
    int* result;
    utils::http::Proxy proxy;
    PARAM() :src(nullptr), body(nullptr), content_type(nullptr), result(nullptr) {}
};

inline DWORD WINAPI get_thread(LPVOID param) {
    auto p = (struct PARAM*)param;
    if (p && p->result && p->src && p->body && p->content_type && p->result) {
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
                *p->result = utils::http::status::Ok;
            }
        }
        OutputDebugString(debug.c_str());
        delete p;
        p = nullptr;
    }
    return 1;
}

inline std::tuple<std::wstring, std::string, std::string, int> get(std::wstring path, std::wstring physics_repository_directory_path, std::vector<repository>& repoServersList) {
    auto tuple = std::make_tuple(std::wstring(), std::string(), std::string(), utils::http::status::NotFound);
    std::get<0>(tuple) = physics_repository_directory_path + string_t(path).replace(L'/', PATH_SEPARATOR);
    if (string_t(std::get<0>(tuple)).to_path()->extension().empty()) {
        std::get<0>(tuple) = string_t(std::get<0>(tuple)).to_path()->combine(string_t(".%s").format(string_t(std::get<0>(tuple)).to_path()->name()));
    }
    if (!string_t(std::get<0>(tuple)).to_file()->exists()) {
        std::vector<HANDLE> threads;
        for (auto& e : repoServersList) {
            struct PARAM* param = new PARAM();
            param->url = e.uri + path;
            param->src = &std::get<0>(tuple);
            param->body = &std::get<1>(tuple);
            param->content_type = &std::get<2>(tuple);
            param->result = &std::get<3>(tuple);
            param->proxy = e.proxy;
            threads.push_back(CreateThread(nullptr, 0, get_thread, param, 0, nullptr));
        }
        for (auto& thread : threads) {
            WaitForSingleObject(thread, INFINITE);
            CloseHandle(thread);
        }
        if (!std::get<1>(tuple).empty()) {
            string_t(string_t(std::get<0>(tuple)).to_path()->directory()).to_directory()->mkdir(true);
            string_t(std::get<1>(tuple)).to_ofstream()->write(std::get<1>(tuple));
        }
    }
    if (string_t(std::get<0>(tuple)).to_file()->exists()) {
        auto fe = _wfopen(std::get<0>(tuple).c_str(), L"rb+");
        if (fe) {
            fseek(fe, 0, SEEK_END);
            auto size = ftell(fe);
            fseek(fe, 0, SEEK_SET);
            auto buffer = new char[size];
            fread(buffer, sizeof(buffer[0]), size, fe);
            std::get<1>(tuple) = std::string(buffer, size);
            delete[] buffer;
            fclose(fe);
        }
    }
    return tuple;
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
                if (string_t(file).to_file()->size() > 0) {
                    body = string_t(file).to_file()->content();
                } else {
                    std::vector<HANDLE> threads;
                    for (auto& e : repos) {
                        struct PARAM* param = new PARAM();
                        auto uri = std::wstring(req_path);
                        for (auto& p : prohibit) {
                            uri = string_t(uri).replace(p, std::wstring());
                        }
                        if (!uri.empty()) {
                            if (uri.front() != L'/') {
                                uri += L'/';
                            }
                            param->url = e.uri + uri;
                            param->src = &src;
                            param->body = &body;
                            param->content_type = &content_type;
                            param->result = &result;
                            param->proxy = e.proxy;
                            threads.push_back(CreateThread(nullptr, 0, get_thread, param, 0, nullptr));
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
                    sprintf(buffer, " -> %d bytes", (int)body.size());
                    std::vector<utils::system::console::text> v;
                    v.push_back(utils::system::console::text(req.remote_addr, FOREGROUND_GREEN | FOREGROUND_INTENSITY));
                    v.push_back(utils::system::console::text(L": "));
                    v.push_back(utils::system::console::text(file, FOREGROUND_GREEN | FOREGROUND_INTENSITY));
                    v.push_back(utils::system::console::text(buffer));
                    utils::system::console::println(v);
                }
            }
        } else {
            auto w = string_t(L"%s -> %s").format(string_t(req.method).c_str(), req_path.c_str());
            println(req, w, FOREGROUND_RED | FOREGROUND_INTENSITY);
        }
    }
}
