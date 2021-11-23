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
static pthread_t poolOfThreads[NUMBER_OF_THREADS];

void threadsCreation()
{
    for (int i = 0; i < NUMBER_OF_THREADS; i++)
    {
        pthread_create(poolOfThreads + i, nullptr, threadFun, nullptr);
    }
}

void threadsRemove()
{
    stop = true;
    for (int i = 0; i < NUMBER_OF_THREADS; i++)
    {
        pthread_cancel(poolOfThreads[i]);
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

static const std::unordered_map<std::string, std::string> typesOfContent = {
    {".html", "text/html"},
    {".jpg", "image/jpeg"},
};

static std::unordered_map<std::string, std::string> getMapOfHeaders(const std::string &header)
{
    std::unordered_map<std::string, std::string> headers;
    std::istringstream iss{header};
    for (std::string line; std::getline(iss, line) && line != "\n";)
    {
        size_t end = line.find(':');

        std::string key = line.substr(0, end);
        std::string value = line.substr(end + 2, line.length() - end - 1);

        headers[key] = value;
    }
    return headers;
}

static std::string setFirstResponseLine(const std::string &protocol, int status)
{
    const std::unordered_map<int, std::string> statuses = {
        {200, "OK"},
        {404, "Not Found"},
        {405, "Method Not Allowed"},
    };

    return protocol + " " + std::to_string(status) + " " + statuses.at(status);
}

static std::string createResponse(const std::string &protocol, int status,
                                  std::unordered_map<std::string, std::string> headers, std::string body)
{
    std::string response = setFirstResponseLine(protocol, status) + "\n";
    std::cout << response;

    if (status != 200)
    {
        std::ifstream infile{"public/errors/" + std::to_string(status) + ".html"};
        body = std::string{std::istreambuf_iterator<char>(infile), std::istreambuf_iterator<char>()};
        infile.close();
        headers["Content-Type"] = typesOfContent.at(".html");
        headers["Content-Length"] = std::to_string(body.length());
    }

    for (const std::string &header : std::array<std::string, 3>{"Content-Type", "Content-Length", "Connection"})
    {
        if (headers.find(header) != headers.end())
        {
            response += header + ": " + headers.at(header) + "\n";
        }
    }

    response += "\n" + body;
    return response;
}

static std::tuple<std::string, std::string, std::string> parseStartRequestLine(const std::string &line)
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

static std::string getContentType(const std::string &ext)
{
    std::string type;
    if (typesOfContent.find(ext) == typesOfContent.end())
    {
        type = "text/plain";
    }
    else
    {
        type = typesOfContent.at(ext);
    }
    return type;
}

static int getStatusForHost(std::unordered_map<std::string, std::string> headers)
{
    int status = 200;
    if (headers.find("Host") == headers.end())
    {
        status = 404;
    }
    else
    {
        std::string host = headers.at("Host");
        std::string port = std::to_string(SERVER_PORT);
        if (host != "127.0.0.1:" + port && host != "localhost:" + port && host != "127.0.0.1:" + port + "\r" &&
            host != "localhost:" + port + "\r")
        {
            status = 404;
        }
    }
    return status;
}

static std::string getExtension(const std::string &path)
{
    auto dot_pos = path.find('.');
    auto ext = path.substr(dot_pos, path.length() - dot_pos);
    return ext;
}

std::string inHex(const std::string &input)
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

static std::string createResponse(const Client &client, const std::string &request)
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

    auto headers = getMapOfHeaders(request.substr(line_end + 1, request.length() - line_end));
    status = getStatusForHost(headers);
    if (status != 200)
    {
        response = createResponse("HTTP/1.1", status, response_headers, body);
        return response;
    }

    auto [method, path, protocol] = parseStartRequestLine(line);
    if (method != "GET" || protocol != "HTTP/1.1")
    {
        status = 405;
        response = createResponse(protocol, status, response_headers, body);
        return response;
    }

    std::ifstream infile{path};
    if (!infile.good())
    {
        status = 404;
        response = createResponse(protocol, status, response_headers, body);
        return response;
    }
    body = std::string{std::istreambuf_iterator<char>(infile), std::istreambuf_iterator<char>()};
    infile.close();

    response_headers["Content-Type"] = getContentType(getExtension(path));
    auto idx = response_headers["Content-Type"].find("image");
    if (idx != std::string::npos)
    {
        body = inHex(body);
    }
    response_headers["Content-Length"] = std::to_string(body.length());

    response = createResponse(protocol, status, response_headers, body);

    return response;
}

static void clientHandler(const Client &client)
{
    char buff[BUFFER_SIZE + 1];
    bzero(buff, sizeof buff);
    read(client.socket, buff, sizeof buff);

    std::string request = buff;
    auto response = createResponse(client, request);

    write(client.socket, response.c_str(), response.size());
}

void *threadFun(__attribute__((unused)) void *argv)
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
            clientHandler(*pclient);
        }
    }
    return nullptr;
}

void addClientToQueue(const sockaddr_in &client_addr, int conn_fd)
{
    pthread_mutex_lock(&mutex);
    queue.emplace(client_addr, conn_fd);
    pthread_cond_signal(&cond);
    pthread_mutex_unlock(&mutex);
}

void signalHandler(int signum)
{
    threadsRemove();
    close(server_sd);
    printf("Good bye!\n");
    exit(EXIT_SUCCESS);
}

int exitOnServerError(const char *str)
{
    threadsRemove();
    close(server_sd);
    perror(str);
    return EXIT_FAILURE;
}
