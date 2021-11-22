#include <arpa/inet.h>
#include <dirent.h>
#include <fcntl.h>
#include <netinet/in.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/sendfile.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#include "properties.h"

#define ERRROR_SOCKET_CREATION 1
#define ERROR_BIND 2
#define ERROR_RECV 3
#define ERROR_SIGACTION 4
#define ERROR_SENDTO 5
#define ERROR_LISTEN 6

static int socketDescr;
static int socketToSendFile;

void outHandle(int sigNum)
{
    close(socketDescr);
    printOkMessage("Good bye!\n");
    exit(EXIT_SUCCESS);
}

void fullFilesInDirectory(char *filesInDirectory)
{
    DIR *d;
    struct dirent *dir;
    d = opendir("./static");
    size_t currentPointerInForm = 0;

    if (d)
    {
        // Пропускаем текущую и родительскую
        for (int i = 0; i < 2; i++, readdir(d))
        {
        }

        // Считываем имена
        while ((dir = readdir(d)) != NULL)
        {
            for (int i = 0; i < strlen(dir->d_name); i++, currentPointerInForm++)
            {
                filesInDirectory[currentPointerInForm] = dir->d_name[i];
            }
            filesInDirectory[currentPointerInForm++] = '\n';
        }

        closedir(d);
    }

    filesInDirectory[currentPointerInForm] = '\0';
}

int startReceiver()
{
    char filesInDirectory[BUFFER_SIZE] = {0};
    fullFilesInDirectory(filesInDirectory);

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

        if (strcmp(flexBuffer, "hello"))
        {
            printError("Hello step is confused because it's not a hello message.");
            continue;
        }

        printf(GREEN "CATCH! And we've got from %s:%d "
                     "Message: %s\n",
               inet_ntoa(client.sin_addr), ntohs(client.sin_port), flexBuffer);

        printf(GREEN "Sending to it filenames...\n");

        printf("%s", filesInDirectory);
        if (sendto(socketDescr, filesInDirectory, strlen(filesInDirectory), 0, (struct sockaddr *)&client,
                   sizeof(client)) < 0)
        {
            printError("Cannot send a message to client");
            return ERROR_SENDTO;
        }

        gotInBytes = recvfrom(socketDescr, flexBuffer, BUFFER_SIZE, 0, (struct sockaddr *)&client, &len);
        if (gotInBytes < 0)
        {
            printError("Recvfrom error");
            return ERROR_RECV;
        }
        flexBuffer[gotInBytes] = '\0';

        char fileToSendName[BUFFER_SIZE] = {0};
        printf("Got buffer: %s with size %ld\n", flexBuffer, strlen(flexBuffer));
        snprintf(fileToSendName, strlen(flexBuffer) + 8, "static/%s", flexBuffer);
        printf("|||%s|||\n", fileToSendName);
        printf("Filename got: %s\n", fileToSendName);
        int fileToSend = open(fileToSendName, O_RDONLY);
        if (fileToSend < 0)
        {
            printError("Cannot open file\n");
            continue;
        }

        struct stat fileStat;
        if (fstat(fileToSend, &fileStat) < 0)
        {
            printError("Cannot get file stats");
            continue;
        }

        socklen_t socketLen = sizeof(struct sockaddr_in);
        struct sockaddr_in peerAddr;
        int peerSocket = accept(socketToSendFile, (struct sockaddr *)&peerAddr, &socketLen);

        if (peerSocket < 0)
        {
            printError("Error in acception");
            continue;
        }

        char fileSize[FILENAME_MAX];
        snprintf(fileSize, FILENAME_MAX, "%ld", fileStat.st_size);
        if (send(peerSocket, fileSize, sizeof(fileSize), 0) < 0)
        {
            printError("Send error");
            continue;
        }

        long offset = 0;
        size_t remainData = fileStat.st_size;

        int sent = 0;
        while (((sent = sendfile(peerSocket, fileToSend, &offset, BUFSIZ)) > 0) && (remainData > 0))
        {
            remainData -= sent;
        }

        close(peerSocket);
        close(fileToSend);

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
        .sin_addr.s_addr = INADDR_ANY // Ко всем доступным интерфейсам, поэтому привязывается к любому свободному порту
    };

    if (bind(socketDescr, (struct sockaddr *)&addr, sizeof(addr)) < 0)
    {
        printError("Bind error");
        return ERROR_BIND;
    }

    socketToSendFile = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (socketToSendFile < 0)
    {
        printError("Socket creation failed.");
        return ERRROR_SOCKET_CREATION;
    }

    struct sockaddr_in addrToSendFile = {
        .sin_family = AF_INET, .sin_port = htons(SER_PORT_DATA_TRANSFER), .sin_addr.s_addr = INADDR_ANY};

    if (bind(socketToSendFile, (struct sockaddr *)&addrToSendFile, sizeof(addrToSendFile)) < 0)
    {
        printError("Bind error");
        return ERROR_BIND;
    }

    if ((listen(socketToSendFile, 5)) < 0)
    {
        printError("Listening error");
        return ERROR_LISTEN;
    }

    printOkMessage("Server is ready to go.\n"
                   "Exit: ctrl + C\n");
    printSeparator();

    struct sigaction sigAct = {.sa_handler = outHandle, .sa_flags = 0};

    if (sigaction(SIGINT, &sigAct, NULL) < 0)
    {
        printError("Can not set sigaction.");
        return ERROR_SIGACTION;
    }

    return startReceiver();
}
