//
// RaidEncounterRegistry — see header for design notes.
//
// Each boss entry lists per-role strategy bundles. At runtime, when a bot's
// target changes to a registered boss, the SoloCommander hook resolves the
// bundle for the bot's (role, class) pair and applies it via ChangeStrategy.
//
// Strategies use cmangos's `+a,-b` syntax. Some tokens here are aspirational
// (e.g. "+frenzy dispel hunter") and represent strategies we'd LIKE the bot
// to have. cmangos's existing strategy registry ignores unknown tokens
// silently, so adding a strategy here that doesn't yet exist as a class is
// harmless — it just means the encounter doesn't get that behavior until
// the strategy class is implemented (next-sprint work).
//

#include "RaidEncounterRegistry.h"
#include <cctype>

namespace ai
{
namespace solocommander
{
    namespace
    {
        // The full encounter table. Initialised once at first-Lookup.
        const std::unordered_map<uint32, RaidEncounter>& GetTable()
        {
            static const std::unordered_map<uint32, RaidEncounter> kTable = {
                // ====================================================
                // MOLTEN CORE
                // ====================================================
                { 11982, { 11982, "Lucifron", "Molten Core",
                    {
                        { BOT_ROLE_NONE,   "druid",   "+decurse curse of lucifron", "Druid decurses Lucifron's Curse on raid" },
                        { BOT_ROLE_NONE,   "mage",    "+decurse curse of lucifron", "Mage decurses Lucifron's Curse on raid" },
                        { BOT_ROLE_HEALER, "priest",  "+dispel magic dominate mind", "Priest dispels Mind Control" },
                        { BOT_ROLE_TANK,   "",        "+kill protectors first", "Tank protectors before Lucifron" },
                    }
                }},
                { 11988, { 11988, "Magmadar", "Molten Core",
                    {
                        { BOT_ROLE_NONE,   "hunter", "+tranquilizing shot frenzy", "Hunter dispels Magmadar's Frenzy" },
                        { BOT_ROLE_TANK,   "",       "+face away from melee",      "Avoid Lava Breath cone" },
                        { BOT_ROLE_HEALER, "priest", "+fear ward",                  "Pre-Fear Ward tank vs Panic" },
                        { BOT_ROLE_HEALER, "shaman", "+tremor totem",              "Shaman drops Tremor for Panic" },
                        { BOT_ROLE_NONE,   "",       "+move out of fire patches",  "Avoid Lava Bomb fire patches" },
                    }
                }},
                { 12259, { 12259, "Gehennas", "Molten Core",
                    {
                        { BOT_ROLE_NONE,   "druid", "+decurse curse of gehennas", "Druid decurse" },
                        { BOT_ROLE_NONE,   "mage",  "+decurse curse of gehennas", "Mage decurse" },
                        { BOT_ROLE_NONE,   "",      "+move out of rain of fire",   "Avoid AoE patches" },
                        { BOT_ROLE_TANK,   "",      "+kill flamewakers first",     "Adds before boss" },
                    }
                }},
                { 12057, { 12057, "Garr", "Molten Core",
                    {
                        { BOT_ROLE_NONE, "warlock", "+banish firesworn", "Banish adds (primary CC)" },
                        { BOT_ROLE_NONE, "mage",    "+polymorph firesworn", "Sheep adds (secondary CC)" },
                        { BOT_ROLE_NONE, "",        "+ranged stay back",  "Magma Shackles slow at melee range" },
                        { BOT_ROLE_TANK, "",        "+kill firesworn carefully", "1-by-1, Eruption AoE on death" },
                    }
                }},
                { 12056, { 12056, "Baron Geddon", "Molten Core",
                    {
                        { BOT_ROLE_NONE,   "", "+living bomb runaway",   "Run if Living Bomb debuff applied" },
                        { BOT_ROLE_NONE,   "", "+geddon ranged spread",  "Ranged stay 25y from boss (Inferno)" },
                        { BOT_ROLE_HEALER, "", "+armageddon awareness",  "10% HP = boss self-explodes" },
                    }
                }},
                { 12264, { 12264, "Shazzrah", "Molten Core",
                    {
                        { BOT_ROLE_NONE, "druid", "+decurse curse of shazzrah", "Druid decurse" },
                        { BOT_ROLE_NONE, "mage",  "+decurse curse of shazzrah", "Mage decurse" },
                        { BOT_ROLE_NONE, "mage",  "+counterspell magic grounding", "Counter heal-buff" },
                        { BOT_ROLE_TANK, "",      "+chase teleport",            "Re-aggro after Shazzrah Gate" },
                        { BOT_ROLE_NONE, "",      "+spread 8y",                  "Avoid teleport-cluster" },
                    }
                }},
                { 11990, { 11990, "Golemagg the Incinerator", "Molten Core",
                    {
                        { BOT_ROLE_TANK,   "",     "+threat focus on rager",  "Adds tied to boss HP, kill first" },
                        { BOT_ROLE_NONE,   "mage", "+counterspell pyroblast", "Interrupt Pyroblast cast" },
                        { BOT_ROLE_HEALER, "",     "+earthquake awareness",   "10% HP = AoE pulses on raid" },
                    }
                }},
                { 12098, { 12098, "Sulfuron Harbinger", "Molten Core",
                    {
                        { BOT_ROLE_NONE, "mage",   "+counterspell priest heal", "Top-priority interrupt" },
                        { BOT_ROLE_NONE, "rogue",  "+kick priest heal",          "Top-priority interrupt" },
                        { BOT_ROLE_NONE, "warrior","+pummel priest heal",        "Berserker stance interrupt" },
                        { BOT_ROLE_TANK, "",       "+kill priests first",        "4 priests, then Sulfuron" },
                    }
                }},
                { 12018, { 12018, "Majordomo Executus", "Molten Core",
                    {
                        { BOT_ROLE_NONE, "mage",   "+polymorph healer",   "Sheep healer adds" },
                        { BOT_ROLE_NONE, "warlock","+banish healer",      "Banish healer adds" },
                        { BOT_ROLE_NONE, "",       "+pause on damage shield", "Wait out Damage Shield" },
                        { BOT_ROLE_NONE, "",       "+pause on magic reflection", "Wait out Magic Reflection" },
                        { BOT_ROLE_NONE, "",       "+stop dps at 12 percent",  "Don't kill — talk for loot" },
                    }
                }},
                { 11502, { 11502, "Ragnaros", "Molten Core",
                    {
                        { BOT_ROLE_TANK,   "",       "+wall position",            "Tank back-to-wall (knockback)" },
                        { BOT_ROLE_NONE,   "",       "+spread 10y",                "Avoid Wrath chain damage" },
                        { BOT_ROLE_HEALER, "",       "+dispel elemental fire",    "Cleanse fire DoT stacks" },
                        { BOT_ROLE_NONE,   "",       "+sons of flame aoe mode",   "Auto-AoE during submerge" },
                    }
                }},

                // ====================================================
                // BLACKWING LAIR
                // ====================================================
                { 12435, { 12435, "Razorgore the Untamed", "Blackwing Lair",
                    {
                        { BOT_ROLE_TANK,   "",       "+pickup add waves",        "Phase 1: defend Orb controller" },
                        { BOT_ROLE_NONE,   "mage",   "+aoe drakonids",           "Phase 1: Blizzard adds" },
                        { BOT_ROLE_NONE,   "warlock","+aoe drakonids",           "Phase 1: Rain of Fire adds" },
                        { BOT_ROLE_HEALER, "priest", "+fear ward",               "Phase 2: Conflagration fear" },
                    }
                }},
                { 13020, { 13020, "Vaelastrasz the Corrupt", "Blackwing Lair",
                    {
                        { BOT_ROLE_NONE, "", "+burst dps",                  "Pure DPS race" },
                        { BOT_ROLE_NONE, "", "+burning adrenaline runaway", "Run out before explosion" },
                        { BOT_ROLE_TANK, "", "+tank rotation rage stack",   "Tanks swap; rage stacks lethal" },
                    }
                }},
                { 12017, { 12017, "Broodlord Lashlayer", "Blackwing Lair",
                    {
                        { BOT_ROLE_TANK,   "",       "+face away from raid", "Cleave + Mortal Strike" },
                        { BOT_ROLE_TANK,   "",       "+tank rotation 3 stacks", "Mortal Strike stacks → swap" },
                        { BOT_ROLE_HEALER, "",       "+mortal strike awareness", "Big tank-damage spikes" },
                        { BOT_ROLE_NONE,   "rogue",  "+disarm suppression",       "Phase 1 trash gauntlet" },
                    }
                }},
                { 11983, { 11983, "Firemaw", "Blackwing Lair",
                    {
                        { BOT_ROLE_TANK, "", "+wing buffet rotation", "Swap at 3 stacks" },
                        { BOT_ROLE_TANK, "", "+face away from raid",  "Shadow Flame cone" },
                        { BOT_ROLE_NONE, "", "+fire resist gear",      "Flame Buffet stacks" },
                    }
                }},
                { 14601, { 14601, "Ebonroc", "Blackwing Lair",
                    {
                        { BOT_ROLE_TANK, "", "+face away from raid",   "Shadow Flame cone" },
                        { BOT_ROLE_NONE, "", "+sustained dps",          "Self-heal counters burst" },
                        { BOT_ROLE_TANK, "", "+wing buffet rotation",   "Swap at 2-3 stacks" },
                    }
                }},
                { 11981, { 11981, "Flamegor", "Blackwing Lair",
                    {
                        { BOT_ROLE_NONE, "hunter", "+tranquilizing shot frenzy", "Critical Frenzy dispel" },
                        { BOT_ROLE_TANK, "",       "+face away from raid",        "Shadow Flame cone" },
                        { BOT_ROLE_TANK, "",       "+wing buffet rotation",      "Swap at 3 stacks" },
                    }
                }},
                { 14020, { 14020, "Chromaggus", "Blackwing Lair",
                    {
                        { BOT_ROLE_NONE,   "druid",   "+remove curse bronze affliction", "Bronze brood = curse" },
                        { BOT_ROLE_NONE,   "druid",   "+cleanse poison green affliction", "Green = poison" },
                        { BOT_ROLE_HEALER, "priest",  "+dispel magic blue red affliction", "Blue + Red = magic" },
                        { BOT_ROLE_HEALER, "priest",  "+cleanse disease black affliction", "Black = disease" },
                        { BOT_ROLE_HEALER, "paladin", "+cleanse all affliction",            "Paladin universal cleanse" },
                        { BOT_ROLE_NONE,   "hunter",  "+tranquilizing shot frenzy",         "Sub-15% Frenzy" },
                        { BOT_ROLE_TANK,   "",        "+face away from raid",              "Random breath cones" },
                    }
                }},
                { 11583, { 11583, "Nefarian", "Blackwing Lair",
                    {
                        { BOT_ROLE_TANK,   "",       "+pickup drakonid waves",        "Phase 1 add management" },
                        { BOT_ROLE_NONE,   "mage",   "+counterspell blue drakonid",   "Phase 1 priority interrupt" },
                        { BOT_ROLE_TANK,   "",       "+face away from raid",          "Phase 2 Shadow Flame" },
                        { BOT_ROLE_HEALER, "priest", "+fear ward",                     "Bellowing Roar fear" },
                        { BOT_ROLE_HEALER, "shaman", "+tremor totem",                  "Bellowing Roar fear" },
                        { BOT_ROLE_NONE,   "",       "+spread 8y",                     "Phase 2 cluster damage" },
                        { BOT_ROLE_NONE,   "",       "+class call awareness",          "Random class disable" },
                    }
                }},

                // ====================================================
                // ONYXIA'S LAIR
                // ====================================================
                { 10184, { 10184, "Onyxia", "Onyxia's Lair",
                    {
                        { BOT_ROLE_TANK,   "",       "+wall position back",      "P1: tank against back wall" },
                        { BOT_ROLE_NONE,   "",       "+avoid tail sweep",         "Never stand behind" },
                        { BOT_ROLE_NONE,   "",       "+spread 8y phase 2",        "Fireball cluster damage" },
                        { BOT_ROLE_NONE,   "",       "+deep breath flee",         "Run perpendicular on warning" },
                        { BOT_ROLE_HEALER, "priest", "+fear ward",                "P3: Bellowing Roar fear" },
                        { BOT_ROLE_HEALER, "shaman", "+tremor totem",             "P3: Bellowing Roar fear" },
                        { BOT_ROLE_TANK,   "",       "+pickup whelps p3",         "Off-tank role in P3" },
                        { BOT_ROLE_NONE,   "",       "+move out of lava cracks",  "P3: ground fire pulses" },
                    }
                }},
            };
            return kTable;
        }

