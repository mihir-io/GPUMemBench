# OpenCL Memory Benchmark

This repository provides a simple CLI tool that uses Metal to test Device to Host, Host to Device, and Device to Device memory operations.

## Usage

```
Usage: ./metalmembench [OPTIONS]

Options:
  --list-devices           List all Metal GPU devices and exit
  --device <N>             Select Metal device index (default: 0)
  --size <value>           Transfer size (e.g. 512M, 1.5G). Default: 1G
  --iter <N>               Number of iterations. Default: 10
  --help                   Show this message
```

### Example Output

```
./metalmembench --size 2G --device 1
Using device: AMD Radeon Pro 560X
Transfer size: 2147483648 bytes (2.00 GB), Iterations: 10
Measuring Metal memory bandwidth (per-transfer timing)...
Host to Device: 5.82 GB/s (3433.55 ms total, 343.355 (+/- 82.8554) ms/iter)
Device to Host: 5.13 GB/s (3901.3 ms total, 390.13 (+/- 257.049) ms/iter)
Device to Device: 2.91 GB/s (6881.29 ms total, 688.129 (+/- 490.517) ms/iter)
```

## Build Instructions

### macOS

#### Prerequisites

* Clang (Part of XCode/CLI Developer Tools)
* Metal SDK (Also part of XCode/Developer Tools)

#### Command

This has been tested on macOS Sequoia on both, ARM and Intel Macs. First, `cd` into the `metal/` directory then run

```bash
clang++ -std=c++17 -ObjC++ -framework Metal -framework Foundation -o metalmembench main.mm
```
