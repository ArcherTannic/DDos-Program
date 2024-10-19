#include <iostream>
#include <winsock2.h>
#include <ws2tcpip.h>
#include <windows.h>
#include <thread>
#include <vector>
#include <mutex>

#pragma comment(lib, "Ws2_32.lib")

std::mutex mtx; // Мьютекс для защиты вывода

void printServerStatus(const std::string& ipAddress, bool isServerDown) {
    std::lock_guard<std::mutex> lock(mtx); // Защита вывода
    if (isServerDown) {
        std::cout << "Server " << ipAddress << " is down" << std::endl;
    } else {
        std::cout << "Server " << ipAddress << " is up" << std::endl;
    }
}

bool sendSYN(const std::string& ipAddress, int port) {
    SOCKET sock;
    sockaddr_in serverAddr;

    sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (sock == INVALID_SOCKET) {
        return false;
    }

    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons(port);
    inet_pton(AF_INET, ipAddress.c_str(), &serverAddr.sin_addr);

    int result = connect(sock, (sockaddr*)&serverAddr, sizeof(serverAddr));
    closesocket(sock);

    return result != SOCKET_ERROR;
}

bool sendUDP(const std::string& ipAddress, int port) {
    SOCKET sock;
    sockaddr_in serverAddr;

    sock = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    if (sock == INVALID_SOCKET) {
        return false;
    }

    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons(port);
    inet_pton(AF_INET, ipAddress.c_str(), &serverAddr.sin_addr);

    char buffer[1] = { 0 };
    int result = sendto(sock, buffer, sizeof(buffer), 0, (sockaddr*)&serverAddr, sizeof(serverAddr));
    closesocket(sock);

    return result != SOCKET_ERROR;
}

void testServer(const std::string& ipAddress, int numRequests, int port) {
    for (int i = 0; i < numRequests || numRequests == -1; ++i) {
        bool synResult = sendSYN(ipAddress, port);  // SYN на указанный порт
        bool udpResult = sendUDP(ipAddress, port);  // UDP на указанный порт

        printServerStatus(ipAddress, !(synResult && udpResult));
    }
}

int main() {
    WSADATA wsaData;
    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
        std::cerr << "WSAStartup failed" << std::endl;
        return 1;
    }

    std::string ipAddress;
    int numRequests;
    int numThreads;

    std::cout << "Enter IP address of the server: ";
    std::cin >> ipAddress;

    std::cout << "Enter number of requests (-1 for infinite): ";
    std::cin >> numRequests;

    std::cout << "Enter number of threads to use: ";
    std::cin >> numThreads;

    if (numRequests == -1) {
        std::cout << "Warning: Infinite requests may cause poor network and system performance!" << std::endl;
    }

    std::vector<std::thread> threads;

    for (int i = 0; i < numThreads; ++i) {
        threads.emplace_back(testServer, ipAddress, numRequests, 80); // Порт 80 (HTTP)
    }

    for (auto& thread : threads) {
        thread.join(); // Ожидание завершения всех потоков
    }

    WSACleanup();
    return 0;
}
