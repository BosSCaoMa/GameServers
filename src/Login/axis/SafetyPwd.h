#ifndef SAFETYPWD_H
#define SAFETYPWD_H

#include <string>

std::string hashPassword(const std::string& password);
bool verifyPassword(const std::string& password, const std::string& storedHash) ;

#endif // SAFETYPWD_H