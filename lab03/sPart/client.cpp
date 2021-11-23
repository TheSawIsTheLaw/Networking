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
    std::ostringstream stringToForm;
    stringToForm << "GET /" << uri << " HTTP/1.1\n";
    stringToForm << "Host: 127.0.0.1:" << SERVER_PORT << '\n';
    stringToForm << "Connection: close\n";
    const auto request = stringToForm.str();

    std::cout << "Request to be sent:\n" << request << '\n';
    write(clientSocket, request.c_str(), request.size());
}

void createResponse()
{
    char buff[BUFFER_SIZE + 1];
    int numberOfRead = 0;
    std::cout << "Response got:\n";
    do
    {
        bzero(buff, sizeof(buff));
        numberOfRead = read(clientSocket, buff, sizeof(buff) - 1);
        std::cout << buff << std::endl;
    } while (numberOfRead == sizeof(buff) - 1);
    std::cout << std::endl;
}

int exitClientOnFailure(std::string errorString)
{
    close(clientSocket);
    std::cout << errorString << std::endl;

    return EXIT_FAILURE;
}

int main(int argc, char **argv)
{
    if (argc == 1 || argc > 3)
    {
        std::cout << "Parameters of program:\n1. Port (required);\n2. URI (unnecessary);\n";

        return EXIT_FAILURE;
    }

    std::string uri = (argc == 3) ? argv[2] : "index.html";

    char *end = argv[1];
    errno = 0;
    int gotPort = strtol(argv[1], &end, 10);
    if (*end || errno)
    {
        std::cout << "IPandSocketObject port parse error :(";

        return EXIT_FAILURE;
    }

    if (gotPort < 1024 || gotPort > 65535)
    {
        std::cout << "Port cannot be used";

        return EXIT_FAILURE;
    }

    if ((clientSocket = socket(AF_INET, SOCK_STREAM, 0)) == -1)
    {
        std::cout << "Error on socket creation" << std::endl;
        return EXIT_FAILURE;
    }

    const sockaddr_in clientAddr = {
        .sin_family = AF_INET, .sin_port = htons(gotPort), .sin_addr{.s_addr = INADDR_ANY}};

    const linger option = {
        .l_onoff = 1,
        .l_linger = 0,
    };
    if (setsockopt(clientSocket, SOL_SOCKET, SO_LINGER, &option, sizeof(option)) == -1)
    {
        return exitClientOnFailure("Error on setsockopt");
    }

    if (bind(clientSocket, reinterpret_cast<const sockaddr *>(&clientAddr), sizeof(clientAddr)) == -1)
    {
        return exitClientOnFailure("Error on bind");
    }

    const sockaddr_in serverAddr = {
        .sin_family = AF_INET,
        .sin_port = htons(SERVER_PORT),
        .sin_addr = {.s_addr = inet_addr("127.0.0.1")},
    };

    if (connect(clientSocket, reinterpret_cast<const sockaddr *>(&serverAddr), sizeof(serverAddr)) == -1)
    {
        return exitClientOnFailure("Error on connection set");
    }

    sendGetRequest(uri);
    std::cout << "Request sent successfully!" << std::endl;
    createResponse();

    close(clientSocket);
}
