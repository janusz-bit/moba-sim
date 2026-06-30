#include "moba/units.hpp"

#include <array>
#include <iomanip>
#include <iostream>

using namespace moba;

// --- Items ---

Item createWarmog() {
  return Item{
    "Warmog's Armor",
    {{Stat::HP, ModType::Base, 1000.0, "Warmog"}},
    {},
    {}
  };
}

Item createRabadon() {
  return Item{
    "Rabadon's Deathcap",
    {{Stat::AP, ModType::Base, 140.0, "Rabadon"}},
    {[](const Champion&, ModDB& db) {
       db.replace(Stat::AP, ModType::More, 35.0, "Item: Rabadon Passive");
     }},
    {}
  };
}

Item createBotRK() {
  return Item{
    "Blade of the Ruined King",
    {{Stat::AD, ModType::Base, 40.0, "BotRK"}},
    {},
    {CombatOverlay{
       "BotRK On-Hit (9% Current HP)", "",
       [](const Damage&, const Damage&, const Champion&, const Champion& def) {
         Damage delta;
         delta.physical = def.currentHp * 0.09;
         return delta;
       }
     }}
  };
}

// --- Champion init ---

void initVladimir(Champion& champ) {
  champ.name = "Vladimir";
  champ.modDB.add(Stat::HP, ModType::Base, 2300, "Base");
  champ.modDB.add(Stat::AP, ModType::Base, 0, "Base");
  champ.modDB.add(Stat::AD, ModType::Base, 100, "Base");
  champ.modDB.add(Stat::MS, ModType::Base, 335, "Base");
  champ.modDB.add(Stat::Armor, ModType::Base, 80, "Base");
  champ.modDB.add(Stat::MR, ModType::Base, 50, "Base");

  // Crimson Pact: AP gives +140% AP as bonus HP, bonus HP gives +2.5% as AP.
  // Uses exclude_source to break the circular dependency.
  champ.statPassives.push_back({"Passive: Crimson Pact", [](const Champion& champ, ModDB& db) {
                                  // AP -> HP (exclude AP-from-HP to avoid counting passive AP)
                                  double apExcl = calculateSingleStat(champ, Stat::AP, "Passive: AP from HP");
                                  double hpFromAp = apExcl * 1.4;
                                  db.replace(Stat::HP, ModType::Base, hpFromAp, "Passive: HP from AP");

                                  // HP -> AP (exclude HP-from-AP, only count bonus HP from items/runes)
                                  double hpExcl = calculateSingleStat(champ, Stat::HP, "Passive: HP from AP");
                                  double naturalBase = db.getBaseFromSource(Stat::HP);
                                  double bonusHp = hpExcl - naturalBase;
                                  double apFromHp = (bonusHp > 0) ? bonusHp * 0.025 : 0.0;
                                  db.replace(Stat::AP, ModType::Base, apFromHp, "Passive: AP from HP");
                                }});
}

void initVayne(Champion& champ) {
  champ.name = "Vayne";
  champ.modDB.add(Stat::HP, ModType::Base, 2100, "Base");
  champ.modDB.add(Stat::AP, ModType::Base, 0, "Base");
  champ.modDB.add(Stat::AD, ModType::Base, 110, "Base");
  champ.modDB.add(Stat::MS, ModType::Base, 330, "Base");
  champ.modDB.add(Stat::Armor, ModType::Base, 70, "Base");
  champ.modDB.add(Stat::MR, ModType::Base, 50, "Base");

  // Silver Bolts: 8% of target's max HP as true damage.
  champ.combatPassives.push_back(CombatOverlay{
    "Vayne W - Silver Bolts (8% Max HP True Damage)", "",
    [](const Damage&, const Damage&, const Champion&, const Champion& def) {
      Damage delta;
      delta.trueDmg = def.get(Stat::HP) * 0.08;
      return delta;
    }
  });
}

