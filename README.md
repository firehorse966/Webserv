# webserv

A lightweight HTTP/1.1 web server written in **C++ (C++98)** as part of the 42 curriculum.  
The project focuses on low-level network programming, request parsing, response generation, and configuration handling, following the constraints of the subject and RFC standards.

---

## 📌 Features

- HTTP/1.1 compliant server
- Support for multiple virtual servers
- Non-blocking I/O using:
  - `epoll` (Linux)
- Configurable via a custom configuration file
- Handles multiple clients simultaneously
- Supported HTTP methods:
  - `GET`
  - `POST`
  - `DELETE`
  - `HEAD`
  - `OPTIONS`

- Static file serving
- Directory listing (autoindex)
- Custom error pages
- File upload support
- Chunked transfer encoding
- CGI execution (e.g. Python, PHP)
- Request body size limits
- Proper HTTP status codes and headers

---

## 🧠 What This Project Demonstrates

- Socket programming (`socket`, `bind`, `listen`, `accept`)
- Event-driven architecture
- Parsing and validating HTTP requests
- Building HTTP responses manually
- Process and file descriptor management
- CGI handling with environment setup
- Robust error handling
- Writing scalable and maintainable C++98 code under strict constraints

---

## 🛠️ Technologies

- Language: **C++**
- Standard: **C++98**
- Platform: Linux / macOS
- System calls:
  - `socket`, `bind`, `listen`, `accept`
  - `recv`, `send`, `read`, `write`
  - `epoll` / `kqueue`
  - file and process-related system calls
- CGI:
  - Python
  - C++

---

## 🚀 Build & Run

### Requirements
- `clang++` or `g++`
- `make`

### Compilation
```bash
make
```

### Run
```bash
./webserv path/to/config.conf
```

If no configuration file is provided, the default configuration is used.

---

## 📖 Configuration Overview

Example configuration snippet:
```conf
server {
    listen 8080;
    server_name localhost;

    root ./www;
    index index.html;

    location /upload {
        method POST;
        upload_path ./uploads;
    }

    location /cgi-bin {
        cgi_pass python;
    }

    error_page 404 ./errors/404.html;
}
```

---

## 🧪 Testing

- Tested with:
  - `curl`
  - web browsers
  - custom stress tests
- HTTP compliance verified against:
  - RFC 7230–7235
- Memory checked with:
```bash
valgrind --leak-check=full ./webserv config.conf
```

---

## ⚠️ Limitations

- HTTPS is not supported
- No support for HTTP/2
- Designed strictly within 42 subject constraints

---

## 📄 License

This project is for educational purposes as part of the 42 curriculum.
