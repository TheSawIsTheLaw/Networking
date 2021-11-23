#ifndef HTTP_H
#define HTTP_H

#include <netinet/in.h>

#include <string>

constexpr int NUMBER_OF_THREADS = 20;
constexpr int LISTEN_COUNT = 100;

void threadsCreation();
void threadsRemove();
void *threadFun(void *argv);
void addClientToQueue(const sockaddr_in &clientAddr, int clientConnection);

void signalHandler(int signum);
int exitOnServerError(std::string errorString);

#endif // HTTP_H
