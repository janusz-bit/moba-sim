#define BOOST_TEST_MODULE moba_tests
#include <boost/test/included/unit_test.hpp>

#include "moba/units.hpp"

#include <cmath>
#include <type_traits>

// --- calculateReduction ---

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

// --- ModDB ---

BOOST_AUTO_TEST_CASE(moddb_add_and_sum_exclude) {
  moba::ModDB db;
  db.add(moba::Stat::HP, moba::ModType::Base, 1000, "Base");
  db.add(moba::Stat::HP, moba::ModType::Base, 500, "Item: Warmog");
  db.add(moba::Stat::HP, moba::ModType::Inc, 10, "Rune");
  BOOST_TEST(db.sumExclude(moba::Stat::HP, moba::ModType::Base, "Base") == 500.0);
  BOOST_TEST(db.sumExclude(moba::Stat::HP, moba::ModType::Base, "") == 1500.0);
  BOOST_TEST(db.sumExclude(moba::Stat::HP, moba::ModType::Inc, "Base") == 10.0);
}

BOOST_AUTO_TEST_CASE(moddb_more_exclude_is_multiplicative) {
  moba::ModDB db;
  db.add(moba::Stat::AP, moba::ModType::More, 35, "Rabadon");
  db.add(moba::Stat::AP, moba::ModType::More, 20, "Other");
  double expected = 1.35 * 1.20;
  BOOST_TEST(std::abs(db.moreExclude(moba::Stat::AP, "") - expected) < 0.001);
  BOOST_TEST(std::abs(db.moreExclude(moba::Stat::AP, "Rabadon") - 1.20) < 0.001);
}

BOOST_AUTO_TEST_CASE(moddb_replace_existing) {
  moba::ModDB db;
  db.add(moba::Stat::HP, moba::ModType::Base, 100, "Base");
  db.replace(moba::Stat::HP, moba::ModType::Base, 200, "Base");
  BOOST_TEST(db.sumExclude(moba::Stat::HP, moba::ModType::Base, "") == 200.0);
}

BOOST_AUTO_TEST_CASE(moddb_replace_creates_if_missing) {
  moba::ModDB db;
  db.replace(moba::Stat::AP, moba::ModType::Base, 50, "Passive");
  BOOST_TEST(db.sumExclude(moba::Stat::AP, moba::ModType::Base, "") == 50.0);
}

BOOST_AUTO_TEST_CASE(moddb_remove_by_source) {
  moba::ModDB db;
  db.add(moba::Stat::HP, moba::ModType::Base, 1000, "Base");
  db.add(moba::Stat::HP, moba::ModType::Base, 500, "Item: Warmog");
  BOOST_TEST(db.removeBySource("Item: Warmog") == true);
  BOOST_TEST(db.sumExclude(moba::Stat::HP, moba::ModType::Base, "") == 1000.0);
  BOOST_TEST(db.removeBySource("Nonexistent") == false);
}

BOOST_AUTO_TEST_CASE(moddb_get_base_from_source) {
  moba::ModDB db;
  db.add(moba::Stat::HP, moba::ModType::Base, 2300, "Base");
  db.add(moba::Stat::HP, moba::ModType::Base, 1000, "Item: Warmog");
  BOOST_TEST(db.getBaseFromSource(moba::Stat::HP) == 2300.0);
  BOOST_TEST(db.getBaseFromSource(moba::Stat::AP) == 0.0);
}

// --- Damage ---

BOOST_AUTO_TEST_CASE(damage_get_final_physical_with_multiplier) {
  moba::Damage dmg;
  dmg.physical = 100;
  dmg.physicalMult = 1.5;
  BOOST_TEST(dmg.getFinalPhysical() == 150.0);
}

BOOST_AUTO_TEST_CASE(damage_get_total_sums_all_types) {
  moba::Damage dmg;
  dmg.physical = 100;
  dmg.physicalMult = 2.0;
  dmg.magic = 50;
  dmg.trueDmg = 25;
  BOOST_TEST(dmg.getTotal() == 275.0);
}

BOOST_AUTO_TEST_CASE(damage_physical_multiplier_is_multiplicative) {
  moba::Damage dmg;
  dmg.physical = 100;
  dmg.applyMultiplier(1.5);
  dmg.applyMultiplier(1.5);
  // 100 * 1.5 * 1.5 = 225, not 100 * 2.0 = 200
  BOOST_TEST(dmg.getFinalPhysical() == 225.0);
}

