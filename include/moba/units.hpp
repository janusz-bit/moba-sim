#pragma once

#include <cstdint>
#include <functional>
#include <numeric>
#include <vector>

namespace moba {
using Type = double;

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

using Numbers = std::vector<Type>;

struct Additive {
  Numbers numbers;
  [[nodiscard]] Type operator()() {
    if (numbers.size() == 0) {
      return 0;
    }
    return std::reduce(numbers.begin(), numbers.end());
  };
};
struct Multiplitive {
  Numbers numbers;
  [[nodiscard]] Type operator()() {
    if (numbers.size() == 0) {
      return 1;
    }
    return std::reduce(numbers.begin(), numbers.end(), 1.0, std::multiplies<>{});
  };
};

struct EquationDamage {
  Additive ap_dmg, ad_dmg, true_dmg;
  Multiplitive ap_dmg_m, ad_dmg_m, true_dmg_m, non_true_dmg_m, dmg_m;
  [[nodiscard]] Type operator()() {
    return ((((ap_dmg() * ap_dmg_m()) + (ad_dmg() * ad_dmg_m())) * non_true_dmg_m()) +
            (true_dmg() * true_dmg_m())) *
           dmg_m();
  };
};

} // namespace moba
