# CMakeLists.txt
cmake_minimum_required(VERSION 3.16)
project(heap_profile_server)

set(CMAKE_CXX_STANDARD 17)

# Include directories for OpenTelemetry
include_directories(/opt/otel-cpp/include)

# Link against OpenTelemetry libraries
link_directories(/opt/otel-cpp/lib)

# Define the executable
add_executable(heap_profile_server heap_profile_server.cpp)

# Link against OpenTelemetry and other required libraries
target_link_libraries(heap_profile_server PRIVATE
    tcmalloc_and_profiler
    opentelemetry_trace
    opentelemetry_exporter_otlp_http
    opentelemetry_resources
    opentelemetry_common
    opentelemetry_proto
    opentelemetry_sdk
    protobuf
    curl
    pthread
)
