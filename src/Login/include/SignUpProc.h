#ifndef SIGNUPPROC_H
#define SIGNUPPROC_H

#include <memory>
#include "Client.h"
#include "HttpRequest.h"
enum class SignUpResult {
    SUCCESS,
    USER_ALREADY_EXISTS,
    INVCODE_INVALID,
    DATABASE_ERROR
};

SignUpResult ProcSignUpRequest(const HttpRequest& request, std::shared_ptr<Client> client);

#endif // SIGNUPPROC_H