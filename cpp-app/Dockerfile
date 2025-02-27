# Stage 1: Base environment for building
FROM alpine:latest AS base

# Add edge/testing repository and install dependencies
RUN echo "http://dl-cdn.alpinelinux.org/alpine/edge/testing" >> /etc/apk/repositories \
    && apk add --no-cache \
    build-base \
    cmake \
    git \
    curl \
    curl-dev \
    protobuf-dev \
    grpc-dev \
    opentelemetry-cpp-dev \
    autoconf \
    automake \
    libtool

# Clone and build gperftools
WORKDIR /tmp
RUN git clone https://github.com/gperftools/gperftools.git \
    && cd gperftools \
    && ./autogen.sh \
    && ./configure \
    && make -j$(nproc) \
    && make install 



WORKDIR /app

# Stage 2: Build the C++ application
FROM base AS cpp-builder

# Copy the C++ source code
COPY heap_profile_server.cpp .

# Build the C++ application using g++
RUN g++ -std=c++17 heap_profile_server.cpp -o heap_profile_server \
    -ltcmalloc_and_profiler \
    -lopentelemetry_trace \
    -lopentelemetry_exporter_otlp_grpc \
    -lopentelemetry_resources \
    -lopentelemetry_common \
    -lopentelemetry_proto \
    -lprotobuf \
    -lgrpc++ \
    -lgrpc \
    -lcurl \
    -lpthread \
    -labsl_log_internal_check_op \
    -labsl_log_severity \
    -labsl_log_internal_globals \
    -labsl_log_internal_message \
    -labsl_str_format_internal \
    -labsl_strings \
    -labsl_base \
    -labsl_synchronization \
    -labsl_time

# Stage 3: Runtime image for the C++ app
FROM alpine:latest AS cpp-app

# Add edge/testing repository and install runtime dependencies
RUN echo "http://dl-cdn.alpinelinux.org/alpine/edge/testing" >> /etc/apk/repositories \
    && apk add --no-cache \
    libcurl \
    libprotobuf \
    grpc \
    opentelemetry-cpp

# Copy the pre-built C++ binary from the builder stage
COPY --from=cpp-builder /app/heap_profile_server /app/heap_profile_server

# Copy the required libraries from the build stage
COPY --from=cpp-builder /usr/local/lib/libtcmalloc_and_profiler.so.4 /usr/local/lib/
COPY --from=cpp-builder /usr/local/lib/libopentelemetry*.so* /usr/local/lib/

# Set the library path to ensure all dependencies are found
ENV LD_LIBRARY_PATH=/usr/local/lib:/usr/lib:/lib

WORKDIR /app

EXPOSE 8080
CMD ["./heap_profile_server"]
