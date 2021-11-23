#ifndef HTTP_H
#define HTTP_H

#include <netinet/in.h>

constexpr int NUMBER_OF_THREADS = 20;
constexpr int LISTEN_COUNT = 100;

void threadsCreation();
void threadsRemove();
void *threadFun(void *argv);
void addClientToQueue(const sockaddr_in &client_addr, int conn_fd);

void signalHandler(int signum);
int exitOnServerError(const char *str);

#endif // HTTP_H
