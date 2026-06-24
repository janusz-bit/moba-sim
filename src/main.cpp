#include <iostream>

namespace moba {
using type = double;

struct Resistances {
  type magic_resistance;
};

class WithHP {};
} // namespace moba

int main() { std::cout << "Hello from moba-sim!\n"; }
