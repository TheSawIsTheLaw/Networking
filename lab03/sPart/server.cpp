#include <arpa/inet.h>
#include <cstdio>
#include <stdlib.h>

#include "http.h"
#include "properties.h"
#include <signal.h>

extern int server_sd;

int main()
{
    threadsCreation();

    if ((server_sd = socket(AF_INET, SOCK_STREAM, 0)) == -1)
    {
        threadsRemove();
        perror("socket");
        return EXIT_FAILURE;
    }

    const sockaddr_in server_addr = {
        .sin_family = AF_INET,
        .sin_port = htons(SERVER_PORT),
        .sin_addr = {.s_addr = INADDR_ANY},
    };

    const linger sl = {
        .l_onoff = 1,
        .l_linger = 0,
    };
    if (setsockopt(server_sd, SOL_SOCKET, SO_LINGER, &sl, sizeof sl) == -1)
    {
        return exitOnServerError("setsockopt");
    }

    if (bind(server_sd, reinterpret_cast<const sockaddr *>(&server_addr), sizeof server_addr) == -1)
    {
        return exitOnServerError("bind");
    }

    if (listen(server_sd, LISTEN_COUNT) == -1)
    {
        return exitOnServerError("listen");
    }

    if (signal(SIGINT, signalHandler) == SIG_ERR)
    {
        return exitOnServerError("signal");
    }

    printf("server is running on %s:%d\n", inet_ntoa(server_addr.sin_addr), ntohs(server_addr.sin_port));
    for (;;)
    {
        sockaddr_in client_addr{};
        socklen_t client_size = sizeof client_addr;
        const int conn_fd = accept(server_sd, reinterpret_cast<sockaddr *>(&client_addr), &client_size);
        if (conn_fd == -1)
        {
            return exitOnServerError("accept");
        }

        addClientToQueue(client_addr, conn_fd);
    }
}
