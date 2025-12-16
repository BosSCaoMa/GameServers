#ifndef RECVPROC_H
#define RECVPROC_H

#include <cstddef>

class Client;

void handleGameMessage(Client* client, const char* buffer, size_t length);

#endif // RECVPROC_H