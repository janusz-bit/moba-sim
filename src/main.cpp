#include <cstdint>
#include <functional>
#include <iostream>
#include <variant>

namespace moba {
using Type = double;

class Alive;

enum class KindDamage : std::int8_t {
  Physical,
  Magic,
  True,
};

struct DamageBase {
  using FnType = std::function<Type(const Alive &)>;
  using Source = std::variant<Type, FnType>;

  Source amount;
  KindDamage kind{};
};

class Resistances {
  Type magic_resistance_{0};
  Type armor_{0};

public:
  [[nodiscard]] auto getMagicResistance() const -> Type {
    return magic_resistance_;
  }
  [[nodiscard]] auto getArmor() const -> Type { return armor_; }
};

class WithHP {
  Type max_health_{100};
};

class Alive : public Resistances, public WithHP {};

} // namespace moba

int main() { std::cout << "Hello from moba-sim!\n"; }
