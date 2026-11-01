# Custom Parallel CLI Downloader

A command-line parallel downloader written in C, built from scratch using POSIX sockets, OpenSSL and low-level file I/O.

**Status: Work in Progress**

## Overview

I'm building a custom multithreaded file downloader that can split files into independently downloadable byte ranges and reconstruct them locally.

One of the main goals is to make the downloader intelligent about how it distributes work. I'm working towards a custom worker-selection algorithm that considers factors such as the system's available CPU threads, the size of the file being downloaded and the available network bandwidth.

I'll also try to explore cache-friendly memory access patterns and efficient resource management to keep unnecessary overhead to a minimum.

The project is primarily about understanding what happens behind the scenes when downloading a file, with a focus on networking, HTTP, TLS, concurrency and systems programming without relying on high-level downloading libraries.

## Current Implementation

- URL parsing and validation
- DNS resolution using getaddrinfo()
- TCP socket creation and connection
- HTTPS support using OpenSSL
- TLS certificate and hostname verification
- HTTP HEAD request construction and transmission
- Dynamic HTTP response buffering
- HTTP response metadata parsing

## Planned Features

- HTTP Range request support
- Binary file downloading
- Random-access file writing using `pwrite()`
- Parallel downloads using POSIX threads
- Adaptive worker-count selection
- Chunk reconstruction and integrity verification
- Failed-chunk recovery and download resumption
- Cache-conscious memory and I/O optimizations
- Throughput benchmarking and performance analysis

## Technologies

- **Language:** C
- **Networking:** POSIX sockets
- **Encryption:** OpenSSL
- **Concurrency:** POSIX Threads
- **File I/O:** POSIX system calls

## Building

Requires GCC, OpenSSL development libraries and a POSIX-compatible environment.

```bash
gcc -std=gnu11 -Wall -Wextra -Wpedantic -g \
    main.c tls.c parser.c connections.c \
    -o downloader -lssl -lcrypto -pthread
```

## Usage

```bash
./downloader "https://example.com/"
```

## Project Goals

This project is a hands-on exploration of systems programming in C. I want to understand networking, manual memory management, concurrency and operating-system interfaces by building something substantial rather than relying entirely on existing abstractions.

It's also an opportunity to experiment with performance optimization and learn how different parts of a system interact under real workloads.

