#include <iostream>
#include "LogM.h"
#include "ConnectionPool.h"
#include <sodium.h>
#include "DBConnPool.h"

using namespace std;

int main()
{
    if (sodium_init() < 0) {
        // panic: libsodium 初始化失败
        return 1;
    }
    // 数据库连接是个较慢的操作，建议程序启动时就初始化连接池
    string pwd;
    cout<<"请输入数据库密码:"<<endl;
    cin>>pwd;
    DBConnInfo userDbInfo{"tcp://127.0.0.1:3306", "user", "password", "userdb"};
    auto userPool = GetUserDBPool();
    DBConnInfo userDbInfo{"tcp://127.0.0.1:3306", "user", "password", "gamedb"};
    auto gamePool = GetGameDBPool();


}