#include "SignUpProc.h"
#include "QueryUserData.h"
#include "SafetyPwd.h"
using namespace std;


bool ProcSignUpRequest(const HttpRequest &request, std::shared_ptr<Client> client)
{
    int client_fd = client->getFd();
    string username = request.getParam("username");
    string password = request.getParam("password");
    string InvCode = request.getParam("invCode");

    if (!VerifyInvCode(InvCode)) {
        // 发送失败响应给客户端
        send_json_response(client_fd, 401, {{"error", "Invalid invitation code"}}, false);
        return INVCODE_INVALID; // 认证失败，连接应该关闭
    }

    if (isUserExist(username)) {
        // 发送失败响应给客户端
        send_json_response(client_fd, 409, {{"error", "User already exists"}}, false);
        return USER_ALREADY_EXISTS; // 认证失败，连接应该关闭
    }

    if (!InsertUserInfo(username, hashPassword(password), InvCode)) {
        // 发送失败响应给客户端
        send_json_response(client_fd, 500, {{"error", "Database error"}}, false);
        return DATABASE_ERROR; // 认证失败，连接应该关闭
    }
    return SUCCESS;
}