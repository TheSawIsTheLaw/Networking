#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

#define ERRROR_SOCKET_CREATION 1
#define ERROR_BIND 2
#define ERROR_RECV 3

#define SIN_PORT 1337

#define BUFFER_SIZE 666
static int socketDescr;

void outHandle(int sigNum)
{
    close(socketDescr);
    exit(EXIT_SUCCESS);
}

int main()
{
    socketDescr = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    if (socketDescr < 0)
    {
        printf("Socket creation failed.");
        return ERRROR_SOCKET_CREATION;
    }

    struct sockaddr_in addr = {
        .sin_family = AF_INET,
        .sin_port = SIN_PORT,
        .sin_addr.s_addr = INADDR_ANY
    };

    if (bind(socketDescr, (struct sockaddr*)&addr, sizeof(addr)) < 0)
    {
        printf("Bind error");
        return ERROR_BIND;
    }

    printf("Server is ready to go.\n"
           "Exit: ctrl + C\n");

    signal(SIGINT, outHandle);

    char flexBuffer[BUFFER_SIZE];
    struct sockaddr_in client = { 0 };
    socklen_t len = sizeof(struct sockaddr_in);
    size_t gotInBytes = 0;
    for (;;)
    {
        gotInBytes = recvfrom(socketDescr, flexBuffer, BUFFER_SIZE, 0, (struct sockaddr*)&client, &len);
        if (gotInBytes < 0)
        {
            printf("Recvfrom error.");
            return ERROR_RECV;
        }
        flexBuffer[gotInBytes] = '\0';

        printf("CATCH! And we've got from %s:%d message: %s", inet_ntoa(client.sin_addr), ntohs(client.sin_port), flexBuffer);
    }

    return 0;
}
