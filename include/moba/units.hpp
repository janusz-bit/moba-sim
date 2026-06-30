#pragma once

#include <algorithm>
#include <array>
#include <cstdint>
#include <functional>
#include <string>
#include <unordered_map>
#include <vector>

namespace moba {

// --- Enums ---

enum class Stat { HP, AP, AD, MS, Armor, MR };
enum class ModType { Base, Inc, More };

enum class TypeDamage : std::uint8_t { Physical, Magic, True, Size };
enum class KindDamage : std::uint8_t { AutoAttack, OnHit, Spell, Size };

// --- Helpers ---

inline std::string statToString(Stat stat) {
  switch (stat) {
    case Stat::HP: return "HP";
    case Stat::AP: return "AP";
    case Stat::AD: return "AD";
    case Stat::MS: return "MS";
    case Stat::Armor: return "Armor";
    case Stat::MR: return "MR";
  }
  return "Unknown";
}

// Resistance formula: handles positive (reduction) and negative (penetration) values.
// Positive: 100 / (100 + resistance) — standard LoL formula.
// Negative: 2 - 100 / (100 - resistance) — amplifies damage beyond 100%.
inline double calculateReduction(double resistance) {
  if (resistance >= 0.0) {
    return 100.0 / (100.0 + resistance);
  }
  return 2.0 - (100.0 / (100.0 - resistance));
}

// --- Modifier & ModDB ---

struct Modifier {
  Stat stat;
  ModType type;
  double value;
  std::string source;
};

class ModDB {
  std::vector<Modifier> mods_;

public:
  void add(Stat stat, ModType type, double value, const std::string& source) {
    mods_.push_back({stat, type, value, source});
  }

  // Sum all modifiers of (stat, type) excluding those from exclude_source.
  // Used to break circular dependencies (e.g. Vladimir's AP<->HP passive).
  [[nodiscard]] double sumExclude(Stat stat, ModType type, const std::string& exclude_source) const {
    double total = 0.0;
    for (const auto& m : mods_) {
      if (m.stat == stat && m.type == type && m.source != exclude_source) {
        total += m.value;
      }
    }
    return total;
  }

  // Multiplicative product of all MORE modifiers for stat, excluding exclude_source.
  [[nodiscard]] double moreExclude(Stat stat, const std::string& exclude_source) const {
    double multiplier = 1.0;
    for (const auto& m : mods_) {
      if (m.stat == stat && m.type == ModType::More && m.source != exclude_source) {
        multiplier *= (1.0 + m.value / 100.0);
      }
    }
    return multiplier;
  }

  // Insert or update a modifier matching (stat, type, source).
  void replace(Stat stat, ModType type, double value, const std::string& source) {
    auto it = std::find_if(mods_.begin(), mods_.end(), [&](const Modifier& m) {
      return m.stat == stat && m.type == type && m.source == source;
    });
    if (it != mods_.end()) {
      it->value = value;
    } else {
      mods_.push_back({stat, type, value, source});
    }
  }

  // Remove all modifiers from a given source (e.g. when unequipping an item).
  [[nodiscard]] bool removeBySource(const std::string& source) {
    auto it = std::remove_if(mods_.begin(), mods_.end(),
                            [&](const Modifier& m) { return m.source == source; });
    if (it == mods_.end()) {
      return false;
    }
    mods_.erase(it, mods_.end());
    return true;
  }

  // Get the BASE modifier value for stat that comes specifically from source "Base".
  // Used to separate natural base stats from bonus stats (e.g. bonus HP for conversions).
  [[nodiscard]] double getBaseFromSource(Stat stat) const {
    for (const auto& m : mods_) {
      if (m.stat == stat && m.type == ModType::Base && m.source == "Base") {
        return m.value;
      }
    }
    return 0.0;
  }

  [[nodiscard]] const std::vector<Modifier>& mods() const { return mods_; }
};

// --- Damage ---

struct Damage {
  double physical = 0.0;
  double magic = 0.0;
  double trueDmg = 0.0;
  double physicalMult = 1.0;

  // Multipliers are multiplicative: two 1.5x overlays yield 2.25x, not 2.0x.
  void applyMultiplier(double mult) { physicalMult *= mult; }

  [[nodiscard]] double getFinalPhysical() const { return physical * physicalMult; }
  [[nodiscard]] double getTotal() const { return getFinalPhysical() + magic + trueDmg; }
};

// --- Champion & Passives ---

struct Champion;

using StatPassiveFn = std::function<void(const Champion&, ModDB&)>;

struct StatPassiveEntry {
  std::string source;
  StatPassiveFn func;
};

struct CombatOverlay {
  std::string name;
  std::string source; // empty for system overlays, "Item: X" for item passives
  std::function<Damage(const Damage& finalDmg, const Damage& prevDmg, const Champion& attacker,
                       const Champion& defender)>
    apply;
};

struct Champion {
  std::string name;
  ModDB modDB;
  std::unordered_map<Stat, double> stats;     // post-soft-cap values
  std::unordered_map<Stat, double> rawStats;    // pre-soft-cap values (for passive calculations)
  double currentHp = 0.0;
  std::vector<StatPassiveEntry> statPassives;
  std::vector<CombatOverlay> combatPassives;

  [[nodiscard]] double get(Stat stat) const {
    auto it = stats.find(stat);
    return it != stats.end() ? it->second : 0.0;
  }

