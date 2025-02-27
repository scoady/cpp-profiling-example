#include <gperftools/heap-profiler.h>
#include <iostream>
#include <vector>
#include <thread>
#include <cstring>
#include <cstdlib>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include <opentelemetry/exporters/otlp/otlp_http_exporter.h>
#include <opentelemetry/sdk/trace/simple_processor.h>
#include <opentelemetry/sdk/trace/tracer_provider.h>
#include <opentelemetry/sdk/trace/tracer_context.h>
#include <opentelemetry/sdk/resource/resource.h>
#include <opentelemetry/trace/provider.h>
#include <opentelemetry/trace/scope.h>

namespace trace = opentelemetry::trace;
namespace sdktrace = opentelemetry::sdk::trace;
namespace nostd = opentelemetry::nostd;

void InitOpenTelemetry() {
    auto exporter = std::make_unique<opentelemetry::exporter::otlp::OtlpHttpExporter>();

    std::vector<std::unique_ptr<sdktrace::SpanProcessor>> processors;
    processors.emplace_back(
        std::make_unique<sdktrace::SimpleSpanProcessor>(std::move(exporter))
    );

    auto resource = opentelemetry::sdk::resource::Resource::Create({{"service.name", "cpp-app"}});

    auto context = std::make_unique<sdktrace::TracerContext>(
        std::move(processors), 
        resource
    );

    auto provider = nostd::shared_ptr<trace::TracerProvider>(
        new sdktrace::TracerProvider(std::move(context))
    );

    trace::Provider::SetTracerProvider(provider);
}


nostd::shared_ptr<trace::Tracer> get_tracer() {
    auto provider = trace::Provider::GetTracerProvider();
    return provider->GetTracer("cpp-app", "1.0.0");
}

void HandleClient(int clientSock) {
    auto tracer = get_tracer();
    auto span = tracer->StartSpan("HandleClient");
    trace::Scope scope(span);

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
        const char* profileData = GetHeapProfile();
        if (profileData) {
            std::string response =
                "HTTP/1.1 200 OK\r\n"
                "Content-Type: text/plain\r\n"
                "Connection: close\r\n\r\n";
            response += profileData;
            send(clientSock, response.c_str(), response.size(), 0);
            delete[] profileData;
        }
    }
    close(clientSock);
    span->End();
}

int main() {
    InitOpenTelemetry();

    HeapProfilerStart("/tmp/heap_profile_example");

    std::vector<int> bigVec;
    bigVec.reserve(10'000'000);
    for (int i = 0; i < 10'000'000; ++i) {
        bigVec.push_back(i);
    }

    int serverSock = socket(AF_INET, SOCK_STREAM, 0);
    sockaddr_in addr;
    std::memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = INADDR_ANY;
    addr.sin_port = htons(8080);

    bind(serverSock, (struct sockaddr*)&addr, sizeof(addr));
    listen(serverSock, 10);

    while (true) {
        sockaddr_in clientAddr;
        socklen_t clientLen = sizeof(clientAddr);
        int clientSock = accept(serverSock, (struct sockaddr*)&clientAddr, &clientLen);
        if (clientSock >= 0) {
            HandleClient(clientSock);
        }
    }

    HeapProfilerStop();
    close(serverSock);
    return 0;
}