void initJanna(Champion& champ) {
  champ.name = "Janna";
  champ.modDB.add(Stat::HP, ModType::Base, 2000, "Base");
  champ.modDB.add(Stat::AP, ModType::Base, 0, "Base");
  champ.modDB.add(Stat::AD, ModType::Base, 90, "Base");
  champ.modDB.add(Stat::MS, ModType::Base, 335, "Base");
  champ.modDB.add(Stat::Armor, ModType::Base, 70, "Base");
  champ.modDB.add(Stat::MR, ModType::Base, 50, "Base");

  // Tailwind stat passive: +8% INC MS.
  champ.statPassives.push_back({"Passive: Tailwind MS", [](const Champion&, ModDB& db) {
                                  db.replace(Stat::MS, ModType::Inc, 8.0, "Passive: Tailwind MS");
                                }});

  // Tailwind combat passive: 20% of bonus MS as physical damage.
  // Uses getRaw (pre-soft-cap) so soft-cap doesn't skew the bonus calculation.
  champ.combatPassives.push_back(CombatOverlay{
    "Janna Tailwind (20% Bonus MS as Physical)", "",
    [](const Damage&, const Damage&, const Champion& att, const Champion&) {
      Damage delta;
      double baseMs = att.modDB.sumExclude(Stat::MS, ModType::Base, "Passive: Tailwind MS");
      double bonusMs = att.getRaw(Stat::MS) - baseMs;
      if (bonusMs > 0) {
        delta.physical = bonusMs * 0.20;
      }
      return delta;
    }
  });
}

// --- Output helpers ---

void printChampionStats(const Champion& champ) {
  static constexpr std::array statList{Stat::HP, Stat::AP, Stat::AD, Stat::MS, Stat::Armor, Stat::MR};
  std::cout << "  Stats for " << champ.name << ":\n";
  for (auto stat : statList) {
    std::cout << "    " << std::left << std::setw(8) << statToString(stat) << ": " << champ.get(stat) << "\n";
  }
}

void printDamage(const Damage& dmg) {
  std::cout << "  Damage breakdown:\n";
  std::cout << "    Physical: " << dmg.getFinalPhysical()
            << " (base=" << dmg.physical << ", mult=" << dmg.physicalMult << "x)\n";
  std::cout << "    Magic:    " << dmg.magic << "\n";
  std::cout << "    True:     " << dmg.trueDmg << "\n";
  std::cout << "    Total:    " << dmg.getTotal() << "\n";
}

// --- Main ---

int main() {
  std::cout << "moba-sim combat simulation\n";
  std::cout << "===========================\n\n";

  // Target dummy — 3000 HP, 100 Armor, 50 MR, currently at 2000 HP.
  Champion dummy;
  dummy.name = "Target Dummy";
  dummy.modDB.add(Stat::HP, ModType::Base, 3000, "Base");
  dummy.modDB.add(Stat::Armor, ModType::Base, 100, "Base");
  dummy.modDB.add(Stat::MR, ModType::Base, 50, "Base");
  updateChampionStats(dummy);
  dummy.currentHp = 2000;

  auto warmog = createWarmog();
  auto rabadon = createRabadon();
  auto botrk = createBotRK();

  // --- Test 1: Vladimir (Warmog + Rabadon) ---
  std::cout << "--- TEST 1: Vladimir (Warmog + Rabadon) ---\n";
  Champion vlad;
  initVladimir(vlad);
  equipItem(vlad, warmog);
  equipItem(vlad, rabadon);
  updateChampionStats(vlad);
  printChampionStats(vlad);
  std::cout << "  Attack -> Target Dummy:\n";
  printDamage(simulateAttack(vlad, dummy));

  // --- Test 2: Vayne (BotRK) ---
  std::cout << "\n--- TEST 2: Vayne (BotRK) ---\n";
  Champion vayne;
  initVayne(vayne);
  equipItem(vayne, botrk);
  updateChampionStats(vayne);
  printChampionStats(vayne);
  std::cout << "  Attack -> Target Dummy:\n";
  printDamage(simulateAttack(vayne, dummy));

  // --- Test 3: Janna (no items) ---
  std::cout << "\n--- TEST 3: Janna (no items) ---\n";
  Champion janna;
  initJanna(janna);
  updateChampionStats(janna);
  printChampionStats(janna);
  std::cout << "  Attack -> Target Dummy:\n";
  printDamage(simulateAttack(janna, dummy));
}