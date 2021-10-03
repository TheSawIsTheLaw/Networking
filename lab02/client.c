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
        printf("Error: no message.");
        return ERROR_PARAMETERS;
    }

    int socketDecsr = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    if (socketDecsr < 0)
    {
        printf("Cannot create socket. Exit...");
        return ERROR_SOCKET_CREATURE;
    }

    struct sockaddr_in serverAddrToSend = {
        .sin_family = AF_INET, .sin_port = htons(SER_PORT), .sin_addr.s_addr = INADDR_ANY};

    if (sendto(socketDecsr, argv[1], strlen(argv[1]), 0, (struct sockaddr *)&serverAddrToSend,
               sizeof(serverAddrToSend)) < 0)
    {
        printf("Cannot send a message. Exit...");
        return ERROR_SEND_TO;
    }

    printf("Message was sent.\n");

    close(socketDecsr);

    return EXIT_SUCCESS;
}
