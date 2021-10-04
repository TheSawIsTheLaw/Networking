#include <netdb.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>

#include "properties.h"

#define ERROR_PARAMETERS 1
#define ERROR_SOCKET_CREATURE 2
#define ERROR_SEND_TO 3

int main(int argc, char *argv[])
{
    if (argc < 2)
    {
        printError("Error: no message in arguments.");
        return ERROR_PARAMETERS;
    }

    int socketDescr = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    if (socketDescr < 0)
    {
        printError("Cannot create socket. Exit...");
        return ERROR_SOCKET_CREATURE;
    }

    struct sockaddr_in serverAddrToSend = {
        .sin_family = AF_INET,
        .sin_port = htons(SER_PORT), 
        .sin_addr.s_addr = INADDR_ANY
    };

    if (sendto(socketDescr, argv[1], strlen(argv[1]), 0, (struct sockaddr *)&serverAddrToSend,
               sizeof(serverAddrToSend)) < 0)
    {
        printError("Cannot send a message.");
        return ERROR_SEND_TO;
    }

    printOkMessage("Message was sent.\n");

    close(socketDescr);

    return EXIT_SUCCESS;
}
