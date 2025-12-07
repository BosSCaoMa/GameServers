
# 登录流程
1. 主线程 (ProcLoginReq)
   ↓
   accept() → client_fd=5
   ↓
   创建 shared_ptr<Client>(fd=5)  [引用计数=1]
   ↓
   启动工作线程 handle_client(client)  [引用计数=2]
   ↓
   继续accept下一个连接

2. 工作线程 (handle_client)
   ↓
   读取HTTP请求
   ↓
   ProcLoginRequest(request, client)
   ↓
   ├─ 认证成功
   │  ↓
   │  SendLoginSuccessResponse() → 发送HTTP 200响应
   │  ↓
   │  BuildSession()
   │  └─ g_eventLoop->addClient(client)  [引用计数=3]
   │     ├─ EventLoop持有client的shared_ptr
   │     └─ 添加到epoll监听
   │  ↓
   │  返回 keepConnection=true
   │  ↓
   │  handle_client函数结束  [引用计数=2]
   │  └─ EventLoop继续持有client，连接保持打开
   │
   └─ 认证失败
      ↓
      send_json_response(401)
      ↓
      返回 keepConnection=false
      ↓
      handle_client函数结束  [引用计数=1]
      ↓
      主线程的线程也结束  [引用计数=0]
      ↓
      Client析构 → close(fd) → 连接关闭

3. EventLoop线程
   ↓
   loop() {
       poll() → 等待事件
       ↓
       收到客户端数据
       ↓
       handleClient() → 处理游戏协议
   }