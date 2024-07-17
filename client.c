#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>

#define PORT 8080

int main() {
    // Socket variables
    int sock = 0;
    struct sockaddr_in serv_addr;
    char buffer[1024] = {0};

    // Create socket file descriptor
    sock = socket(AF_INET, SOCK_STREAM, 0);

    // Set server address
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(PORT);
    inet_pton(AF_INET, "127.0.0.1", &serv_addr.sin_addr);

    // Connect to the server
    connect(sock, (struct sockaddr *)&serv_addr, sizeof(serv_addr));

    // Continuously read and print random numbers from server
    while (1) {
        int valread = read(sock, buffer, sizeof(buffer));
        if (valread > 0) {
            buffer[valread] = '\0';  // Null-terminate the string
            printf("%s", buffer);
        }
    }

    close(sock);
    return 0;
}
