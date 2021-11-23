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
        std::cout << "Cannot create socket";

        return EXIT_FAILURE;
    }

    const sockaddr_in serverAddr = {
        .sin_family = AF_INET,
        .sin_port = htons(SERVER_PORT),
        .sin_addr = {.s_addr = INADDR_ANY},
    };

    const linger option = {
        .l_onoff = 1,
        .l_linger = 0,
    };
    if (setsockopt(serverSocket, SOL_SOCKET, SO_LINGER, &option, sizeof(option)) == -1)
    {
        return exitOnServerError("Error on setsockopt");
    }

    if (bind(serverSocket, reinterpret_cast<const sockaddr *>(&serverAddr), sizeof(serverAddr)) == -1)
    {
        return exitOnServerError("Error on bind");
    }

    if (listen(serverSocket, LISTEN_COUNT) == -1)
    {
        return exitOnServerError("Error on listen");
    }

    if (signal(SIGINT, signalHandler) == SIG_ERR)
    {
        return exitOnServerError("Error on exit signal handler setter");
    }

    std::cout << "Server started on port " << ntohs(serverAddr.sin_port) << std::endl;
    for (;;)
    {
        sockaddr_in clientAddr;
        socklen_t client_size;
        const int connectionSocket = accept(serverSocket, reinterpret_cast<sockaddr *>(&clientAddr), &client_size);
        if (connectionSocket == -1)
        {
            return exitOnServerError("Error on acception");
        }

        addClientToQueue(clientAddr, connectionSocket);
    }
}
