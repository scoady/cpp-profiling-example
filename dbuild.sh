#!/bin/bash

export VERSION=$(cat VERSION)
docker build -t seancoady22/cpp-profiling-demo:${VERSION} --target cpp-app . -f CombinedDockerfile
docker build -t seancoady22/go-bridge-cpp-pprof:${VERSION} --target go-bridge . -f CombinedDockerfile
docker push seancoady22/cpp-profiling-demo:${VERSION}
docker push seancoady22/go-bridge-cpp-pprof:${VERSION}


