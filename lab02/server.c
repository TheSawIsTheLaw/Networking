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
#define ERROR_SIGACTION 4

static int socketDescr;

static char *itoaAlphabet = "0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZ";

int itoa(int numToConvert, int base, char *outString)
{
    if (base > 36 || base < 2)
    {
        return 0;
    }

    int memoryRemains = numToConvert % base;
    // Fortune, fame, mirror, vain, gone insane but the memory remains...
    // Sry.

    numToConvert = numToConvert / base;

    if (numToConvert == 0)
    {
        outString[0] = itoaAlphabet[memoryRemains];
        return 1;
    }

    int proccessedIndex = itoa(numToConvert, base, outString);
    outString[proccessedIndex++] = itoaAlphabet[memoryRemains];

    return proccessedIndex;
}

void outHandle(int sigNum)
{
    close(socketDescr);
    printOkMessage("Good bye!\n");
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
            printError("Recvfrom error");
            return ERROR_RECV;
        }
        flexBuffer[gotInBytes] = '\0';

        int usedNum = atoi(flexBuffer);
        printf(GREEN "CATCH! And we've got from %s:%d "
                     "Decimal message: %s\n",
               inet_ntoa(client.sin_addr), ntohs(client.sin_port), flexBuffer);

        if (usedNum < 0)
        {
            printError("Negative numbers are not supported :(\n");
        }
        else
        {
            char bin[100] = "\0";
            char hex[100] = "\0";
            char oct[100] = "\0";
            char twenty[100] = "\0";

            itoa(usedNum, 2, bin);
            itoa(usedNum, 16, hex);
            itoa(usedNum, 8, oct);
            itoa(usedNum, 20, twenty);
            printf(CYAN "Got message in binary: %s\n"
                        "Got message in hexidecimal: %s\n"
                        "Got message in octal: %s\n"
                        "Got message in variant 20: %s\n" RESET,
                   bin, hex, oct, twenty);
        }

        printSeparator();
    }
}

int main()
{
    socketDescr = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    if (socketDescr < 0)
    {
        printError("Socket creation failed.");
        return ERRROR_SOCKET_CREATION;
    }

    struct sockaddr_in addr = {
        // IP-based communication
        .sin_family = AF_INET,
        .sin_port = htons(SER_PORT),
        .sin_addr.s_addr = INADDR_ANY // ???? ???????? ?????????????????? ??????????????????????, ?????????????? ?????????????????????????? ?? ???????????? ???????????????????? ??????????
    };

    if (bind(socketDescr, (struct sockaddr *)&addr, sizeof(addr)) < 0)
    {
        printError("Bind error");
        return ERROR_BIND;
    }

    printOkMessage("Server is ready to go.\n"
                   "Exit: ctrl + C\n");
    printSeparator();

    struct sigaction sigAct = {
        .sa_handler = outHandle,
        .sa_flags = 0
    };

    if (sigaction(SIGINT, &sigAct, NULL) < 0)
    {
        printError("Can not set sigaction.");
        return ERROR_SIGACTION;
    }

    return startReceiver();
}
