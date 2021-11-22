#include "http.h"

#include <arpa/inet.h>
#include <pthread.h>
#include <sys/socket.h>
#include <unistd.h>

#include <array>
#include <fstream>
#include <iostream>
#include <queue>
#include <sstream>
#include <string>
#include <unordered_map>

#include "sys/un.h"
#include <signal.h>

#include "properties.h"

int server_sd;

static sig_atomic_t stop;
static pthread_t thread_pool[THREAD_POOL_SIZE];

void create_threads()
{
    for (auto &thread : thread_pool)
    {
        pthread_create(&thread, nullptr, thread_function, nullptr);
    }
}

void cancel_threads()
{
    stop = true;
    for (auto &thread : thread_pool)
    {
        pthread_cancel(thread);
    }
}

class Client
{
  public:
    Client(sockaddr_in client_addr, int conn_fd)
        : ip(std::string(inet_ntoa(client_addr.sin_addr)) + ":" + std::to_string(ntohs(client_addr.sin_port))),
          socket(conn_fd)
    {
    }

    const std::string ip;
    const int socket;
};

static pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
static pthread_cond_t cond = PTHREAD_COND_INITIALIZER;
static std::queue<Client> queue;

static const std::unordered_map<std::string, std::string> content_types = {
    {".html", "text/html"}, {".css", "text/css"},    {".js", "application/javascript"},
    {".jpg", "image/jpeg"}, {".jpeg", "image/jpeg"}, {".png", "image/png"},
    {".gif", "image/gif"},
};

static std::unordered_map<std::string, std::string> get_headers(const std::string &header)
{
    std::unordered_map<std::string, std::string> headers;
    std::istringstream iss{header};
    for (std::string line; std::getline(iss, line) && line != "\n";)
    {
        auto key_end = line.find(':');

        auto key = line.substr(0, key_end);
        auto value = line.substr(key_end + 2, line.length() - key_end - 1);

        headers[key] = value;
    }
    return headers;
}

static std::string get_start_response_line(const std::string &protocol, int status_code)
{
    const std::unordered_map<int, std::string> statuses = {
        {200, "OK"},
        {403, "Forbidden"},
        {404, "Not Found"},
        {405, "Method Not Allowed"},
    };

    return protocol + " " + std::to_string(status_code) + " " + statuses.at(status_code);
}

static std::string response_format(const std::string &protocol, int status_code,
                                   std::unordered_map<std::string, std::string> headers, std::string body)
{
    std::string response = get_start_response_line(protocol, status_code) + "\n";
    std::cout << response;

    if (status_code != 200)
    {
        std::ifstream infile{"public/errors/" + std::to_string(status_code) + ".html"};
        body = std::string{std::istreambuf_iterator<char>(infile), std::istreambuf_iterator<char>()};
        infile.close();
        headers["Content-Type"] = content_types.at(".html");
        headers["Content-Length"] = std::to_string(body.length());
    }

    for (const auto &header : std::array<std::string, 3>{"Content-Type", "Content-Length", "Connection"})
    {
        if (headers.find(header) != headers.end())
        {
            response += header + ": " + headers.at(header) + "\n";
        }
    }

    response += "\n" + body;
    return response;
}

static std::tuple<std::string, std::string, std::string> split_start_request_line(const std::string &line)
{
    auto method_end = line.find(' ');
    auto path_end = line.find(' ', method_end + 1);

    auto method = line.substr(0, method_end);
    auto path = line.substr(method_end + 1, path_end - method_end - 1);
    auto protocol = line.substr(path_end + 1, line.length() - path_end - 1);
    if (path[0] == '/' && path.length() != 1)
    {
        path = path.substr(1, path.length() - 1);
    }
    else
    {
        path = "index.html";
    }

    path = "static/" + path;

    return {method, path, protocol};
}

static std::string get_content_type(const std::string &ext)
{
    std::string content_type;
    if (content_types.find(ext) == content_types.end())
    {
        content_type = "text/plain";
    }
    else
    {
        content_type = content_types.at(ext);
    }
    return content_type;
}

