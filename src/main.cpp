#include <iostream>
// #include <cstring>
#include <unistd.h>
#include <netinet/in.h>

char handle_greeting_request(int client_fd)
{
    unsigned char buffer[1024];

    ssize_t n = recv(client_fd, buffer, sizeof(buffer) - 1, 0);

    std::cout << "received " << n << " bytes" << std::endl;

    unsigned char ver = buffer[0];
    std::cout << "ver: " << static_cast<int>(ver) << std::endl;

    unsigned char nauth = buffer[1];
    std::cout << "nauth: " << static_cast<int>(nauth) << std::endl;

    for (int i = 0; i < nauth; i++)
    {
        unsigned char auth = buffer[2 + i];
        std::cout << "auth: " << static_cast<int>(auth) << std::endl;

        if (auth == 2)
        {
            unsigned char response[2] = {ver, 0x02};
            ssize_t n = send(client_fd, response, sizeof(response), 0);

            if (n == -1)
            {
                std::cout << "unable to send" << std::endl;

                return -1;
            }

            return 0x02;
        }
    }

    unsigned char response[2] = {ver, 0xff};
    send(client_fd, response, sizeof(response), 0);

    return -1;
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

    // const char *message = "Hello from C++ TCP server!\n";
    // send(client_fd, message, strlen(message), 0);

    close(client_fd);
    close(server_fd);

    return 0;
}