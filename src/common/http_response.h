#ifndef HTTP_RESPONSE_H
#define HTTP_RESPONSE_H

#include <string>
#include <vector>
#include <utility>
#include <unordered_map>
#include <unistd.h>     // write() / close()
#include <errno.h>

#include <nlohmann/json.hpp>

/*
    sendMsg()
    ↓
    write_all()
    ↓
    可能写不完 → sendBuffer → 监听 EPOLLOUT
    ↓
    epoll 通知 EPOLLOUT
    ↓
    handleWrite()
    ↓
    sendBuffer==空 → 取消 EPOLLOUT
*/
using json = nlohmann::json;
inline const std::string& status_text(int code)
{
    static const std::unordered_map<int, std::string> kStatusText = {
        {200, "OK"},
        {201, "Created"},
        {204, "No Content"},
        {400, "Bad Request"},
        {401, "Unauthorized"},
        {403, "Forbidden"},
        {404, "Not Found"},
        {500, "Internal Server Error"},
        {502, "Bad Gateway"},
        {503, "Service Unavailable"},
    };
    auto it = kStatusText.find(code);
    static const std::string kUnknown = "Unknown";
    return (it != kStatusText.end()) ? it->second : kUnknown;
}

inline bool write_all(int fd, const char* data, size_t len)
{
    while (len > 0) {
        ssize_t n = ::write(fd, data, len);
        if (n > 0) {
            data += n;
            len  -= static_cast<size_t>(n);
            continue;
        }
        if (n == -1 && errno == EINTR) {
            continue; // 被信号打断，重试
        }
        // 非阻塞 socket + EAGAIN/EWOULDBLOCK 的完整处理
        // 一般要集成到你的事件循环，这里简单返回 false
        return false;
    }
    return true;
}

inline bool send_json_response(int client_fd, int statusCode, const json& bodyJson, bool keepAlive = true,
    const std::vector<std::pair<std::string, std::string>>& extraHeaders = {})
{
    // 1. 序列化 JSON
    std::string body = bodyJson.dump();  // 如需美化，可 dump(4)

    // 2. 预估响应大小，减少 realloc
    std::string resp;
    resp.reserve(128 + body.size());

    // 3. 起始行
    resp += "HTTP/1.1 ";
    resp += std::to_string(statusCode);
    resp += " ";
    resp += status_text(statusCode);
    resp += "\r\n";

    // 4. 必要头
    resp += "Content-Type: application/json; charset=utf-8\r\n";
    resp += "Content-Length: ";
    resp += std::to_string(body.size());
    resp += "\r\n";
    resp += "Connection: ";
    resp += (keepAlive ? "keep-alive" : "close");
    resp += "\r\n";

    // 5. 自定义头
    for (const auto& kv : extraHeaders) {
        resp += kv.first;
        resp += ": ";
        resp += kv.second;
        resp += "\r\n";
    }

    // 6. 头结束 + body
    resp += "\r\n";
    resp += body;

    return write_all(client_fd, resp.data(), resp.size());
}

#endif // HTTP_RESPONSE_H