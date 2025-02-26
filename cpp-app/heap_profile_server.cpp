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

// Naive function that blocks and handles a single connection
// (again, not production-ready).
void HandleClient(int clientSock) {
    // Minimal parsing: we'll only look for GET /debug/pprof/heap
    // (In reality you'd parse the HTTP properly).
    const int bufferSize = 4096;
    char buffer[bufferSize] = {0};
    ssize_t bytesRead = recv(clientSock, buffer, bufferSize - 1, 0);
    if (bytesRead <= 0) {
        close(clientSock);
        return;
    }

    // Convert to a C-string just in case.
    buffer[bytesRead] = '\0';

    // Check if it's the path we want:
    // "GET /debug/pprof/heap"
    std::string request(buffer);
    if (request.find("GET /debug/pprof/heap") != std::string::npos) {
        // Grab the heap profile from gperftools
        const char* profileData = GetHeapProfile();
        if (profileData) {
            // Construct a minimal HTTP response
            std::string response = 
                "HTTP/1.1 200 OK\r\n"
                "Content-Type: text/plain\r\n"
                "Connection: close\r\n\r\n";

            // Append the actual profile data
            response += profileData;

            // Send the response
            send(clientSock, response.c_str(), response.size(), 0);

            // Must free memory allocated by GetHeapProfile()
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
        // Not the path we want
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
    // Start the heap profiler with some prefix (file output is optional)
    // This also writes .heap files to /tmp, but we won't rely on them directly.
    HeapProfilerStart("/tmp/heap_profile_example");

    // Allocate some memory so we have something to see in the profile
    std::vector<int> bigVec;
    bigVec.reserve(10'000'000);
    for (int i = 0; i < 10'000'000; ++i) {
        bigVec.push_back(i);
    }

    std::cout << "Memory allocated. Starting naive HTTP server on port 8080.\n";
    std::cout << "Visit http://localhost:8080/debug/pprof/heap to get the profile.\n";

    // Socket setup
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
    addr.sin_addr.s_addr = INADDR_ANY; // 0.0.0.0
    addr.sin_port = htons(8080);

    if (bind(serverSock, (struct sockaddr*)&addr, sizeof(addr)) < 0) {
        std::cerr << "Failed to bind.\n";
        return 1;
    }

    if (listen(serverSock, 10) < 0) {
        std::cerr << "Failed to listen.\n";
        return 1;
    }

    // Accept loop (single-threaded)
    while (true) {
        sockaddr_in clientAddr;
        socklen_t clientLen = sizeof(clientAddr);
        int clientSock = accept(serverSock, (struct sockaddr*)&clientAddr, &clientLen);
        if (clientSock < 0) {
            std::cerr << "Failed to accept client.\n";
            continue;
        }
        // Handle the request (blocking)
        HandleClient(clientSock);
    }

    // We'll never reach this in the example, but for completeness:
    HeapProfilerStop();
    close(serverSock);
    return 0;
}

