#define BOOST_TEST_MODULE moba_tests
#include <boost/test/included/unit_test.hpp>

#include "moba/units.hpp"

#include <type_traits>

BOOST_AUTO_TEST_CASE(resistances_default_zero) {
  moba::Resistances r;
  BOOST_TEST(r.getArmor() == 0.0);
  BOOST_TEST(r.getMagicResistance() == 0.0);
}

BOOST_AUTO_TEST_CASE(withhp_starts_full) {
  moba::WithHP hp;
  BOOST_TEST(hp.currentHp() == hp.maxHp());
  BOOST_TEST(hp.maxHp() == 100.0);
}

BOOST_AUTO_TEST_CASE(alive_has_resistances_and_hp) {
  moba::Alive a;
  BOOST_TEST(a.getArmor() == 0.0);
  BOOST_TEST(a.getMagicResistance() == 0.0);
  BOOST_TEST(a.currentHp() == a.maxHp());
}

BOOST_AUTO_TEST_CASE(damage_base_default_physical) {
  moba::DamageBase d;
  BOOST_TEST(static_cast<std::int8_t>(d.kind) ==
             static_cast<std::int8_t>(moba::TypeDamage::Physical));
  BOOST_TEST(std::get<moba::Type>(d.amount) == moba::Type{0});
}

BOOST_AUTO_TEST_CASE(kind_damage_underlying_int8) {
  static_assert(std::is_same_v<std::underlying_type_t<moba::TypeDamage>, std::int8_t>);
  BOOST_TEST(true);
}
