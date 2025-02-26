#include <gperftools/heap-profiler.h>
#include <iostream>
#include <vector>
#include <thread>
#include <cstring>
#include <cstdlib>
#include <unistd.h>     // for close()
#include <sys/socket.h> // for socket stuff
#include <netinet/in.h> // for sockaddr_in
#include <arpa/inet.h>  // for INADDR_ANY

// Intentionally leaked memory
std::vector<int*> leakedMemory;

void SimulateMemoryLeak() {
    // Allocate 1 MB of memory and never free it (memory leak!)
    int* leakedChunk = new int[256 * 1024]; // 1 MB (256k * 4 bytes)
    leakedMemory.push_back(leakedChunk);
    std::cout << "Leaked 1 MB of memory. Total leaks: " << leakedMemory.size() << " MB\n";
}

// Naive function that blocks and handles a single connection
void HandleClient(int clientSock) {
    const int bufferSize = 4096;
    char buffer[bufferSize] = {0};
    ssize_t bytesRead = recv(clientSock, buffer, bufferSize - 1, 0);
    if (bytesRead <= 0) {
        close(clientSock);
        return;
    }

    buffer[bytesRead] = '\0';
    std::string request(buffer);

    if (request.find("GET /debug/pprof/heap") != std::string::npos) {
        // Simulate a memory leak on each request
        SimulateMemoryLeak();

        // Grab the heap profile from gperftools
        const char* profileData = GetHeapProfile();
        if (profileData) {
            std::string response = 
                "HTTP/1.1 200 OK\r\n"
                "Content-Type: text/plain\r\n"
                "Connection: close\r\n\r\n";
            response += profileData;
            send(clientSock, response.c_str(), response.size(), 0);
            delete[] profileData;
        } else {
            std::string errResponse = 
                "HTTP/1.1 500 Internal Server Error\r\n"
                "Content-Type: text/plain\r\n"
                "Connection: close\r\n\r\n"
                "Failed to get heap profile.\n";
            send(clientSock, errResponse.c_str(), errResponse.size(), 0);
        }
    } else {
        std::string notFound = 
            "HTTP/1.1 404 Not Found\r\n"
            "Content-Type: text/plain\r\n"
            "Connection: close\r\n\r\n"
            "Unknown endpoint.\n";
        send(clientSock, notFound.c_str(), notFound.size(), 0);
    }

    close(clientSock);
}

int main() {
    HeapProfilerStart("/tmp/heap_profile_example");

    std::cout << "Memory allocated. Starting naive HTTP server on port 8080.\n";
    std::cout << "Visit http://localhost:8080/debug/pprof/heap to get the profile.\n";

    int serverSock = socket(AF_INET, SOCK_STREAM, 0);
    if (serverSock < 0) {
        std::cerr << "Failed to create socket.\n";
        return 1;
    }

    int opt = 1;
    setsockopt(serverSock, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    sockaddr_in addr;
    std::memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = INADDR_ANY;
    addr.sin_port = htons(8080);

    if (bind(serverSock, (struct sockaddr*)&addr, sizeof(addr)) < 0) {
        std::cerr << "Failed to bind.\n";
        return 1;
    }

    if (listen(serverSock, 10) < 0) {
        std::cerr << "Failed to listen.\n";
        return 1;
    }

    while (true) {
        sockaddr_in clientAddr;
        socklen_t clientLen = sizeof(clientAddr);
        int clientSock = accept(serverSock, (struct sockaddr*)&clientAddr, &clientLen);
        if (clientSock < 0) {
            std::cerr << "Failed to accept client.\n";
            continue;
        }
        HandleClient(clientSock);
    }

    HeapProfilerStop();
    close(serverSock);
    return 0;
}