// --- calculateSingleStat ---

BOOST_AUTO_TEST_CASE(calculate_single_stat_base_inc_more) {
  moba::Champion champ;
  champ.modDB.add(moba::Stat::AD, moba::ModType::Base, 100, "Base");
  champ.modDB.add(moba::Stat::AD, moba::ModType::Inc, 50, "Rune");
  champ.modDB.add(moba::Stat::AD, moba::ModType::More, 20, "Item");
  // 100 * (1 + 0.5) * 1.2 = 180
  BOOST_TEST(std::abs(moba::calculateSingleStat(champ, moba::Stat::AD) - 180.0) < 0.01);
}

BOOST_AUTO_TEST_CASE(calculate_single_stat_with_exclude) {
  moba::Champion champ;
  champ.modDB.add(moba::Stat::HP, moba::ModType::Base, 2000, "Base");
  champ.modDB.add(moba::Stat::HP, moba::ModType::Base, 500, "Item: Warmog");
  // Excluding "Item: Warmog" should give only base: 2000
  BOOST_TEST(moba::calculateSingleStat(champ, moba::Stat::HP, "Item: Warmog") == 2000.0);
}

// --- MS soft-cap ---

BOOST_AUTO_TEST_CASE(apply_ms_soft_cap_below_threshold_no_change) {
  moba::Champion champ;
  champ.stats[moba::Stat::MS] = 350;
  moba::applyMsSoftCap(champ);
  BOOST_TEST(champ.stats[moba::Stat::MS] == 350.0);
}

BOOST_AUTO_TEST_CASE(apply_ms_soft_cap_between_415_and_490) {
  moba::Champion champ;
  champ.stats[moba::Stat::MS] = 450;
  moba::applyMsSoftCap(champ);
  // 415 + 35 * 0.8 = 443
  BOOST_TEST(std::abs(champ.stats[moba::Stat::MS] - 443.0) < 0.01);
}

BOOST_AUTO_TEST_CASE(apply_ms_soft_cap_above_490) {
  moba::Champion champ;
  champ.stats[moba::Stat::MS] = 500;
  moba::applyMsSoftCap(champ);
  // 415 + 75 * 0.8 + 10 * 0.5 = 415 + 60 + 5 = 480
  BOOST_TEST(std::abs(champ.stats[moba::Stat::MS] - 480.0) < 0.01);
}

// --- Combat simulation ---

BOOST_AUTO_TEST_CASE(simulate_attack_basic_ad_vs_armor) {
  moba::Champion attacker;
  attacker.name = "Attacker";
  attacker.modDB.add(moba::Stat::AD, moba::ModType::Base, 100, "Base");
  attacker.stats[moba::Stat::AD] = 100;

  moba::Champion defender;
  defender.name = "Defender";
  defender.modDB.add(moba::Stat::Armor, moba::ModType::Base, 100, "Base");
  defender.stats[moba::Stat::Armor] = 100;

  auto dmg = moba::simulateAttack(attacker, defender);
  // 100 AD, armor 100 -> 100 * 0.5 = 50
  BOOST_TEST(std::abs(dmg.getTotal() - 50.0) < 0.01);
}

BOOST_AUTO_TEST_CASE(simulate_attack_magic_vs_mr) {
  moba::Champion attacker;
  attacker.name = "Attacker";
  attacker.modDB.add(moba::Stat::AD, moba::ModType::Base, 0, "Base");
  attacker.stats[moba::Stat::AD] = 0;
  // Add a combat passive that deals magic damage
  attacker.combatPassives.push_back(moba::CombatOverlay{
    "Test Magic", "",
    [](const moba::Damage&, const moba::Damage&, const moba::Champion&, const moba::Champion&) {
      moba::Damage delta;
      delta.magic = 100;
      return delta;
    }
  });

  moba::Champion defender;
  defender.name = "Defender";
  defender.modDB.add(moba::Stat::Armor, moba::ModType::Base, 100, "Base");
  defender.modDB.add(moba::Stat::MR, moba::ModType::Base, 50, "Base");
  defender.stats[moba::Stat::Armor] = 100;
  defender.stats[moba::Stat::MR] = 50;

  auto dmg = moba::simulateAttack(attacker, defender);
  // 100 magic, MR 50 -> 100 * (100/150) = 66.67
  BOOST_TEST(std::abs(dmg.getTotal() - 66.667) < 0.1);
}

// --- Enum properties ---

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