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

### Example Output

```
./oclmembench-1.0.0-win-x86_64.exe --size 2G
Using device: gfx1100
Transfer size: 2147483648 bytes (2.00 GB), Iterations: 1
Measuring OpenCL memory bandwidth (per-transfer timing)...
Host to Device:   25.69 GB/s (77.8422 ms total, 77.8422 (+/- 0) ms/iter)
Device to Host:   25.92 GB/s (77.1463 ms total, 77.1463 (+/- 0) ms/iter)
Device to Device: 147.16 GB/s (13.5909 ms total, 13.5909 (+/- 0) ms/iter)
```

## Build Instructions

### Windows

#### Prerequisites

* MinGW GCC (tested with the `g++` that comes with StrawberryPerl)
* OpenCL SDK (you can download the latest one [here](https://github.com/KhronosGroup/OpenCL-SDK/releases))

#### Command

I use Git Bash to build this, hence the Unix-style paths. You should also subsitute the paths with the exact version of the OpenCL SDK you downloaded.
`cd` into the `opencl` directory and run this:

```bash
g++ -std=c++17 main.cpp -L ~/Downloads/OpenCL-SDK-v2024.10.24-Win-x64/lib/ -I ~/Downloads/OpenCL-SDK-v2024.10.24-Win-x64/include/ -lOpenCL -o oclmembench.exe
```
