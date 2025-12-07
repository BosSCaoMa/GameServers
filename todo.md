## 痛点 1

client_fd编写代码的时候，一直在考虑，资源释放问题


## 2
数据库连接池写好了，在程序开始前怎么进行初始化呢？如果是在main中初始化，其他函数中使用ConnectionPoolAgent去用连接池的时候不太好用，一般用什么办法解决？

封装成全局/单例：例如在 DBConnPool 内提供 static DBConnPool& instance()，程序启动时（main or InitServer）调用 DBConnPool::instance().init(...)，其他地方只需要 ConnectionPoolAgent agent{&DBConnPool::instance()};。做法简单，但全局状态会让测试和多环境配置麻烦。

RAII 服务容器：在 main 创建一个 ServerContext ctx{DBConnPool{...}};，把指针/引用传进所有需要访问 DB 的模块（构造注入或 setter 注入）。业务 handler 再用 ConnectionPoolAgent agent{ctx.dbPool};。这是最常见的 C++ 服务器写法，方便配置/依赖替换。


## 3 第三方库要执行命令安装

方案 A（最常用）：用脚本自动检测 + 提示安装
方案 B：完全不用系统包，而是把 libsodium 当第三方库拉进工程（推荐度 最高）