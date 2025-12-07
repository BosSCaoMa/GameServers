#ifndef HTTP_RESPONSE_H
#define HTTP_RESPONSE_H

#include <string>
#include <vector>
#include <utility>
#include <unordered_map>
#include <unistd.h>     // write() / close()
#include <errno.h>

#include <json.hpp>
 
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
inline const std::string& status_text(int code);

inline bool send_json_response(int client_fd, int statusCode, const json& bodyJson, bool keepAlive = true,
    const std::vector<std::pair<std::string, std::string>>& extraHeaders = {});

#endif // HTTP_RESPONSE_H