Here is a `README.md` for your CMake-based C++17 library that implements a C++ wrapper for GStreamer:

---

# libdhgst

[![Build Status](https://github.com/DragonHillsTech/libdhgst/actions/workflows/ci.yml/badge.svg)](https://github.com/DragonHillsTech/libdhgst/actions)

## Overview
`libdhgst` is a C++17 library that provides a high-level wrapper for the GStreamer multimedia framework. It simplifies the use of GStreamer elements and bins in C++ applications by leveraging modern C++ features.

## Dependencies
- **GStreamer** (`gstreamer-1.0`, `gstreamer-app-1.0`)
- **Boost**
- **spdlog**
- **CMake 3.10+**
- **C++17 compiler**

## Building the Library
1. Clone the repository:
   ```bash
   git clone git@github.com:DragonHillsTech/libdhgst.git
   cd your-repo
   ```

2. Build the project:
   ```bash
   mkdir build
   cd build
   cmake ..
   make
   ```

3. Run tests (optional):
   ```bash
   ctest --output-on-failure
   ```

## Installing
To install the library:
```bash
sudo make install
```

## Setting Installation Prefix
By default, CMake installs files to `/usr/local`. You can customize the installation directory by setting the `CMAKE_INSTALL_PREFIX`:

```bash
cmake -DCMAKE_INSTALL_PREFIX=/path/to/install ..
make
sudo make install
```

For example, to install the library to `/usr`:
```bash
cmake -DCMAKE_INSTALL_PREFIX=/usr ..
make
sudo make install
```

This allows you to control where the library and headers are installed on your system.