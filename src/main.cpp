#include <iostream>
// #include <cstring>
#include <unistd.h>
#include <netinet/in.h>

int main() {
    std::cout << "Initializing..." << std::endl;

    int server_fd = socket(AF_INET, SOCK_STREAM, 0);

    if (server_fd == -1) {
        std::cerr << "Unable to create socket\n";
        return 1;
    }

    sockaddr_in address{};
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(8080);

    sockaddr_in* address_pointer = &address;

    if (bind(server_fd, reinterpret_cast<sockaddr*>(address_pointer), sizeof(address)) < 0) {
        std::cerr << "Unable to bind socket\n";
        close(server_fd);
        return 1;
    }

    if (listen(server_fd, 1) < 0) {
        std::cerr << "Unable to listen\n";
        close(server_fd);
        return 1;
    }

    std::cout << "Listening..." << std::endl;

    int client_fd = accept(server_fd, nullptr, nullptr);

    if (client_fd < 0) {
        std::cerr << "Unable to accept client\n";
        close(server_fd);
        return 1;
    }

    std::cout << "Connected..." << std::endl;

    const char* message = "Hello from C++ TCP server!\n";
    send(client_fd, message, strlen(message), 0);

    close(client_fd);
    close(server_fd);

    return 0;
}