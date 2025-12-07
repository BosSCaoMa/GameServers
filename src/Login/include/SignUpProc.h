#ifndef SIGNUPPROC_H
#define SIGNUPPROC_H

#include <memory>
#include "Client.h"
#include "HttpRequest.h"
#include "http_response.h"
#include "SafetyPwd.h"
#include "QueryUserData.h"

bool ProcSignUpRequest(const HttpRequest& request, std::shared_ptr<Client> client);
bool VerifyInvCode(const std::string& invCode);

#endif // SIGNUPPROC_H