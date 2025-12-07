#include "RecvProc.h"
#include "LogM.h"
#include <sys/socket.h>  // socket
#include <netinet/in.h>  // sockaddr_in，AF_INET 常量
#include <arpa/inet.h>   // inet_addr
#include <unistd.h>      // close, read
#include <stdio.h>       // 基础输入输出（如 perror 打印错误信息）
#include <stdlib.h>      // 标准库（如 exit 函数
#include "json.hpp"
#include <memory>
#include <thread>
#include <errno.h>       // errno

#include "SafetyPwd.h"
#include "Client.h"
#include "parseHttp.h"
#include "LoginProc.h"
#include "SignUpProc.h"
using namespace std;


void handle_client(std::shared_ptr<Client> client)
{
    int client_fd = client->getFd();
    LOG_DEBUG("Handling new client: fd=%d", client_fd);

    std::string raw;
    raw.resize(8192);
    int n = read(client_fd, &raw[0], raw.size() - 1);
    if (n <= 0) {
        LOG_ERROR("Read from client failed or connection closed");
        return;
    }
    raw.resize(n);

    HttpRequest request(raw);
    if (!request.isValid()) {
        LOG_ERROR("Invalid HTTP request");
        return;
    }

    // 处理请求
    bool keepConnection = false;
    if (request.getPath() == "/api/login") {
        keepConnection = ProcLoginRequest(request, client);
    } else if (request.getPath() == "/api/register") {
        keepConnection = ProcSignUpRequest(request, client);
    } else {
        LOG_ERROR("Unknown API endpoint: %s", request.getPath().c_str());
    }
    
    /* 生命周期管理说明：
     * - keepConnection=true: EventLoop持有client的shared_ptr，连接保持打开
     * - keepConnection=false: 只有本函数持有client，函数结束后引用计数归0，
     *   Client析构函数会自动close(fd)，连接关闭
     */
    if (keepConnection) {
        LOG_DEBUG("Connection fd=%d transferred to EventLoop, keep-alive", client_fd);
        // client的shared_ptr被EventLoop持有，不会被关闭
    } else {
        LOG_DEBUG("Connection fd=%d will be closed on function exit", client_fd);
        // 函数结束时client引用计数归0，析构函数关闭连接
    }
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
        auto client = std::make_shared<Client>(client_fd); // RAII 管理客户端连接
        std::thread t(handle_client, client);
        t.detach();
    }
    close(server_fd);
    return 0;
}