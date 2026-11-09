# Custom Parallel CLI Downloader

A command-line parallel downloader written in C, built from scratch using POSIX sockets, OpenSSL and low-level file I/O.

**Status: Work in Progress**

## Overview

I'm building a custom multithreaded file downloader that can split files into independently downloadable byte ranges and reconstruct them locally.

One of the main goals is to make the downloader intelligent about how it distributes work. I'm developing a worker-selection algorithm that considers factors such as available CPU threads, file size, network bandwidth and server response time.

The project is primarily about understanding what happens behind the scenes when downloading a file, with a focus on networking, HTTP, TLS, concurrency, memory management and systems programming without relying on high-level downloading libraries.

## Current Implementation

- URL parsing and validation
- Filename extraction from URL paths
- DNS resolution using `getaddrinfo()`
- TCP socket creation and connection
- HTTP and HTTPS support
- TLS handshake using OpenSSL
- TLS certificate and hostname verification
- HTTP HEAD request construction and transmission
- Dynamic HTTP response buffering with memory reallocation
- HTTP response metadata parsing
- Extraction of Content-Length, Accept-Ranges and redirect Location
- HTTP response status validation
- Initial worker-count selection algorithm
- Output-file creation and preallocation using `ftruncate()`

## Planned Features

- HTTP Range GET requests
- Content-Range validation
- Binary response body handling
- Random-access file writing using `pwrite()`
- Parallel downloads using POSIX threads
- Independent worker connections
- Chunk reconstruction and integrity verification
- Failed-chunk recovery and download resumption
- Refinement of the worker-selection algorithm
- Throughput benchmarking and performance analysis
- Cache-conscious memory and I/O optimizations

## Technologies

- **Language:** C
- **Networking:** POSIX sockets
- **Encryption:** OpenSSL
- **Concurrency:** POSIX Threads
- **File I/O:** POSIX system calls
- **Build:** GCC

## Building

Requires GCC, OpenSSL development libraries and a POSIX-compatible environment.

```bash
gcc -std=gnu11 -Wall -Wextra -Wpedantic -g \
    main.c tls.c parser.c connections.c optimal_threadc.c \
    -o downloader -lssl -lcrypto -pthread
```

## Usage

```bash
./downloader "<URL>" <MAX_BANDWIDTH_Mbps>
```

Example:

```bash
./downloader "https://example.com/file.zip" 300
```

The bandwidth argument represents the maximum network bandwidth in Mbps.

## Project Goals

This project is a hands-on exploration of systems programming in C. I want to understand networking, manual memory management, concurrency and operating-system interfaces by building something substantial rather than relying entirely on existing abstractions.

It's also an opportunity to experiment with performance optimization and understand how different parts of a system interact under real workloads.