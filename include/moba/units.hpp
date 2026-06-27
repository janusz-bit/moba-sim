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
  void operator+=(const EquationDamage &other) {
    ap_dmg.numbers.insert(
      ap_dmg.numbers.end(), other.ap_dmg.numbers.begin(), other.ap_dmg.numbers.end());
    ad_dmg.numbers.insert(
      ad_dmg.numbers.end(), other.ad_dmg.numbers.begin(), other.ad_dmg.numbers.end());
    true_dmg.numbers.insert(
      true_dmg.numbers.end(), other.true_dmg.numbers.begin(), other.true_dmg.numbers.end());
    ap_dmg_m.numbers.insert(
      ap_dmg_m.numbers.end(), other.ap_dmg_m.numbers.begin(), other.ap_dmg_m.numbers.end());
    ad_dmg_m.numbers.insert(
      ad_dmg_m.numbers.end(), other.ad_dmg_m.numbers.begin(), other.ad_dmg_m.numbers.end());
    true_dmg_m.numbers.insert(
      true_dmg_m.numbers.end(), other.true_dmg_m.numbers.begin(), other.true_dmg_m.numbers.end());
    non_true_dmg_m.numbers.insert(non_true_dmg_m.numbers.end(),
                                  other.non_true_dmg_m.numbers.begin(),
                                  other.non_true_dmg_m.numbers.end());
    dmg_m.numbers.insert(
      dmg_m.numbers.end(), other.dmg_m.numbers.begin(), other.dmg_m.numbers.end());
  };
};

struct Champion {
  Type hp{hp_max}, hp_max{100}, ad{50}, ap{}, armor_ad{}, armor_ap{}, attack_speed{}, cdr{},
    hp_regen{}, mana{100}, mana_max{};
};

} // namespace moba
