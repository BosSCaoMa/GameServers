#ifndef LOGIN_PROC_H
#define LOGIN_PROC_H

#include <memory>
#include "Client.h"

#include "ParseHttp.h"

// 返回true表示连接已交给EventLoop管理，false表示连接应该关闭
bool ProcLoginRequest(HttpRequest& request, std::shared_ptr<Client> client);

#endif // LOGIN_PROC_H