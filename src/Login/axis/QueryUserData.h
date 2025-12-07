#ifndef QUERY_USER_DATA_H
#define QUERY_USER_DATA_H

#include <string>

std::string queryUserPwd(const std::string& username);
bool IsUserExists(const std::string& username);
bool InsertUserInfo(const std::string& username, std::const string pwd, std::const string invCode);
#endif // QUERY_USER_DATA_H