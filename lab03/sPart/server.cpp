#include <arpa/inet.h>
#include <cstdio>
#include <stdlib.h>

#include "properties.h"
#include <signal.h>
#include "http.h"

extern int server_sd;

int main() {
    create_threads();

    if ((server_sd = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
        cancel_threads();
        perror("socket");
        return EXIT_FAILURE;
    }

    const sockaddr_in server_addr = {
        .sin_family = AF_INET,
        .sin_port = htons(SERVER_PORT),
        .sin_addr = {.s_addr = INADDR_ANY},
    };

    const linger sl = {
        .l_onoff = 1,
        .l_linger = 0,
    };
    if (setsockopt(server_sd, SOL_SOCKET, SO_LINGER, &sl, sizeof sl) == -1) {
        return shutdown_server("setsockopt");
    }

    if (bind(server_sd, reinterpret_cast<const sockaddr*>(&server_addr), sizeof server_addr) == -1) {
        return shutdown_server("bind");
    }

    if (listen(server_sd, LISTEN_COUNT) == -1) {
        return shutdown_server("listen");
    }

    if (signal(SIGINT, sighandler) == SIG_ERR) {
        return shutdown_server("signal");
    }

    printf("server is running on %s:%d\n", inet_ntoa(server_addr.sin_addr), ntohs(server_addr.sin_port));
    for (;;) {
        sockaddr_in client_addr{};
        socklen_t client_size = sizeof client_addr;
        const int conn_fd = accept(server_sd, reinterpret_cast<sockaddr*>(&client_addr), &client_size);
        if (conn_fd == -1) {
            return shutdown_server("accept");
        }

        new_client(client_addr, conn_fd);
    }
}
