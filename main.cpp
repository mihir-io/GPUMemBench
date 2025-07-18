#define CL_TARGET_OPENCL_VERSION 120
#include <CL/cl.h>

#include <iostream>
#include <vector>
#include <chrono>
#include <string>
#include <iomanip>
#include <cstring>
#include <cstdlib>
#include <cctype>
#include <cmath>
#include <numeric>

#define CHECK_CL(err, msg) \
    if (err != CL_SUCCESS) { \
        std::cerr << "OpenCL Error: " << msg << " (" << err << ")" << std::endl; \
        std::exit(1); \
    }

void printUsage(const char* progName) {
    std::cout << "Usage: " << progName << " [OPTIONS]\n\n"
              << "Options:\n"
              << "  --list-devices           List all OpenCL devices and exit\n"
              << "  --device <N>             Select OpenCL device by index (default: 0)\n"
              << "  --size <value>           Transfer size (e.g. 512M, 1.25G) [default: 1G]\n"
              << "  --iter <N>               Number of iterations [default: 1]\n"
              << "  --help                   Show this message\n";
}

std::string formatBytes(double bytes) {
    const char* units[] = { "B/s", "KB/s", "MB/s", "GB/s", "TB/s" };
    int i = 0;
    while (bytes >= 1024 && i < 4) {
        bytes /= 1024;
        ++i;
    }
    std::ostringstream out;
    out << std::fixed << std::setprecision(2) << bytes << " " << units[i];
    return out.str();
}

std::string formatSize(size_t bytes) {
    const char* units[] = { "B", "KB", "MB", "GB", "TB" };
    int i = 0;
    double val = static_cast<double>(bytes);
    while (val >= 1024 && i < 4) {
        val /= 1024;
        ++i;
    }
    std::ostringstream out;
    out << std::fixed << std::setprecision(2) << val << " " << units[i];
    return out.str();
}

size_t parseSizeArgument(const std::string& input) {
    double multiplier = 1.0;
    char suffix = std::toupper(input.back());
    std::string numberPart = input;

    if (std::isalpha(suffix)) {
        numberPart = input.substr(0, input.size() - 1);
        switch (suffix) {
            case 'K': multiplier = 1024; break;
            case 'M': multiplier = 1024 * 1024; break;
            case 'G': multiplier = 1024 * 1024 * 1024; break;
            case 'T': multiplier = 1024.0 * 1024 * 1024 * 1024; break;
            default: throw std::runtime_error("Invalid size suffix (K/M/G/T)");
        }
    }

    double value = std::stod(numberPart);
    return static_cast<size_t>(value * multiplier);
}

void listAllOpenCLDevices() {
    cl_uint numPlatforms = 0;
    CHECK_CL(clGetPlatformIDs(0, nullptr, &numPlatforms), "platform count");
    std::vector<cl_platform_id> platforms(numPlatforms);
    CHECK_CL(clGetPlatformIDs(numPlatforms, platforms.data(), nullptr), "platforms");

    int deviceID = 0;
    for (cl_platform_id plat : platforms) {
        char pname[128];
        clGetPlatformInfo(plat, CL_PLATFORM_NAME, sizeof(pname), pname, nullptr);
        std::cout << "Platform: " << pname << "\n";

        cl_uint numDevices = 0;
        clGetDeviceIDs(plat, CL_DEVICE_TYPE_ALL, 0, nullptr, &numDevices);
        std::vector<cl_device_id> devices(numDevices);
        clGetDeviceIDs(plat, CL_DEVICE_TYPE_ALL, numDevices, devices.data(), nullptr);

        for (cl_device_id dev : devices) {
            char dname[128];
            clGetDeviceInfo(dev, CL_DEVICE_NAME, sizeof(dname), dname, nullptr);
            std::cout << "  [" << deviceID++ << "] " << dname << "\n";
        }
    }
}

