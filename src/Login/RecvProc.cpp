#include "RecvProc.h"
#include "LogM.h"
#include <sys/socket.h> // socket
#include <netinet/in.h> // sockaddr_in，AF_INET 常量
#include <stdio.h>      // 基础输入输出（如 perror 打印错误信息）
#include <stdlib.h>     // 标准库（如 exit 函数
#include "json.hpp"

#include "SafetyPwd.h"

using namespace std;
void ProcLoginRequest(const HttpRequest& request, int client_fd)
{
    string username = request.getParam("username");
    string password = request.getParam("password");
    
    if (verifyPassword(password, queryUserPwd(username))) {

    }

    // 发送失败响应给客户端
    
}

void ProcRegisterRequest(const HttpRequest& request, int client_fd)
{

}

void handle_client(int client_fd)
{
    LOG_DEBUG("Handling new client: fd=%d", client_fd);

    std::string raw;
    raw.resize(8192);
    int n = read(client_fd, &raw[0], raw.size() - 1);
    if (n <= 0) {
        LOG_ERROR("Read from client failed or connection closed");
        close(client_fd);
        return;
    }
    raw.resize(n);

    HttpRequest request(raw);
    if (!request.isValid()) {
        LOG_ERROR("Invalid HTTP request");
        close(client_fd);
        return;
    }

    // 处理请求
    if (request.getPath() == "/api/login") {
        ProcLoginRequest(request, client_fd);
    } else if (request.getPath() == "/api/register") {
        ProcRegisterRequest(request, client_fd);
    } else {
        LOG_ERROR("Unknown API endpoint: %s", request.getPath().c_str());
    }

    // close(client_fd); webserver 可能会继续使用连接
}

int ProcLoginReq(int port)
{
    int server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd < 0) {
        LOG_ERROR("Socket creation failed");
        return 1;
    }

    int opt = 1;
    setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
    sockaddr_in addr{};
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = inet_addr("127.0.0.1");
    addr.sin_port = htons(port);

    if (bind(server_fd, (struct sockaddr*)&addr, sizeof(addr)) < 0) {
        LOG_ERROR("Bind failed");
        close(server_fd);
        return 1;
    }

    if (listen(server_fd, 16) < 0) {
        perror("listen");
        close(server_fd);
        return 1;
    }

    LOG_DEBUG("Server listening on port %d", port);

    while (true) {
        sockaddr_in client_addr{};
        socklen_t client_len = sizeof(client_addr);
        int client_fd = accept(server_fd, (struct sockaddr*)&client_addr, &client_len);
        if (client_fd < 0) {
            LOG_ERROR("Accept failed");
            continue;
        }
        std::thread t(parase, client_fd);
        t.detach();
    }
    close(server_fd);
    return 0;
}