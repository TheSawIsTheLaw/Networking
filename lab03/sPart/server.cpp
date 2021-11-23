#include <arpa/inet.h>
#include <cstdio>
#include <iostream>
#include <stdlib.h>

#include "http.h"
#include "properties.h"
#include <signal.h>

extern int serverSocket;

int main()
{
    threadsCreation();

    if ((serverSocket = socket(AF_INET, SOCK_STREAM, 0)) == -1)
    {
        threadsRemove();
        std::cout << "Cannot create socket.";

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
    if (setsockopt(serverSocket, SOL_SOCKET, SO_LINGER, &sl, sizeof sl) == -1)
    {
        return exitOnServerError("setsockopt");
    }

    if (bind(serverSocket, reinterpret_cast<const sockaddr *>(&server_addr), sizeof server_addr) == -1)
    {
        return exitOnServerError("bind");
    }

    if (listen(serverSocket, LISTEN_COUNT) == -1)
    {
        return exitOnServerError("listen");
    }

    if (signal(SIGINT, signalHandler) == SIG_ERR)
    {
        return exitOnServerError("signal");
    }

    std::cout << "Server started on port " << ntohs(server_addr.sin_port) << std::endl;
    for (;;)
    {
        sockaddr_in client_addr{};
        socklen_t client_size = sizeof client_addr;
        const int conn_fd = accept(serverSocket, reinterpret_cast<sockaddr *>(&client_addr), &client_size);
        if (conn_fd == -1)
        {
            return exitOnServerError("Error on acception");
        }

        addClientToQueue(client_addr, conn_fd);
    }
}
