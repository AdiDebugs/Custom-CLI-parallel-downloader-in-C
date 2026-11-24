# Custom Parallel CLI Downloader
#### Video Demo: https://youtu.be/0tE4GU1ttV0
#### Description:
A multithreaded HTTP/HTTPS command-line downloader written in C using POSIX sockets, OpenSSL and low-level file I/O.

This project started as an attempt to understand what actually happens under the hood when a file is downloaded. Instead of relying on high-level networking or downloading libraries such as libcurl, I implemented the networking, HTTP request handling, TLS connections, concurrency and file-writing mechanisms myself.

**Status: Functional prototype — actively developing**

## Why I built this

I wanted to work with the lower-level interfaces exposed by the operating system rather than abstract them away behind existing libraries.

A parallel downloader turned out to be a useful project because it combines several areas of systems programming:

- TCP socket programming and DNS resolution.
- HTTP/1.1 request and response handling.
- TLS connection establishment using OpenSSL.
- Dynamic memory allocation and response parsing.
- Concurrency and thread lifecycle management.
- Low-level positional file I/O.
- Network performance measurement and optimization.

The main objective is to divide a file into independent byte ranges, download those ranges concurrently, and write each chunk directly to its correct position in the destination file.

## Features

- HTTP and HTTPS downloads.
- Custom URL parsing and validation.
- DNS resolution using `getaddrinfo()`.
- Manual HTTP/1.1 request construction.
- TLS connections using OpenSSL.
- Server metadata retrieval using HTTP HEAD requests.
- HTTP byte-range downloading.
- Dynamic worker selection, capped at 8 concurrent workers.
- Independent TCP connections for individual workers.
- Positional file writes using `pwrite()`.
- Output-file allocation using `fallocate()`, with an `ftruncate()` fallback.
- Response validation using HTTP status codes, `Content-Length` and `Content-Range`.
- Handling of partial socket/TLS writes, partial file writes and interrupted system calls.
- Worker result tracking and thread joining.

## Architecture and download workflow

### 1. URL parsing and validation

The URL parser extracts the protocol, hostname, port, path and filename.

It handles default ports for HTTP and HTTPS, as well as explicitly specified ports. The resulting URL structure is used throughout the networking and download routines.

### 2. Server probing and metadata retrieval

Before starting the download, the program resolves the hostname and establishes an initial connection.

It sends an HTTP HEAD request to retrieve information such as:

- HTTP response status.
- File size through `Content-Length`.
- Byte-range support through `Accept-Ranges`.
- Redirect destination through `Location`.

The retrieved metadata is used to determine whether the server is suitable for parallel downloading.

### 3. Worker selection and range partitioning

The worker-selection algorithm considers:

- File size.
- Measured server response time.
- Maximum bandwidth supplied by the user.

The selected worker count is capped at **8 concurrent workers**.

The file is divided into non-overlapping byte ranges. Internally, the ranges are represented as half-open intervals:

`[start_offset, end_offset)`

Since HTTP Range requests use inclusive end offsets, the request is constructed using:

`Range: bytes=start_offset-(end_offset - 1)`

Each worker is responsible for downloading only its assigned range.

### 4. Independent HTTP and TLS connections

Every worker establishes its own TCP connection to the server.

For HTTPS downloads, every worker also establishes an independent TLS session using OpenSSL.

The program constructs HTTP GET requests manually and includes the appropriate Range header.

Each worker validates the server's response before accepting the data. In particular, it checks for `206 Partial Content` and validates the returned content length and content range against its assigned byte interval.

### 5. File allocation and positional writes

Before downloading begins, the destination file is created and prepared for the expected file size.

The program first attempts to allocate space using:

`fallocate()`

If allocation through `fallocate()` is unavailable or unsupported by the filesystem, it falls back to:

`ftruncate()`

There is an important difference between these operations:

- `fallocate()` requests allocation of the underlying filesystem blocks.
- `ftruncate()` establishes the logical file size but does not necessarily reserve the physical disk space immediately.

Once the file is prepared, workers write their downloaded data using `pwrite()`.

Unlike ordinary `write()`, `pwrite()` writes at a specified file offset without modifying the shared file descriptor's current file position.

This allows multiple workers to write to separate regions of the same file without requiring a shared file offset or a mutex around file writes.

The implementation also handles partial writes and interrupted system calls.

### 6. Thread lifecycle and result handling

Workers are launched using `pthread_create()` and collected using `pthread_join()`.

Each worker maintains its own result status. The main thread checks the results after joining the workers and reports failures rather than treating an incomplete download as successful.

Successfully created threads are joined even when thread creation fails partway through.

## Why the worker limit is 8