static int check_host(std::unordered_map<std::string, std::string> headers)
{
    int status = 200;
    if (headers.find("Host") == headers.end())
    {
        status = 404;
    }
    else
    {
        auto host = headers.at("Host");
        auto port = std::to_string(SERVER_PORT);
        if (host != "127.0.0.1:" + port && host != "localhost:" + port && host != "127.0.0.1:" + port + "\r" &&
            host != "localhost:" + port + "\r")
        {
            status = 404;
        }
    }
    return status;
}

static std::string get_extension(const std::string &path)
{
    auto dot_pos = path.find('.');
    auto ext = path.substr(dot_pos, path.length() - dot_pos);
    return ext;
}

std::string string_to_hex(const std::string &input)
{
    constexpr char HEX_ALPHABET[] = "0123456789ABCDEF";

    std::string output;
    output.reserve(input.length() * 2);
    for (unsigned char c : input)
    {
        output.push_back(HEX_ALPHABET[c >> 4]);
        output.push_back(HEX_ALPHABET[c & 15]);
    }

    return output;
}

static std::string get_response(const Client &client, const std::string &request)
{
    std::string response;
    int status = 200;
    std::string status_string = "OK";
    std::unordered_map<std::string, std::string> response_headers;
    response_headers["Connection"] = "close";
    std::string body;

    auto line_end = request.find('\n');
    auto line = request.substr(0, line_end);
    printf("\n[%s]  request: %s\n", client.ip.c_str(), line.c_str());
    std::cout << request << std::endl;
    printf("[%s] response: ", client.ip.c_str());

    auto headers = get_headers(request.substr(line_end + 1, request.length() - line_end));
    status = check_host(headers);
    if (status != 200)
    {
        response = response_format("HTTP/1.1", status, response_headers, body);
        return response;
    }

    auto [method, path, protocol] = split_start_request_line(line);
    if (method != "GET" || protocol != "HTTP/1.1")
    {
        status = 405;
        response = response_format(protocol, status, response_headers, body);
        return response;
    }

    std::ifstream infile{path};
    if (!infile.good())
    {
        status = 404;
        response = response_format(protocol, status, response_headers, body);
        return response;
    }
    body = std::string{std::istreambuf_iterator<char>(infile), std::istreambuf_iterator<char>()};
    infile.close();

    response_headers["Content-Type"] = get_content_type(get_extension(path));
    auto idx = response_headers["Content-Type"].find("image");
    if (idx != std::string::npos)
    {
        body = string_to_hex(body);
    }
    response_headers["Content-Length"] = std::to_string(body.length());

    response = response_format(protocol, status, response_headers, body);

    return response;
}

static void handle_function(const Client &client)
{
    char buff[MSG_LEN + 1];
    bzero(buff, sizeof buff);
    read(client.socket, buff, sizeof buff);

    std::string request = buff;
    auto response = get_response(client, request);

    write(client.socket, response.c_str(), response.size());
}

void *thread_function(__attribute__((unused)) void *argv)
{
    while (!stop)
    {
        Client *pclient = nullptr;

        pthread_mutex_lock(&mutex);
        if (queue.empty())
        {
            pthread_cond_wait(&cond, &mutex);
            pclient = &queue.front();
            queue.pop();
        }
        pthread_mutex_unlock(&mutex);

        if (pclient)
        {
            handle_function(*pclient);
        }
    }
    return nullptr;
}

void new_client(const sockaddr_in &client_addr, int conn_fd)
{
    pthread_mutex_lock(&mutex);
    queue.emplace(client_addr, conn_fd);
    pthread_cond_signal(&cond);
    pthread_mutex_unlock(&mutex);
}

void sighandler(int signum)
{
    cancel_threads();
    close(server_sd);
    printf("[[Stop by %d]]\n", signum);
    exit(EXIT_SUCCESS);
}

int shutdown_server(const char *str)
{
    cancel_threads();
    close(server_sd);
    perror(str);
    return EXIT_FAILURE;
}