        // Helper: lower-case the name for case-insensitive class matching.
        std::string ToLower(const std::string& s)
        {
            std::string out = s;
            for (size_t i = 0; i < out.size(); ++i)
                out[i] = (char)std::tolower((unsigned char)out[i]);
            return out;
        }
    }

    const RaidEncounter* RaidEncounterRegistry::Lookup(uint32 creatureId)
    {
        const auto& tab = GetTable();
        auto it = tab.find(creatureId);
        return (it != tab.end()) ? &it->second : nullptr;
    }

    std::string RaidEncounterRegistry::ResolveStrategies(const RaidEncounter* enc,
                                                          BotRoles role,
                                                          const std::string& className)
    {
        if (!enc) return "";
        std::string lowerClass = ToLower(className);

        std::string out;
        for (const auto& bundle : enc->bundles)
        {
            // Role match: BOT_ROLE_NONE means "applies to all roles"
            bool roleMatch = (bundle.role == BOT_ROLE_NONE) ||
                              ((bundle.role & role) != 0);
            if (!roleMatch) continue;

            // Class match: empty matches any class
            if (bundle.classMatch && bundle.classMatch[0] != '\0')
            {
                if (lowerClass != bundle.classMatch) continue;
            }

            // Append the strategies CSV
            if (bundle.strategies && bundle.strategies[0] != '\0')
            {
                if (!out.empty()) out += ",";
                out += bundle.strategies;
            }
        }
        return out;
    }

}}  // namespace ai::solocommander
