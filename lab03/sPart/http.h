#ifndef HTTP_H
#define HTTP_H

#include <netinet/in.h>

constexpr int THREAD_POOL_SIZE = 20;
constexpr int LISTEN_COUNT = 100;

void create_threads();
void cancel_threads();
void *thread_function(void *argv);
void new_client(const sockaddr_in &client_addr, int conn_fd);

void sighandler(int signum);
int shutdown_server(const char *str);

#endif // HTTP_H
