FROM debian:13-slim

RUN apt-get update && \
    apt-get install -y --no-install-recommends \
        gcc \
        cmake \
        make \
        libcriterion-dev \
    && rm -rf /var/lib/apt/lists/*

WORKDIR /project
