#ifndef RECV_PROC_H
#define RECV_PROC_H

// 单独起一个线程调用这个函数，监听Login连接请求
int ProcLoginReq(int port);

#endif // RECV_PROC_H