#define BOOST_TEST_MODULE moba_tests
#include <boost/test/included/unit_test.hpp>

#include "moba/units.hpp"

#include <type_traits>

BOOST_AUTO_TEST_CASE(calculate_reduction_no_resistance_full_damage) {
  BOOST_TEST(moba::calculateReduction(0.0) == 1.0);
}

BOOST_AUTO_TEST_CASE(calculate_reduction_positive_halves_damage) {
  BOOST_TEST(moba::calculateReduction(100.0) == 0.5);
}

BOOST_AUTO_TEST_CASE(calculate_reduction_negative_increases_damage) {
  BOOST_TEST(moba::calculateReduction(-50.0) == 2.0 - (100.0 / 150.0));
}

BOOST_AUTO_TEST_CASE(calculate_reduction_neg100_gives_one_and_a_half) {
  BOOST_TEST(moba::calculateReduction(-100.0) == 1.5);
}

BOOST_AUTO_TEST_CASE(additive_empty_is_zero) {
  moba::Additive a;
  BOOST_TEST(a() == 0.0);
}

BOOST_AUTO_TEST_CASE(additive_sums_elements) {
  moba::Additive a;
  a.numbers = {2.0, 3.0, 5.0};
  BOOST_TEST(a() == 10.0);
}

BOOST_AUTO_TEST_CASE(multiplitive_empty_is_one) {
  moba::Multiplitive m;
  BOOST_TEST(m() == 1.0);
}

BOOST_AUTO_TEST_CASE(multiplitive_multiplies_elements) {
  moba::Multiplitive m;
  m.numbers = {2.0, 3.0, 0.5};
  BOOST_TEST(m() == 3.0);
}

BOOST_AUTO_TEST_CASE(equation_damage_all_empty_is_zero) {
  moba::EquationDamage e;
  BOOST_TEST(e() == 0.0);
}

BOOST_AUTO_TEST_CASE(equation_damage_sums_components) {
  moba::EquationDamage e;
  e.ap_dmg.numbers   = {10.0};
  e.ad_dmg.numbers   = {20.0};
  e.true_dmg.numbers = {5.0};
  BOOST_TEST(e() == 35.0);
}

BOOST_AUTO_TEST_CASE(equation_damage_non_true_multiplier_applies_to_ap_and_ad) {
  moba::EquationDamage e;
  e.ap_dmg.numbers         = {10.0};
  e.ad_dmg.numbers         = {20.0};
  e.true_dmg.numbers       = {5.0};
  e.non_true_dmg_m.numbers = {2.0};
  BOOST_TEST(e() == 65.0);
}

BOOST_AUTO_TEST_CASE(equation_damage_global_multiplier_scales_total) {
  moba::EquationDamage e;
  e.ap_dmg.numbers   = {10.0};
  e.ad_dmg.numbers   = {20.0};
  e.true_dmg.numbers = {5.0};
  e.dmg_m.numbers    = {0.5};
  BOOST_TEST(e() == 17.5);
}

BOOST_AUTO_TEST_CASE(type_damage_underlying_uint8) {
  static_assert(std::is_same_v<std::underlying_type_t<moba::TypeDamage>, std::uint8_t>);
  BOOST_TEST(true);
}

BOOST_AUTO_TEST_CASE(type_damage_size_is_three) {
  static_assert(static_cast<std::uint8_t>(moba::TypeDamage::Size) == 3);
  BOOST_TEST(true);
}

BOOST_AUTO_TEST_CASE(kind_damage_size_is_three) {
  static_assert(static_cast<std::uint8_t>(moba::KindDamage::Size) == 3);
  BOOST_TEST(true);
}
