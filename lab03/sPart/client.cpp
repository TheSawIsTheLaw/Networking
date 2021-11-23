#include <iostream>
#include <sstream>
#include <string>

#include <arpa/inet.h>
#include <cerrno>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <netinet/in.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

#include "properties.h"

int clientSocket;

void sendGetRequest(const std::string &uri)
{
    std::ostringstream oss;
    oss << "GET /" << uri << " HTTP/1.1\n";
    oss << "Host: 127.0.0.1:" << SERVER_PORT << '\n';
    oss << "Connection: close\n";
    const auto request = oss.str();

    std::cout << "[[Request]]\n" << request << '\n';
    write(clientSocket, request.c_str(), request.size());
}

void createResponse()
{
    char buff[BUFFER_SIZE + 1];
    int n = 0;
    std::cout << "[[Response]]\n";
    do
    {
        bzero(buff, sizeof buff);
        n = read(clientSocket, buff, sizeof buff - 1);
        std::cout << buff;
        fflush(stdout);
    } while (n == sizeof buff - 1);
    std::cout << std::endl;
}

int exitClientOnFailure(const char *str)
{
    close(clientSocket);
    perror(str);
    return EXIT_FAILURE;
}

int main(int argc, char **argv)
{
    const char *filename = "index.html";

    if (argc == 1 || argc > 3)
    {
        std::cout << "Parameters of program:\n1. Port (required);\n2. URI (unnecessary);\n";
        return EXIT_FAILURE;
    }

    std::string uri = filename;

    errno = 0;
    char *endptr = argv[1];
    int client_port = strtol(argv[1], &endptr, 10);
    if (*endptr || errno)
    {
        std::cout << "Client port parse error :(";
        return EXIT_FAILURE;
    }

    constexpr int MIN_PORT = 1024;
    constexpr int MAX_PORT = 65535;
    if (client_port < MIN_PORT || client_port > MAX_PORT || client_port == SERVER_PORT)
    {
        fprintf(stderr, "argv[1]: invalid port\n");
        return EXIT_FAILURE;
    }

    if (argc == 3)
    {
        uri = argv[2];
    }

    if ((clientSocket = socket(AF_INET, SOCK_STREAM, 0)) == -1)
    {
        perror("socket");
        return EXIT_FAILURE;
    }

    const sockaddr_in client_addr = {
        .sin_family = AF_INET, .sin_port = htons(client_port), .sin_addr{.s_addr = INADDR_ANY}};

    const linger sl = {
        .l_onoff = 1,
        .l_linger = 0,
    };
    if (setsockopt(clientSocket, SOL_SOCKET, SO_LINGER, &sl, sizeof sl) == -1)
    {
        return exitClientOnFailure("setsockopt");
    }

    if (bind(clientSocket, reinterpret_cast<const sockaddr *>(&client_addr), sizeof client_addr) == -1)
    {
        return exitClientOnFailure("bind");
    }

    const sockaddr_in server_addr = {
        .sin_family = AF_INET,
        .sin_port = htons(SERVER_PORT),
        .sin_addr = {.s_addr = inet_addr("127.0.0.1")},
    };

    if (connect(clientSocket, reinterpret_cast<const sockaddr *>(&server_addr), sizeof server_addr) == -1)
    {
        return exitClientOnFailure("connect");
    }

    sendGetRequest(uri);
    std::cout << "Request sent.";
    createResponse();

    close(clientSocket);
}
