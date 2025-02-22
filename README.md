### Remote Shell

Implementation of a remote shell over a TCP connection.

---

## **Table of Contents**

1. [Prerequisites](#Prequisites)
2. [Installation](#Installation)
3. [Usage](#Usage)

---

## **Prerequisites**

This project uses a build system written by D'arcy Smith.

To compile using the build system you need:
- D'arcy's libraries built from [https://github.com/programming101dev/scripts]

Tested Platforms:
- Arch Linux 2024.12.01
- Manjaro 24.2
- Ubuntu 2024.04.1
- MacOS 14.2 (clang only)

Dependencies:
- gcc or clang (Makefile specifies clang)
- make

---

## **Installation**

Clone this repository:
```sh
git clone https://github.com/kvnbanunu/remoteshell
```

Build with D'arcy's system:
1. Link your local scripts directory
   ```sh
   ./create-links.sh <path>
   ```
2. Change compiler to gcc or clang
   ```sh
   ./change-compiler.sh -c <gcc or clang>
   ```
3. Generate cmakelist
   ```sh
   ./generate-cmakelists.sh
   ```
4. Build
   ```sh
   ./build.sh
   ```

Build with Make:
```sh
make build
```

---

## **Usage**

Run with Make:
- Server
    ```sh
    make server
    ```

- Client
    ```sh
    make client ARGS="ip port"
    ```

Run directly:
- Server
    ```sh
    ./build/server
    ```

- Client
    ```sh
    ./build/client <ip> <port>
    ```

---