void measureWriteWithEvents(cl_command_queue queue, cl_mem buffer, const void* hostPtr, size_t size, int iterations) {
    std::vector<double> durations;
    for (int i = 0; i < iterations; ++i) {
        cl_event evt = nullptr;
        CHECK_CL(clEnqueueWriteBuffer(queue, buffer, CL_FALSE, 0, size, hostPtr, 0, nullptr, &evt), "enqueue write");
        CHECK_CL(clWaitForEvents(1, &evt), "wait write");

        cl_ulong start = 0, end = 0;
        clGetEventProfilingInfo(evt, CL_PROFILING_COMMAND_START, sizeof(start), &start, nullptr);
        clGetEventProfilingInfo(evt, CL_PROFILING_COMMAND_END, sizeof(end), &end, nullptr);
        durations.push_back((end - start) * 1e-6);
        clReleaseEvent(evt);
    }

    double total = std::accumulate(durations.begin(), durations.end(), 0.0);
    double avg = total / iterations;
    double stddev = std::sqrt(std::accumulate(durations.begin(), durations.end(), 0.0,
        [avg](double acc, double val) { return acc + (val - avg) * (val - avg); }) / iterations);

    double bandwidth = (iterations * size) / (total / 1000.0);
    std::cout << "Host to Device:   " << formatBytes(bandwidth)
              << " (" << total << " ms total, " << avg << " (+/- " << stddev << ") ms/iter)\n";
}

void measureReadWithEvents(cl_command_queue queue, cl_mem buffer, void* hostPtr, size_t size, int iterations) {
    std::vector<double> durations;
    for (int i = 0; i < iterations; ++i) {
        cl_event evt = nullptr;
        CHECK_CL(clEnqueueReadBuffer(queue, buffer, CL_FALSE, 0, size, hostPtr, 0, nullptr, &evt), "enqueue read");
        CHECK_CL(clWaitForEvents(1, &evt), "wait read");

        cl_ulong start = 0, end = 0;
        clGetEventProfilingInfo(evt, CL_PROFILING_COMMAND_START, sizeof(start), &start, nullptr);
        clGetEventProfilingInfo(evt, CL_PROFILING_COMMAND_END, sizeof(end), &end, nullptr);
        durations.push_back((end - start) * 1e-6);
        clReleaseEvent(evt);
    }

    double total = std::accumulate(durations.begin(), durations.end(), 0.0);
    double avg = total / iterations;
    double stddev = std::sqrt(std::accumulate(durations.begin(), durations.end(), 0.0,
        [avg](double acc, double val) { return acc + (val - avg) * (val - avg); }) / iterations);

    double bandwidth = (iterations * size) / (total / 1000.0);
    std::cout << "Device to Host:   " << formatBytes(bandwidth)
              << " (" << total << " ms total, " << avg << " (+/- " << stddev << ") ms/iter)\n";
}

void measureDeviceToDeviceWithEvents(cl_command_queue queue, cl_mem src, cl_mem dst, size_t size, int iterations) {
    std::vector<double> durations;
    for (int i = 0; i < iterations; ++i) {
        cl_event evt = nullptr;
        CHECK_CL(clEnqueueCopyBuffer(queue, src, dst, 0, 0, size, 0, nullptr, &evt), "copy buffer");
        CHECK_CL(clWaitForEvents(1, &evt), "wait");

        cl_ulong start = 0, end = 0;
        clGetEventProfilingInfo(evt, CL_PROFILING_COMMAND_START, sizeof(start), &start, nullptr);
        clGetEventProfilingInfo(evt, CL_PROFILING_COMMAND_END, sizeof(end), &end, nullptr);
        durations.push_back((end - start) * 1e-6);
        clReleaseEvent(evt);
    }

    double total = std::accumulate(durations.begin(), durations.end(), 0.0);
    double avg = total / iterations;
    double stddev = std::sqrt(std::accumulate(durations.begin(), durations.end(), 0.0,
        [avg](double acc, double val) { return acc + (val - avg) * (val - avg); }) / iterations);

    double bandwidth = (iterations * size) / (total / 1000.0);
    std::cout << "Device to Device: " << formatBytes(bandwidth)
              << " (" << total << " ms total, " << avg << " (+/- " << stddev << ") ms/iter)\n";
}

