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
#define ERROR_RECV 4
#define ERROR_CATCH_ANSWER 5

int main()
{
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

    // Отправка сообщения-привестствия для того, чтобы узнать, какие есть файлы
    if (sendto(socketDescr, "hello", strlen("hello"), 0, (struct sockaddr *)&serverAddrToSend,
               sizeof(serverAddrToSend)) < 0)
    {
        printError("Cannot send a hello message.");
        return ERROR_SEND_TO;
    }

    // Получение информации о файлах на сервере
    char flexBuffer[BUFFER_SIZE];
    struct sockaddr_in server = {0};
    socklen_t len = sizeof(struct sockaddr_in);
    size_t gotInBytes = recvfrom(socketDescr, flexBuffer, BUFFER_SIZE, 0, (struct sockaddr *)&server, &len);
    if (gotInBytes < 0)
    {
        printError("Recvfrom error");
        return ERROR_RECV;
    }

    if (server.sin_family != serverAddrToSend.sin_family ||
            server.sin_port != serverAddrToSend.sin_port)
    {
        printError("Got answer from another server. Kinda strange...");
        return ERROR_CATCH_ANSWER;
    }

    flexBuffer[gotInBytes] = '\0';
    printOkMessage("Files on server:");
    printOkMessage(flexBuffer);

    printOkMessage("\nChoose filename from this list: ");
    char choice[FILENAME_MAX];
    scanf("%s", choice);

    if (sendto(socketDescr, choice, strlen(choice), 0, (struct sockaddr *)&serverAddrToSend,
               sizeof(serverAddrToSend)) < 0)
    {
        printError("Cannot send name of file to server.");
        return ERROR_SEND_TO;
    }

    close(socketDescr);

    return EXIT_SUCCESS;
}
