# Custom Parallel CLI Downloader

I'm building a custom CLI parallel downloader in C to understand what happens under the hood when a file is downloaded. It uses POSIX sockets, OpenSSL, and low-level file I/O instead of relying on a high-level downloading library.

**Status: Work in progress**

## Why I'm building this

I wanted to build something that would make me work with the lower-level parts of systems programming rather than hide them behind an existing library. A downloader turned out to be a good way to learn about TCP connections, HTTP, TLS, memory management, concurrency, and file I/O in one project.

The goal is to split a file into byte ranges, download those ranges concurrently, and write each chunk directly to its correct position in the output file. I'm also working on a worker-selection algorithm that uses the file size, measured server response time, and the bandwidth limit supplied by the user to decide how many workers to use.

## How it is designed

The program first parses the URL, resolves the hostname, connects to the server, and sends an HTTP `HEAD` request. It uses the response to inspect the status code, file size, redirect information, and whether the server advertises byte-range support.

When range downloading is available, the file can be divided into non-overlapping byte ranges. Each worker will use its own connection to request its assigned range, validate the response, and write the received bytes at the corresponding offset with `pwrite()`.

The output file is prepared before the download starts. The program currently tries `fallocate()` and falls back to `ftruncate()` when the filesystem does not support allocation through `fallocate()`.

## Why the worker limit is 8

The worker-selection algorithm has a hard ceiling of **8 concurrent workers**. This is a design choice, not a limit imposed by HTTP/1.1.

The current approach gives each worker its own TCP connection and, for HTTPS, its own TLS session. HTTP/1.1 does not provide HTTP/2-style multiplexing of concurrent requests over one connection, so increasing the number of workers also increases the number of connections opened to the same server.

Opening too many connections from one client IP can run into per-client connection limits or cause server-side throttling. It also adds connection and TLS-handshake overhead, so more workers do not automatically mean a faster download. The limit of eight is intended to keep parallelism useful without being unnecessarily aggressive toward the server. The algorithm can select fewer workers depending on its inputs, but it will not exceed this cap.

## What is implemented so far

### URL, networking, and TLS

- URL parsing and validation.
- Hostname, port, path, and filename extraction.
- DNS resolution with `getaddrinfo()`.
- TCP socket creation and connection.
- HTTP and HTTPS server probing.
- TLS client setup using OpenSSL.
- Certificate and hostname verification.

### HTTP response handling

- HTTP `HEAD` request construction and transmission.
- Dynamic response buffering using memory allocation and `realloc()`.
- Parsing of response status and headers.
- Extraction of `Content-Length`, `Accept-Ranges`, and redirect `Location`.
- Detection of byte-range support.

### File and range-worker groundwork

- Initial worker-count selection algorithm, capped at eight workers.
- File-size-based byte-range partitioning.
- Output-file creation and preallocation using `fallocate()`, with a `ftruncate()` fallback.
- Initial HTTPS `GET` range-worker routine.
- Partial TLS request-write handling.
- Validation of `206 Partial Content`, `Content-Length`, and `Content-Range` responses.
- Handling of body bytes that arrive in the same read as the HTTP headers.
- Random-access writes using `pwrite()`, including partial-write and `EINTR` handling.

**The important distinction:** the HTTPS range-worker routine is a work in progress, but the complete parallel download flow is not integrated yet. Thread creation and joining, HTTP range workers, sequential fallback, and end-to-end download verification still need to be completed. At this stage, the program can probe a server and prepare the destination file.
## What I plan to work on next

1. **Finish thread orchestration** — create workers with `pthread_create()`, join every worker with `pthread_join()`, and check each worker's result.
2. **Complete HTTPS parallel downloads** — connect the worker routine to the main download flow and verify that all byte ranges are written correctly.
3. **Add HTTP range workers** — implement the equivalent worker over ordinary TCP sockets.
4. **Build sequential fallback** — download the file using one connection when ranges are unsupported or unsuitable.
5. **Handle failures and recovery** — deal with servers that ignore range requests, failed workers, interrupted connections, malformed responses, and incomplete files.
6. **Test correctness** — compare downloaded files with the originals using a checksum, and test different file sizes, servers, and worker counts.
7. **Measure performance** — compare single-connection and parallel downloads and tune the worker-selection algorithm based on results.

## Technologies

- **Language:** C
- **Networking:** POSIX sockets
- **DNS:** `getaddrinfo()`
- **HTTP:** HTTP/1.1
- **TLS:** OpenSSL
- **Concurrency:** POSIX threads
- **File I/O:** `open()`, `fallocate()`, `ftruncate()`, and `pwrite()`
- **Compiler:** GCC

## Building

The project is being developed for a POSIX-compatible environment and requires GCC, OpenSSL development headers and libraries, and POSIX threads.

From the project directory, compile the current source files with:

```bash
gcc -std=gnu11 \
    -Wall -Wextra -Wpedantic \
    -Wshadow -Wformat=2 \
    -g \
    main.c parser.c connections.c tls.c optimal_threadc.c \
    -o downloader \
    -pthread -lssl -lcrypto
```

The source file list may need to be adjusted if the project is split into additional modules. The build command is for the current development layout, not a promise that every planned feature is already available.

## Usage

The intended command-line format is:

```bash
./downloader <URL> <MAX_BANDWIDTH_Mbps>
```

For example:

```bash
./downloader https://proof.ovh.net/files/100Mb.dat 45
```

The bandwidth argument is the maximum network bandwidth in megabits per second (Mbps) that is supplied to the worker-selection algorithm. It is an input to the algorithm, not a guarantee of the speed the download will reach.

## Project goals

I'm using this project to get more comfortable with C and the interfaces that operating systems expose to network applications. The main things I want to learn by building it are socket programming, HTTP message handling, TLS, concurrency, memory ownership, file operations, and performance measurement.