int main(int argc, char* argv[]) {
    size_t size = 1024 * 1024 * 1024;
    int iterations = 1;
    int selectedDevice = 0;
    bool listOnly = false;

    const char* progName = argv[0];
    for (int i = 1; i < argc; ++i) {
        std::string arg = argv[i];
        if (arg == "--help") {
            printUsage(progName);
            return 0;
        } else if (arg == "--list-devices") {
            listOnly = true;
        } else if (arg == "--device" && i + 1 < argc) {
            selectedDevice = std::stoi(argv[++i]);
        } else if (arg == "--size" && i + 1 < argc) {
            size = parseSizeArgument(argv[++i]);
        } else if (arg == "--iter" && i + 1 < argc) {
            iterations = std::stoi(argv[++i]);
        } else {
            std::cerr << "Unknown or malformed option: " << arg << "\n";
            printUsage(progName);
            return 1;
        }
    }

    cl_uint numPlatforms = 0;
    CHECK_CL(clGetPlatformIDs(0, nullptr, &numPlatforms), "platform count");
    std::vector<cl_platform_id> platforms(numPlatforms);
    CHECK_CL(clGetPlatformIDs(numPlatforms, platforms.data(), nullptr), "platform list");

    std::vector<cl_device_id> allDevices;
    for (auto platform : platforms) {
        cl_uint numDevices;
        clGetDeviceIDs(platform, CL_DEVICE_TYPE_ALL, 0, nullptr, &numDevices);
        std::vector<cl_device_id> devices(numDevices);
        clGetDeviceIDs(platform, CL_DEVICE_TYPE_ALL, numDevices, devices.data(), nullptr);
        allDevices.insert(allDevices.end(), devices.begin(), devices.end());
    }

    if (listOnly) {
        listAllOpenCLDevices();
        return 0;
    }

    if (selectedDevice >= allDevices.size()) {
        std::cerr << "Invalid device index.\n";
        return 1;
    }

    cl_device_id device = allDevices[selectedDevice];
    char devName[128];
    clGetDeviceInfo(device, CL_DEVICE_NAME, sizeof(devName), devName, nullptr);
    std::cout << "Using device: " << devName << "\n";
    std::cout << "Transfer size: " << size << " bytes (" << formatSize(size) << "), Iterations: " << iterations << "\n";

    cl_int err;
    cl_context context = clCreateContext(nullptr, 1, &device, nullptr, nullptr, &err);
    CHECK_CL(err, "create context");

    cl_command_queue queue = clCreateCommandQueue(context, device, CL_QUEUE_PROFILING_ENABLE, &err);
    CHECK_CL(err, "create command queue");

    cl_mem hostBuf = clCreateBuffer(context, CL_MEM_READ_WRITE | CL_MEM_ALLOC_HOST_PTR, size, nullptr, &err);
    void* hostPtr = clEnqueueMapBuffer(queue, hostBuf, CL_TRUE, CL_MAP_READ | CL_MAP_WRITE, 0, size, 0, nullptr, nullptr, &err);

    cl_mem devBuf1 = clCreateBuffer(context, CL_MEM_READ_WRITE, size, nullptr, &err);
    cl_mem devBuf2 = clCreateBuffer(context, CL_MEM_READ_WRITE, size, nullptr, &err);

    std::memset(hostPtr, 0xA5, size);

    std::cout << "Measuring OpenCL memory bandwidth (per-transfer timing)...\n";

    measureWriteWithEvents(queue, devBuf1, hostPtr, size, iterations);
    measureReadWithEvents(queue, devBuf1, hostPtr, size, iterations);
    measureDeviceToDeviceWithEvents(queue, devBuf1, devBuf2, size, iterations);

    clEnqueueUnmapMemObject(queue, hostBuf, hostPtr, 0, nullptr, nullptr);
    clReleaseMemObject(hostBuf);
    clReleaseMemObject(devBuf1);
    clReleaseMemObject(devBuf2);
    clReleaseCommandQueue(queue);
    clReleaseContext(context);

    return 0;
}