  // Raw stat before soft-caps — use in passives that need unsoftcapped values.
  [[nodiscard]] double getRaw(Stat stat) const {
    auto it = rawStats.find(stat);
    return it != rawStats.end() ? it->second : 0.0;
  }
};

// --- Stat Calculation ---

// Compute a single stat from BASE + INC + MORE modifiers, optionally excluding
// one source to break circular dependencies.
inline double calculateSingleStat(const Champion& champ, Stat stat,
                                  const std::string& exclude_source = "") {
  double base = champ.modDB.sumExclude(stat, ModType::Base, exclude_source);
  double inc = champ.modDB.sumExclude(stat, ModType::Inc, exclude_source);
  double more = champ.modDB.moreExclude(stat, exclude_source);
  return base * (1.0 + inc / 100.0) * more;
}

// LoL movement speed soft-caps: MS above 415 is reduced, above 490 is reduced further.
inline void applyMsSoftCap(Champion& champ) {
  double rawMs = champ.stats[Stat::MS];
  if (rawMs <= 415.0) {
    return;
  }
  if (rawMs > 490.0) {
    champ.stats[Stat::MS] = 415.0 + (490.0 - 415.0) * 0.8 + (rawMs - 490.0) * 0.5;
  } else {
    champ.stats[Stat::MS] = 415.0 + (rawMs - 415.0) * 0.8;
  }
}

// Run 3 iterations of stat calculation + passive resolution.
// Multiple iterations let circular passives (e.g. Vladimir AP<->HP) converge.
inline void updateChampionStats(Champion& champ) {
  static constexpr std::array statList{Stat::HP, Stat::AP, Stat::AD, Stat::MS, Stat::Armor, Stat::MR};

  for (int iter = 0; iter < 3; ++iter) {
    for (auto stat : statList) {
      champ.stats[stat] = calculateSingleStat(champ, stat);
    }
    for (auto& passive : champ.statPassives) {
      passive.func(champ, champ.modDB);
    }
  }

  // Snapshot raw stats before soft-caps so passives can read unsoftcapped values.
  champ.rawStats = champ.stats;
  applyMsSoftCap(champ);
}

// --- Items ---

struct Item {
  std::string name;
  std::vector<Modifier> statModifiers;
  std::vector<StatPassiveFn> statPassives;
  std::vector<CombatOverlay> combatPassives;
};

inline std::string itemSource(const Item& item) { return "Item: " + item.name; }

inline void equipItem(Champion& champ, const Item& item) {
  auto source = itemSource(item);
  for (const auto& mod : item.statModifiers) {
    champ.modDB.add(mod.stat, mod.type, mod.value, source);
  }
  for (const auto& sp : item.statPassives) {
    champ.statPassives.push_back({source, sp});
  }
  for (const auto& cp : item.combatPassives) {
    CombatOverlay tagged = cp;
    tagged.source = source;
    champ.combatPassives.push_back(tagged);
  }
}

inline void unequipItem(Champion& champ, const Item& item) {
  auto source = itemSource(item);
  (void)champ.modDB.removeBySource(source);

  auto spIt = std::remove_if(champ.statPassives.begin(), champ.statPassives.end(),
                            [&](const StatPassiveEntry& e) { return e.source == source; });
  champ.statPassives.erase(spIt, champ.statPassives.end());

  auto cpIt = std::remove_if(champ.combatPassives.begin(), champ.combatPassives.end(),
                            [&](const CombatOverlay& c) { return c.source == source; });
  champ.combatPassives.erase(cpIt, champ.combatPassives.end());
}

// --- Combat Engine ---

// Build a damage pipeline from the attacker's base auto-attack + combat passives,
// then apply defender's armor and MR reductions. The pipeline runs 2 iterations:
// the second pass gives overlays access to the first pass's final damage, enabling
// effects that react to computed totals (e.g. execute thresholds).
inline Damage simulateAttack(const Champion& attacker, const Champion& defender) {
  std::vector<CombatOverlay> pipeline;

  // 1. Base auto-attack — always present, deals AD as physical damage.
  pipeline.push_back(CombatOverlay{
    "Base Auto-Attack (AD)", "",
    [](const Damage&, const Damage&, const Champion& att, const Champion&) {
      Damage delta;
      delta.physical = att.get(Stat::AD);
      return delta;
    }
  });

  // 2. Attacker's combat passives (e.g. Vayne Silver Bolts, BotRK on-hit, Janna Tailwind).
  for (const auto& cp : attacker.combatPassives) {
    pipeline.push_back(cp);
  }

  // 3. Armor reduction (physical damage only).
  pipeline.push_back(CombatOverlay{
    "Armor Reduction", "",
    [](const Damage&, const Damage& prev, const Champion&, const Champion& def) {
      Damage delta;
      double rawPhys = prev.physical;
      double reducedPhys = rawPhys * calculateReduction(def.get(Stat::Armor));
      delta.physical = reducedPhys - rawPhys;
      return delta;
    }
  });

  // 4. MR reduction (magic damage only).
  pipeline.push_back(CombatOverlay{
    "MR Reduction", "",
    [](const Damage&, const Damage& prev, const Champion&, const Champion& def) {
      Damage delta;
      double rawMagic = prev.magic;
      double reducedMagic = rawMagic * calculateReduction(def.get(Stat::MR));
      delta.magic = reducedMagic - rawMagic;
      return delta;
    }
  });

  // Run pipeline.
  Damage finalDmg;
  for (int iter = 0; iter < 2; ++iter) {
    Damage currentDmg;
    for (const auto& overlay : pipeline) {
      Damage delta = overlay.apply(finalDmg, currentDmg, attacker, defender);
      currentDmg.physical += delta.physical;
      currentDmg.magic += delta.magic;
      currentDmg.trueDmg += delta.trueDmg;
      currentDmg.physicalMult *= delta.physicalMult;
    }
    finalDmg = currentDmg;
  }

  return finalDmg;
}

} // namespace moba