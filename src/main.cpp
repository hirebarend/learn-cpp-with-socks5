#include <iostream>
// #include <cstring>
#include <unistd.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <thread>

char handle_greeting_request(int client_fd)
{
    char buffer[256];

    ssize_t n = recv(client_fd, buffer, sizeof(buffer), 0);

    std::cout << "received " << n << " bytes" << std::endl;

    char ver = buffer[0];
    std::cout << "ver: " << static_cast<int>(ver) << std::endl;

    char nauth = buffer[1];
    std::cout << "nauth: " << static_cast<int>(nauth) << std::endl;

    for (int i = 0; i < nauth; i++)
    {
        char auth = buffer[2 + i];
        std::cout << "auth: " << static_cast<int>(auth) << std::endl;

        if (auth == 2)
        {
            char response[2] = {ver, 0x02};
            ssize_t n = send(client_fd, response, sizeof(response), 0);

            if (n == -1)
            {
                std::cout << "unable to send" << std::endl;

                return -1;
            }

            return 0x02;
        }
    }

    unsigned response[2] = {static_cast<unsigned char>(ver), 0xff};
    send(client_fd, response, sizeof(response), 0);

    return -1;
}

char handle_authentication_username_password_request(int client_fd)
{
    char buffer[512];

    ssize_t n = recv(client_fd, buffer, sizeof(buffer), 0);

    std::cout << "received " << n << " bytes" << std::endl;

    char ver = buffer[0];
    std::cout << "ver: " << static_cast<int>(ver) << std::endl;

    char id_len = buffer[1];
    std::cout << "id_len: " << static_cast<int>(id_len) << std::endl;

    std::string id(
        &buffer[2],
        id_len);

    std::cout << "id: " << id << std::endl;

    char pw_len = buffer[2 + id_len];
    std::cout << "pw_len: " << static_cast<int>(pw_len) << std::endl;

    std::string pw(
        &buffer[3 + id_len],
        pw_len);

    std::cout << "pw: " << pw << std::endl;

    unsigned char response[2] = {static_cast<unsigned char>(ver), 0x00};
    send(client_fd, response, sizeof(response), 0);

    return 0;
}

int handle_connection_request(int client_fd)
{
    char buffer[512];

    ssize_t n = recv(client_fd, buffer, sizeof(buffer), 0);

    std::cout << "received " << n << " bytes" << std::endl;

    char ver = buffer[0];
    std::cout << "ver: " << static_cast<int>(ver) << std::endl;

    char cmd = buffer[1];
    std::cout << "cmd: " << static_cast<int>(cmd) << std::endl;

    char rsv = buffer[2];
    std::cout << "rsv: " << static_cast<int>(rsv) << std::endl;

    char atyp = buffer[3];
    std::cout << "atyp: " << static_cast<int>(atyp) << std::endl;

    std::string host;
    unsigned short port = 0;

    if (atyp == 0x01)
    {
        char addr[INET_ADDRSTRLEN];
        inet_ntop(AF_INET, &buffer[4], addr, sizeof(addr));
        host = std::string(addr);

        unsigned char p1 = static_cast<unsigned char>(buffer[8]);
        unsigned char p2 = static_cast<unsigned char>(buffer[9]);
        port = (p1 << 8) | p2;
    }
    else if (atyp == 0x03)
    {
        unsigned char domain_len = static_cast<unsigned char>(buffer[4]);
        std::cout << "domain_len: " << static_cast<int>(domain_len) << std::endl;

        host = std::string(
            &buffer[5],
            domain_len);

        unsigned char p1 = static_cast<unsigned char>(buffer[5 + domain_len]);
        unsigned char p2 = static_cast<unsigned char>(buffer[6 + domain_len]);
        port = (p1 << 8) | p2;
    }

    std::cout << "host: " << host << std::endl;
    std::cout << "port: " << port << std::endl;

    std::string port_str = std::to_string(port);

    addrinfo hints{};
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;

    addrinfo *result = nullptr;
    int gai = getaddrinfo(host.c_str(), port_str.c_str(), &hints, &result);

    if (gai != 0)
    {
        std::cout << "unable to resolve host" << std::endl;

        unsigned char response[10] = {0x05, 0x01, 0x00, 0x01, 0, 0, 0, 0, 0, 0};
        send(client_fd, response, sizeof(response), 0);

        return -1;
    }

    int remote_fd = socket(result->ai_family, result->ai_socktype, result->ai_protocol);

    if (remote_fd == -1)
    {
        std::cout << "unable to create remote socket" << std::endl;
        freeaddrinfo(result);

        unsigned char response[10] = {0x05, 0x01, 0x00, 0x01, 0, 0, 0, 0, 0, 0};
        send(client_fd, response, sizeof(response), 0);

        return -1;
    }

    if (connect(remote_fd, result->ai_addr, result->ai_addrlen) < 0)
    {
        std::cout << "unable to connect to remote" << std::endl;
        close(remote_fd);
        freeaddrinfo(result);

        unsigned char response[10] = {0x05, 0x01, 0x00, 0x01, 0, 0, 0, 0, 0, 0};
        send(client_fd, response, sizeof(response), 0);

        return -1;
    }

    freeaddrinfo(result);

    std::cout << "connected to remote" << std::endl;

    unsigned char response[10] = {0x05, 0x00, 0x00, 0x01, 0, 0, 0, 0, 0, 0};
    send(client_fd, response, sizeof(response), 0);

    return remote_fd;
}

void relay(int from_fd, int to_fd)
{
    char buffer[4096];

    while (true)
    {
        ssize_t n = recv(from_fd, buffer, sizeof(buffer), 0);

        std::cout << "relay " << from_fd << " -> " << to_fd << ": " << n << " bytes" << std::endl;

        if (n <= 0)
        {
            break;
        }

        send(to_fd, buffer, n, 0);
    }

    shutdown(to_fd, SHUT_WR);
}

int main()
{
    std::cout << "initializing..." << std::endl;

    int server_fd = socket(AF_INET, SOCK_STREAM, 0);

    if (server_fd == -1)
    {
        std::cerr << "Unable to create socket\n";
        return 1;
    }

    sockaddr_in address{};
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(8080);

    sockaddr_in *address_pointer = &address;

    if (bind(server_fd, reinterpret_cast<sockaddr *>(address_pointer), sizeof(address)) < 0)
    {
        std::cerr << "unable to bind socket\n";
        close(server_fd);
        return 1;
    }

    if (listen(server_fd, 1) < 0)
    {
        std::cerr << "unable to listen\n";
        close(server_fd);
        return 1;
    }

    std::cout << "listening..." << std::endl;

    int client_fd = accept(server_fd, nullptr, nullptr);

    if (client_fd < 0)
    {
        std::cerr << "unable to accept client\n";
        close(server_fd);
        return 1;
    }

    std::cout << "connected..." << std::endl;

    char auth = handle_greeting_request(client_fd);

    if (auth == -1)
    {
        std::cout << "unable to handle greeting request" << std::endl;
        close(client_fd);
        close(server_fd);

        return -1;
    }

    char result = handle_authentication_username_password_request(client_fd);

    int remote_fd = handle_connection_request(client_fd);

    if (remote_fd == -1)
    {
        std::cout << "unable to handle connection request" << std::endl;
        close(client_fd);
        close(server_fd);

        return -1;
    }

    std::thread t1(relay, client_fd, remote_fd);
    std::thread t2(relay, remote_fd, client_fd);
    t1.join();
    t2.join();

    close(remote_fd);
    close(client_fd);
    close(server_fd);

    return 0;
}