FROM golang:1.23

# Install the Go-based pprof
RUN go install github.com/google/pprof@latest
ENV PATH="/go/bin:${PATH}"
RUN apt-get update && apt-get install -y \
  libgoogle-perftools-dev \
  google-perftools 
RUN curl -fL https://github.com/grafana/pyroscope/releases/download/v1.1.5/profilecli_1.1.5_linux_amd64.tar.gz | tar xvz
RUN chmod +x profilecli
RUN mv profilecli /go/bin/
WORKDIR /app

COPY server.go .

# Minimal mod setup if server.go uses modules
RUN go mod init example.com/bridge || true
RUN go mod tidy

RUN go build -o bridge-server server.go

EXPOSE 8081
CMD ["./bridge-server"]
