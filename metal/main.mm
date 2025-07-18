#import <Metal/Metal.h>
#import <Foundation/Foundation.h>

#include <iostream>
#include <vector>
#include <chrono>
#include <string>
#include <iomanip>
#include <cstring>
#include <cmath>
#include <sstream>
#include <numeric>

std::string formatBytes(double bytes) {
    const char* units[] = { "B/s", "KB/s", "MB/s", "GB/s", "TB/s" };
    int i = 0;
    while (bytes >= 1024.0 && i < 4) {
        bytes /= 1024.0;
        ++i;
    }
    std::ostringstream oss;
    oss << std::fixed << std::setprecision(2) << bytes << " " << units[i];
    return oss.str();
}

std::string formatSize(size_t bytes) {
    const char* units[] = { "B", "KB", "MB", "GB", "TB" };
    int i = 0;
    double val = static_cast<double>(bytes);
    while (val >= 1024.0 && i < 4) {
        val /= 1024.0;
        ++i;
    }
    std::ostringstream oss;
    oss << std::fixed << std::setprecision(2) << val << " " << units[i];
    return oss.str();
}

size_t parseSizeArgument(const std::string& input) {
    double multiplier = 1.0;
    char suffix = std::toupper(input.back());
    std::string numberPart = input;

    if (std::isalpha(suffix)) {
        numberPart = input.substr(0, input.size() - 1);
        switch (suffix) {
            case 'K': multiplier = 1024.0; break;
            case 'M': multiplier = 1024.0 * 1024; break;
            case 'G': multiplier = 1024.0 * 1024 * 1024; break;
            default: throw std::runtime_error("Invalid size suffix");
        }
    }

    double value = std::stod(numberPart);
    return static_cast<size_t>(value * multiplier);
}

void printUsage(const char* prog) {
    std::cout << "Usage: " << prog << " [OPTIONS]\n\n"
              << "Options:\n"
              << "  --list-devices           List all Metal GPU devices and exit\n"
              << "  --device <N>             Select Metal device index (default: 0)\n"
              << "  --size <value>           Transfer size (e.g. 512M, 1.5G). Default: 1G\n"
              << "  --iter <N>               Number of iterations. Default: 10\n"
              << "  --help                   Show this message\n";
}

void benchmark(id<MTLDevice> device,
               size_t size,
               int iterations) {
    id<MTLCommandQueue> queue = [device newCommandQueue];

    id<MTLBuffer> h2d_src = [device newBufferWithLength:size options:MTLResourceStorageModeShared];
    id<MTLBuffer> h2d_dst = [device newBufferWithLength:size options:MTLResourceStorageModePrivate];

    id<MTLBuffer> d2h_src = [device newBufferWithLength:size options:MTLResourceStorageModePrivate];
    id<MTLBuffer> d2h_dst = [device newBufferWithLength:size options:MTLResourceStorageModeShared];

    id<MTLBuffer> d2d_src = [device newBufferWithLength:size options:MTLResourceStorageModePrivate];
    id<MTLBuffer> d2d_dst = [device newBufferWithLength:size options:MTLResourceStorageModePrivate];

    auto measure = [&](const std::string& label,
                       id<MTLBuffer> src,
                       id<MTLBuffer> dst) {
        std::vector<double> times;

        for (int i = 0; i < iterations; ++i) {
            auto start = std::chrono::high_resolution_clock::now();

            id<MTLCommandBuffer> cb = [queue commandBuffer];
            id<MTLBlitCommandEncoder> blit = [cb blitCommandEncoder];
            [blit copyFromBuffer:src
                    sourceOffset:0
                        toBuffer:dst
               destinationOffset:0
                            size:size];
            [blit endEncoding];
            [cb commit];
            [cb waitUntilCompleted];

            auto end = std::chrono::high_resolution_clock::now();
            double ms = std::chrono::duration<double, std::milli>(end - start).count();
            times.push_back(ms);
        }

        double total = std::accumulate(times.begin(), times.end(), 0.0);
        double avg = total / iterations;
        double stddev = std::sqrt(std::accumulate(times.begin(), times.end(), 0.0,
                                [avg](double acc, double val) { return acc + (val - avg) * (val - avg); }) / iterations);

        double bandwidth = (size * iterations) / (total / 1000.0);

        std::cout << label << ": " << formatBytes(bandwidth)
                  << " (" << total << " ms total, " << avg << " (+/- " << stddev << ") ms/iter)\n";
    };

    std::cout << "Measuring Metal memory bandwidth (per-transfer timing)...\n";
    measure("Host to Device", h2d_src, h2d_dst);
    measure("Device to Host", d2h_src, d2h_dst);
    measure("Device to Device", d2d_src, d2d_dst);
}

int main(int argc, const char* argv[]) {
    size_t size = 1024 * 1024 * 1024; // 1GB
    int iterations = 10;
    int deviceIndex = 0;
    bool listOnly = false;

    const char* prog = argv[0];

    for (int i = 1; i < argc; ++i) {
        std::string arg = argv[i];
        if (arg == "--help") {
            printUsage(prog);
            return 0;
        } else if (arg == "--list-devices") {
            listOnly = true;
        } else if (arg == "--device" && i + 1 < argc) {
            deviceIndex = std::stoi(argv[++i]);
        } else if (arg == "--size" && i + 1 < argc) {
            size = parseSizeArgument(argv[++i]);
        } else if (arg == "--iter" && i + 1 < argc) {
            iterations = std::stoi(argv[++i]);
        } else {
            std::cerr << "Unknown option: " << arg << "\n";
            printUsage(prog);
            return 1;
        }
    }

    NSArray<id<MTLDevice>>* devices = MTLCopyAllDevices();

    if (listOnly) {
        for (NSUInteger i = 0; i < [devices count]; ++i) {
            std::cout << "  [" << i << "] " << [[[devices objectAtIndex:i] name] UTF8String] << "\n";
        }
        return 0;
    }

    if (deviceIndex >= [devices count]) {
        std::cerr << "Invalid device index.\n";
        return 1;
    }

    id<MTLDevice> device = [devices objectAtIndex:deviceIndex];
    std::cout << "Using device: " << [[device name] UTF8String] << "\n";
    std::cout << "Transfer size: " << size << " bytes (" << formatSize(size)
              << "), Iterations: " << iterations << "\n";

    benchmark(device, size, iterations);

    return 0;
}

