FROM ubuntu:22.04

RUN apt-get update && apt-get install -y \
    g++ \
    libgoogle-perftools-dev \
    google-perftools \
    && rm -rf /var/lib/apt/lists/*

WORKDIR /app

# Copy and build your C++ code
COPY heap_profile_server.cpp .

RUN g++ -std=c++17 heap_profile_server.cpp -o heap_profile_server -ltcmalloc_and_profiler

# Make a local directory where we can copy the binary
RUN mkdir /build-output

# Copy our compiled binary to /build-output
# Because we've declared "cxx-binary-volume:/build-output" in docker-compose,
# this step will make the binary visible in that volume at runtime.
RUN cp heap_profile_server /build-output/heap_profile_server

EXPOSE 8080
CMD ["./heap_profile_server"]
