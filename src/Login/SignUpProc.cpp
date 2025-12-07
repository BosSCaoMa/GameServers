#include "SignUpProc.h"
#include "QueryUserData.h"
#include "SafetyPwd.h"
#include "http_response.h"
using namespace std;

bool VerifyInvCode(const std::string& invCode)
{
    // 简单的邀请码验证逻辑
    // 实际项目中可能需要查询数据库或其他验证方式
    return invCode == "test123";
}

bool ProcSignUpRequest(const HttpRequest &request, std::shared_ptr<Client> client)
{
    int client_fd = client->getFd();
    string username = request.getParam("username");
    string password = request.getParam("password");
    string InvCode = request.getParam("invCode");

    if (!VerifyInvCode(InvCode)) {
        // 发送失败响应给客户端
        send_json_response(client_fd, 401, {{"error", "Invalid invitation code"}}, false);
        return false; // 认证失败，连接应该关闭
    }

    if (IsUserExists(username)) {
        // 发送失败响应给客户端
        send_json_response(client_fd, 409, {{"error", "User already exists"}}, false);
        return false; // 认证失败，连接应该关闭
    }

    if (!InsertUserInfo(username, hashPassword(password), InvCode)) {
        // 发送失败响应给客户端
        send_json_response(client_fd, 500, {{"error", "Database error"}}, false);
        return false; // 认证失败，连接应该关闭
    }
    return SUCCESS;
}