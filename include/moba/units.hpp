#pragma once

#include <cstdint>
#include <functional>
#include <numeric>
#include <utility>
#include <variant>
#include <vector>

namespace moba {
using Type = double;

class Alive;

[[nodiscard]] Type calculateReduction(Type resistances) {
  if (resistances >= 0.0) {
    return (100.0 / (100.0 + resistances));
  } else {
    return (2.0 - (100.0 / (100.0 - resistances)));
  }
}

enum class TypeDamage : std::uint8_t {
  Physical,
  Magic,
  True,
  Size,
};

enum class KindDamage : std::uint8_t {
  AutoAttack,
  OnHit,
  Spell,
  Size,
};

struct DamageBase {
  using FnType = std::function<Type(Alive &)>;
  using Source = std::variant<Type, FnType>;

  Source amount;
  TypeDamage type{};
  KindDamage kind{};
};

using Numbers = std::vector<Type>;

struct Additive {
  Numbers numbers;
  [[nodiscard]] Type operator()() { return std::reduce(numbers.begin(), numbers.end()); };
};
struct Multiplitive {
  Numbers numbers;
  [[nodiscard]] Type operator()() {
    return std::reduce(numbers.begin(), numbers.end(), 1.0, std::multiplies<>{});
  };
};

struct EquationDamage {};

} // namespace moba
