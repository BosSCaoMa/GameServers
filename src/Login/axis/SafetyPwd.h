#ifndef SAFETYPWD_H
#define SAFETYPWD_H

#include <string>

std::string hashPassword(const std::string& password);
bool verifyPassword(const std::string& password, const std::string& storedHash) ;
std::string generateToken(const std::string& username);
#endif // SAFETYPWD_H