The maximum worker count is a deliberate design decision, not a restriction imposed by HTTP/1.1.

Every worker opens an independent TCP connection and, for HTTPS, establishes its own TLS session.

HTTP/1.1 does not provide HTTP/2-style multiplexing of independent requests over one connection. Increasing the worker count therefore increases connection and TLS-handshake overhead.

Too many simultaneous connections can also encounter server-side connection limits or throttling.

The worker-selection algorithm attempts to choose a useful level of parallelism without unnecessarily increasing the number of connections.

More threads do not automatically mean better performance.

## Current limitations

The project is functional, but it does not attempt to implement every behaviour expected from a production download manager.

The following limitations are intentional and documented:

### No sequential-download fallback

The current implementation relies on HTTP byte-range support.

When the server does not advertise byte-range support, the program reports that ranged downloading is unavailable and terminates the download attempt.

It does not currently switch to a conventional single-connection download.

### No automatic redirect following

The program retrieves the `Location` header when a server responds with a redirect.

However, it does not automatically follow the redirect. Instead, it displays the destination and requires the user to rerun the program with the new URL.

### No download resumption

Interrupted downloads cannot currently resume from their previously downloaded byte ranges.

The program does not maintain a persistent download-state file or verify previously completed chunks before continuing.

### No automatic worker retry or range reassignment

When a worker fails, the failure is reported to the main thread.

The program does not currently retry failed ranges or dynamically reassign them to other workers.

### Range support is server-dependent

The parallel workflow requires a server that supports byte-range requests and returns valid partial-content responses.

A server that ignores Range requests or provides inconsistent response metadata cannot be handled through the current parallel workflow.

## Building

### Requirements

- Linux or WSL with a POSIX-compatible development environment.
- GCC.
- OpenSSL development headers and libraries.
- POSIX threads.

On Ubuntu/WSL, install the dependencies using:

```bash
sudo apt update
sudo apt install build-essential libssl-dev
```

### Compile

From the project directory:

```bash
gcc -std=gnu11 \
    -Wall -Wextra -Wpedantic \
    -Wshadow -Wformat=2 \
    -g \
    main.c parser.c connections.c tls.c \
    optimal_threadc.c multithreading.c \
    -o downloader \
    -pthread -lssl -lcrypto
```

## Usage

The command-line interface is:

```bash
./downloader <URL> <MAX_BANDWIDTH_Mbps>
```

Example:

```bash
./downloader \
    "https://assets.mixkit.co/videos/41576/41576-1080.mp4" \
    40
```

The second argument represents the maximum network bandwidth, in megabits per second, supplied to the worker-selection algorithm.

It is an input to the algorithm, not a guarantee of the actual download speed.

Quote URLs containing shell-sensitive characters such as parentheses or ampersands. URL paths should use valid percent-encoding where appropriate.

## Technologies

| Component | Technology |
|---|---|
| Language | C |
| Compiler | GCC |
| Networking | POSIX sockets |
| DNS resolution | `getaddrinfo()` |
| Application protocol | HTTP/1.1 |
| Secure transport | OpenSSL |
| Concurrency | POSIX threads (`pthread`) |
| Memory management | `malloc()`, `realloc()`, `free()` |
| File operations | `open()`, `fallocate()`, `ftruncate()`, `pwrite()` |
| Socket/TLS I/O | `send()`, `recv()`, `SSL_write()`, `SSL_read()` |

## Testing and performance

The downloader has been tested using HTTP and HTTPS file endpoints, including larger binary and video files.

Testing has included:

- TLS connection establishment and protocol negotiation.
- Multiple concurrent HTTPS workers.
- HTTP partial-content response validation.
- Different file sizes and worker counts.
- Downloading over conventional Wi-Fi and mobile 5G connections.
- Comparing download completion times against browser downloads.

For integrity verification, compare the downloaded file against a reference file using SHA-256:

```bash
sha256sum downloader_output.mp4 reference.mp4
```

Matching hashes indicate that the downloaded contents are identical.

Download performance depends on the server, available network bandwidth, connection limits and selected worker count.

## Future improvements

- Sequential downloading when byte ranges are unavailable.
- Automatic redirect resolution and following.
- Resume support using persistent download metadata.
- Automatic retries and failed-range reassignment.
- More robust timeout and connection-failure handling.
- IPv6 support.
- More systematic performance benchmarking.
- Further optimization of worker selection.

## Project goals

This project is primarily a systems-programming learning exercise.

I built it to gain practical experience with networking interfaces, HTTP protocol handling, TLS, memory ownership, multithreading and low-level file operations.

Rather than depending on a high-level downloading library, the goal was to understand and implement the mechanisms that make concurrent file transfers possible.