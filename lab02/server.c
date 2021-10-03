#include <arpa/inet.h>
#include <netinet/in.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

#include "properties.h"

#define ERRROR_SOCKET_CREATION 1
#define ERROR_BIND 2
#define ERROR_RECV 3

static int socketDescr;

int itoa(int a, int p, char *s)
{
    char letters[30] = {"0123456789ABCDEFGHIJKLMNOPQRST"};

    int num = (int)a;
    int rest = num % p;
    num /= p;
    if (num == 0)
    {
        s[0] = letters[rest];
        return 1;
    }
    int k = itoa(num, p, s);
    s[k++] = letters[rest];
    return k;
}

void outHandle(int sigNum)
{
    close(socketDescr);
    exit(EXIT_SUCCESS);
}

int startReceiver()
{
    char flexBuffer[BUFFER_SIZE];
    struct sockaddr_in client = {0};
    socklen_t len = sizeof(struct sockaddr_in);
    size_t gotInBytes = 0;
    for (;;)
    {
        gotInBytes = recvfrom(socketDescr, flexBuffer, BUFFER_SIZE, 0, (struct sockaddr *)&client, &len);
        if (gotInBytes < 0)
        {
            printf("Recvfrom error.");
            return ERROR_RECV;
        }
        flexBuffer[gotInBytes] = '\0';

        int usedNum = atoi(flexBuffer);
        printf("CATCH! And we've got from %s:%d "
               "Decimal message: %s\n",
               inet_ntoa(client.sin_addr), ntohs(client.sin_port), flexBuffer);

        char bin[100] = "\0";
        char hex[100] = "\0";
        char oct[100] = "\0";
        char twenty[100] = "\0";

        itoa(usedNum, 2, bin);
        itoa(usedNum, 16, hex);
        itoa(usedNum, 8, oct);
        itoa(usedNum, 20, twenty);
        printf("Got message in binary: %s\n"
               "Got message in hexidecimal: %s\n"
               "Got message in octal: %s\n"
               "Got message in variant 20: %s\n",
               bin, hex, oct, twenty);
    }
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
        // IP-based communication
        .sin_family = AF_INET,
        .sin_port = htons(SER_PORT),
        .sin_addr.s_addr = INADDR_ANY // Any enable interface
    };

    if (bind(socketDescr, (struct sockaddr *)&addr, sizeof(addr)) < 0)
    {
        printf("Bind error");
        return ERROR_BIND;
    }

    printf("Server is ready to go.\n"
           "Exit: ctrl + C\n");

    signal(SIGINT, outHandle);

    return startReceiver();
}
