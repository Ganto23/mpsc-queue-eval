#include <iostream>
#include <thread>

int main() {
    std::cout << "MPSC Queue Benchmarking System Initialized.\n";
    std::cout << "Hardware Concurrency (Cores): " << std::thread::hardware_concurrency() << "\n";
    return 0;
}