#include <iostream>

int main() {
    std::cout << "Hello from moba-sim!\n";
#if defined(_WIN32) || defined(__MINGW32__)
    std::cout << "Running on Windows build.\n";
#elif defined(__linux__)
    std::cout << "Running on Linux build.\n";
#else
    std::cout << "Running on unknown platform.\n";
#endif
    return 0;
}
