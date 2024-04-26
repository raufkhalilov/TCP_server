#include <iostream>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <cstring>

constexpr int PORT = 8080;
constexpr int BUFFER_SIZE = 1024;

class CircularBuffer {
private:
    char *buffer;
    int capacity;
    int front;
    int rear;
    int count;

public:
    CircularBuffer(int size) {
        capacity = size;
        buffer = new char[capacity];
        front = 0;
        rear = 0;
        count = 0;
    }

    ~CircularBuffer() {
        delete[] buffer;
    }

    void write(const char *data, int length) {
        for (int i = 0; i < length; ++i) {
            buffer[rear] = data[i];
            rear = (rear + 1) % capacity;
            if (count == capacity) {
                front = (front + 1) % capacity;
            } else {
                ++count;
            }
        }
    }

    int read(char *data, int length) {
        int bytesRead = 0;
        while (bytesRead < length && count > 0) {
            data[bytesRead] = buffer[front];
            front = (front + 1) % capacity;
            ++bytesRead;
            --count;
        }
        return bytesRead;
    }
};

int main() {
    int serverSocket, newSocket;
    struct sockaddr_in serverAddr, clientAddr;
    socklen_t addrSize = sizeof(struct sockaddr_in);

    if ((serverSocket = socket(AF_INET, SOCK_STREAM, 0)) == 0) {
        perror("Socket creation failed");
        return -1;
    }

    serverAddr.sin_family = AF_INET;
    serverAddr.sin_addr.s_addr = INADDR_ANY;
    serverAddr.sin_port = htons(PORT);

    if (bind(serverSocket, (struct sockaddr *)&serverAddr, sizeof(serverAddr)) < 0) {
        perror("Bind failed");
        return -1;
    }

    if (listen(serverSocket, 5) < 0) {
        perror("Listen failed");
        return -1;
    }

    std::cout << "Server listening on port " << PORT << std::endl;

    if ((newSocket = accept(serverSocket, (struct sockaddr *)&clientAddr, &addrSize)) < 0) {
        perror("Accept failed");
        return -1;
    }

    std::cout << "Client connected" << std::endl;

    CircularBuffer buffer(BUFFER_SIZE);

    while (true) {
        char recvBuffer[BUFFER_SIZE] = {0};
        int bytesRead = recv(newSocket, recvBuffer, BUFFER_SIZE, 0);
        if (bytesRead < 0) {
            perror("Receive failed");
            break;
        } else if (bytesRead == 0) {
            std::cout << "Client disconnected" << std::endl;
            break;
        }

        buffer.write(recvBuffer, bytesRead);

        // Отправка подтверждения клиенту
        const char *ackMsg = "Message received";
        if (send(newSocket, ackMsg, strlen(ackMsg), 0) < 0) {
            perror("Send failed");
            break;
        }

        char processBuffer[BUFFER_SIZE];
        int processedBytes = buffer.read(processBuffer, BUFFER_SIZE);
        if (processedBytes > 0) {
            std::cout << "Processed message: " << processBuffer << std::endl;
        }
    }

    close(newSocket);
    close(serverSocket);

    return 0;
}
