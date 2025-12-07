# GameServers
高性能游戏服务器

## 技术栈全景图
```
┌─────────────────────────────────────────────────────────────────┐
│                      游戏服务器架构                              │
├─────────────────────────────────────────────────────────────────┤
│  客户端 ←→ 网关服务器 ←→ 游戏逻辑服务器 ←→ 数据库/缓存             │
│                ↓              ↓              ↓                  │
│            负载均衡       消息队列        持久化存储              │
└─────────────────────────────────────────────────────────────────┘
```

```javascript
                    ┌─────────────┐
                    │   客户端    │
                    └──────┬──────┘
                           │
                    ┌──────▼──────┐
                    │  网关服务器  │  ← 负载均衡、连接管理
                    │   Gateway   │
                    └──────┬──────┘
                           │
        ┌──────────────────┼──────────────────┐
        │                  │                  │
┌───────▼───────┐  ┌───────▼───────┐  ┌───────▼───────┐
│   登录服务器   │  │   游戏服务器   │  │   匹配服务器   │
│    Login      │  │     Game      │  │    Match      │
└───────┬───────┘  └───────┬───────┘  └───────────────┘
        │                  │
        └────────┬─────────┘
                 │
        ┌────────▼────────┐
        │    数据服务器    │  ← DB代理，缓存管理
        │   DB Proxy      │
        └────────┬────────┘
                 │
    ┌────────────┴────────────┐
    │                         │
┌───▼───┐               ┌─────▼─────┐
│ Redis │               │   MySQL   │
└───────┘               └───────────┘
```

## WebSocket

### 前端
```JavaScript
// 建立连接
const ws = new WebSocket('wss://game.com/ws');

// 连接成功
ws.onopen = function() {
    console.log('连接成功');
};

// 收到消息（服务器随时可能发过来）
ws.onmessage = function(event) {
    const msg = JSON.parse(event.data);
    
    if (msg.type === 'attack_result') {
        console.log('攻击结果:', msg);
    }
    else if (msg.type === 'be_hit') {
        console.log('你被打了！', msg);
    }
    else if (msg.type === 'player_enter') {
        console.log('有人进入视野', msg);
    }
    // ... 其他消息类型
};

// 连接断开
ws.onclose = function() {
    console.log('断开连接');
};

// 发送消息
function attack(targetId) {
    ws.send(JSON.stringify({
        type: 'attack',
        targetId: targetId
    }));
    // 不用等返回，结果会通过 onmessage 收到
}
```

```JavaScript
┌─────────────────────────────────────────────────────────────┐
│                         HTTP                                 │
├─────────────────────────────────────────────────────────────┤
│                                                             │
│   发请求  ──→  等待  ──→  收响应  ──→  结束                  │
│                                                             │
│   想收消息？自己去问（轮询）                                  │
│                                                             │
│   代码模式：                                                 │
│       result = await fetch(...)                             │
│       // 发完就能拿到结果                                    │
│                                                             │
└─────────────────────────────────────────────────────────────┘

┌─────────────────────────────────────────────────────────────┐
│                       WebSocket                              │
├─────────────────────────────────────────────────────────────┤
│                                                             │
│   建立连接后：                                               │
│       发消息：ws.send(...)      随时可以发                   │
│       收消息：ws.onmessage      随时可能收到                 │
│                                                             │
│   服务器想发就发，不用你问                                    │
│                                                             │
│   代码模式：                                                 │
│       ws.send(...)              // 发                       │
│       ws.onmessage = (msg) => { // 收（事件回调）            │
│           ...                                               │
│       }                                                     │
│                                                             │
└─────────────────────────────────────────────────────────────┘
```

### 后端

```cpp
// WebSocket：连接一直保持
void websocket_server() {
    
    while (true) {
        Socket conn = accept();         // 等待连接
        
        // 1. 先完成握手
        if (!do_handshake(conn)) {
            conn.close();
            continue;
        }
        
        // 2. 创建 Session，绑定这个连接
        Session* session = new Session(conn);
        
        // 3. 这个连接会一直活着，放到列表里管理
        all_sessions.add(session);
        
        // 4. 开个线程/协程持续处理这个连接
        start_session_loop(session);
    }
}

void session_loop(Session* session) {
    
    while (session->connected) {
        
        // 不停地读消息
        Frame frame = read_frame(session->conn);
        
        // 处理消息
        handle(session, frame);
        
        // 不关闭！继续循环读下一条消息
    }
    
    // 连接断开了才会到这里
    all_sessions.remove(session);
    delete session;
}
```

# 接口文档

## 后端接口

### 前端
前端json接口：Content-Type application/json
表单：其他

登录路径 "/api/login"


### 数据库
`查询密码 SELECT password_hash FROM sys_user WHERE username = ?`
