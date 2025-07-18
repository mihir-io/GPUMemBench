# OpenCL Memory Benchmark

This repository provides a simple CLI tool that uses OpenCL to test Device to Host, Host to Device, and Device to Device memory operations.

## Usage

```
Usage: oclmembench.exe [OPTIONS]

Options:
  --list-devices           List all OpenCL devices and exit
  --device <N>             Select OpenCL device by index (default: 0)
  --size <value>           Transfer size (e.g. 512M, 1.25G) [default: 1G]
  --iter <N>               Number of iterations [default: 1]
  --help                   Show this message
```

## Build Instructions

### Windows

#### Prerequisites

* MinGW GCC (tested with the `g++` that comes with StrawberryPerl)
* OpenCL SDK (you can download the latest one [here](https://github.com/KhronosGroup/OpenCL-SDK/releases))

#### Command

I use Git Bash to build this, hence the Unix-style paths. You should also subsitute the paths with the exact version of the OpenCL SDK you downloaded.

```bash
g++ -std=c++17 main.cpp -L ~/Downloads/OpenCL-SDK-v2024.10.24-Win-x64/lib/ -I ~/Downloads/OpenCL-SDK-v2024.10.24-Win-x64/include/ -lOpenCL -o oclmembench.exe
```
