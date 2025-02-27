# CMakeLists.txt
cmake_minimum_required(VERSION 3.16)
project(heap_profile_server)

set(CMAKE_CXX_STANDARD 17)

# Find the required packages
find_package(opentelemetry-cpp REQUIRED COMPONENTS trace exporter_otlp_grpc sdk resources common proto)
find_package(Protobuf REQUIRED)
find_package(gRPC CONFIG REQUIRED)
find_package(CURL REQUIRED)

# Define the executable
add_executable(heap_profile_server heap_profile_server.cpp)

# Link against OpenTelemetry and other required libraries
target_link_libraries(heap_profile_server PRIVATE
    tcmalloc_and_profiler
    opentelemetry_trace
    opentelemetry_exporter_otlp_grpc
    opentelemetry_resources
    opentelemetry_common
    opentelemetry_proto
    opentelemetry_version
    protobuf
    grpc++
    grpc
    curl
    pthread
)

# Ensure the linker knows where to find libraries
link_directories(/usr/lib)

# Verbose linking output to help debug
set(CMAKE_VERBOSE_MAKEFILE ON)
