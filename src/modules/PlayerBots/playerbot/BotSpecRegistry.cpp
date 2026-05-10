//
// BotSpecRegistry — see header for design notes.
//
// Mirrors runtime/bot-builds/ROSTER.md (Alliance 40 + Horde 39 = 79 bots).
// Names use the original ROSTER CamelCase form, but at lookup time we also
// try the lower-cased form because cmangos's name lookup normalises
// "BromStone" -> "Bromstone" via Util::normalizePlayerName. Both resolve
// to the same registry entry.
//

#include "BotSpecRegistry.h"
#include <cctype>

namespace ai
{
namespace solocommander
{
    namespace
    {
        // Helper: unordered_map keyed by name. Initialised lazily on first
        // Lookup so we don't pay the construction cost at startup unless
        // someone actually queries the registry.
        const std::unordered_map<std::string, BotSpecEntry>& GetTable()
        {
            static const std::unordered_map<std::string, BotSpecEntry> kTable = {
                // ---------- ALLIANCE (40) ----------

                // Humans
                { "AldricVane",    { BOT_ROLE_DPS,    "arms",        "Arms Warrior" } },
                { "LyraBright",    { BOT_ROLE_HEALER, "holy",        "Holy Paladin" } },         // *
                { "MarcasBlack",   { BOT_ROLE_DPS,    "beast",       "BM Hunter" } },
                { "EdwinHart",     { BOT_ROLE_DPS,    "combat",      "Combat Rogue" } },
                { "ElaraReed",     { BOT_ROLE_HEALER, "discipline",  "Disc Priest" } },
                { "OrinFrost",     { BOT_ROLE_DPS,    "frost",       "Frost Mage" } },
                { "VossHallow",    { BOT_ROLE_DPS,    "affliction",  "Affliction Lock" } },

                // Dwarves
                { "BromStone",     { BOT_ROLE_TANK,   "protection",  "Prot Warrior" } },         // *
                { "ThorinOak",     { BOT_ROLE_DPS,    "retribution", "Ret Paladin" } },
                { "DurikHawk",     { BOT_ROLE_DPS,    "marksman",    "MM Hunter" } },
                { "GrimBrokk",     { BOT_ROLE_DPS,    "subtlety",    "Sub Rogue" } },
                { "BalinHolt",     { BOT_ROLE_HEALER, "holy",        "Holy Priest" } },
                { "KhazorRune",    { BOT_ROLE_DPS,    "fire",        "Fire Mage" } },
                { "VrazCoal",      { BOT_ROLE_DPS,    "demonology",  "Demo Lock" } },

                // Night Elves
                { "SylvarMoon",    { BOT_ROLE_DPS,    "fury",        "Fury Warrior" } },
                { "FaeranWild",    { BOT_ROLE_DPS,    "beast",       "BM Hunter" } },
                { "ShaeShadow",    { BOT_ROLE_DPS,    "subtlety",    "Sub Rogue" } },
                { "VaeralaDusk",   { BOT_ROLE_DPS,    "shadow",      "Shadow Priest" } },        // *
                { "TalanarSky",    { BOT_ROLE_DPS,    "balance",     "Balance Druid" } },        // *

                // Gnomes
                { "FizzleBolt",    { BOT_ROLE_TANK,   "protection",  "Prot Warrior" } },
                { "PibbixWhisk",   { BOT_ROLE_DPS,    "marksman",    "MM Hunter" } },
                { "QuibixWick",    { BOT_ROLE_DPS,    "combat",      "Combat Rogue" } },
                { "QuirrikSpark",  { BOT_ROLE_DPS,    "fire",        "Fire Mage" } },
                { "BlixxNoxx",     { BOT_ROLE_DPS,    "demonology",  "Demo Lock" } },

                // High Elves
                { "QuelVandar",    { BOT_ROLE_DPS,    "arms",        "Arms Warrior" } },
                { "AelwynDawn",    { BOT_ROLE_HEALER, "holy",        "Holy Paladin" } },
                { "QuelronVane",   { BOT_ROLE_DPS,    "marksman",    "MM Hunter" } },
                { "SaelDusk",      { BOT_ROLE_DPS,    "combat",      "Combat Rogue" } },
                { "LyrielSong",    { BOT_ROLE_HEALER, "discipline",  "Disc Priest" } },
                { "ThaelinSun",    { BOT_ROLE_DPS,    "arcane",      "Arcane Mage" } },

                // Alliance raid fillers (10)
                { "EvianaLeaf",    { BOT_ROLE_HEALER, "restoration", "Resto Druid" } },
                { "DorenStone",    { BOT_ROLE_HEALER, "holy",        "Holy Priest" } },
                { "ThelminGold",   { BOT_ROLE_HEALER, "holy",        "Holy Paladin" } },
                { "KaelorThorn",   { BOT_ROLE_DPS,    "feral",       "Feral Druid" } },
                { "MaerynShade",   { BOT_ROLE_DPS,    "shadow",      "Shadow Priest" } },
                { "TelorWynn",     { BOT_ROLE_DPS,    "arcane",      "Arcane Mage" } },
                { "BizzleWhirl",   { BOT_ROLE_DPS,    "fury",        "Fury Warrior" } },
                { "GromirIron",    { BOT_ROLE_DPS,    "fury",        "Fury Warrior" } },
                { "MorvanShade",   { BOT_ROLE_DPS,    "destruction", "Destro Lock" } },
                { "VaerinSilver",  { BOT_ROLE_DPS,    "survival",    "Survival Hunter" } },

                // ---------- HORDE (39) ----------
                // (For completeness — Alliance is the play focus, but keeping
                // the Horde half registered keeps tooling consistent.)

                // Orcs
                { "GromGorefist",  { BOT_ROLE_TANK,   "protection",  "Prot Warrior" } },         // *
                { "ZurakBlood",    { BOT_ROLE_DPS,    "marksman",    "MM Hunter" } },
                { "GnashFang",     { BOT_ROLE_DPS,    "subtlety",    "Sub Rogue" } },
                { "DurakStorm",    { BOT_ROLE_DPS,    "enhancement", "Enh Shaman" } },
                { "VeknArcanus",   { BOT_ROLE_DPS,    "frost",       "Frost Mage" } },
                { "MorgKurat",     { BOT_ROLE_DPS,    "demonology",  "Demo Lock" } },

                // Undead
                { "VarmusGrim",    { BOT_ROLE_DPS,    "arms",        "Arms Warrior" } },
                { "DraknRot",      { BOT_ROLE_DPS,    "marksman",    "MM Hunter" } },
                { "SilasNyx",      { BOT_ROLE_DPS,    "subtlety",    "Sub Rogue" } },
                { "MorthrenAsh",   { BOT_ROLE_DPS,    "shadow",      "Shadow Priest" } },
                { "VortrenHex",    { BOT_ROLE_DPS,    "frost",       "Frost Mage" } },
                { "NerubGall",     { BOT_ROLE_DPS,    "affliction",  "Affliction Lock" } },

                // Tauren
                { "TalornStone",   { BOT_ROLE_TANK,   "protection",  "Prot Warrior" } },
                { "MoraSkywind",   { BOT_ROLE_DPS,    "beast",       "BM Hunter" } },
                { "OelaSong",      { BOT_ROLE_HEALER, "holy",        "Holy Priest" } },
                { "KornaThunder",  { BOT_ROLE_HEALER, "restoration", "Resto Shaman" } },         // *
                { "RuhokRain",     { BOT_ROLE_DPS,    "balance",     "Balance Druid" } },        // *

                // Trolls
                { "ZulHaktar",     { BOT_ROLE_DPS,    "arms",        "Arms Warrior" } },
                { "JoraTusk",      { BOT_ROLE_DPS,    "beast",       "BM Hunter" } },
                { "SenjinShade",   { BOT_ROLE_DPS,    "subtlety",    "Sub Rogue" } },
                { "VolJorin",      { BOT_ROLE_DPS,    "shadow",      "Shadow Priest" } },        // *
                { "RokahMoja",     { BOT_ROLE_DPS,    "enhancement", "Enh Shaman" } },
                { "VashaDjin",     { BOT_ROLE_DPS,    "frost",       "Frost Mage" } },
                { "ZakaroVoll",    { BOT_ROLE_DPS,    "affliction",  "Affliction Lock" } },

                // Goblins
                { "GribbleGash",   { BOT_ROLE_TANK,   "protection",  "Prot Warrior" } },
                { "ZippoSlick",    { BOT_ROLE_DPS,    "marksman",    "MM Hunter" } },
                { "FizzleBin",     { BOT_ROLE_DPS,    "combat",      "Combat Rogue" } },
                { "BobbleSpark",   { BOT_ROLE_DPS,    "fire",        "Fire Mage" } },
                { "VennoSludge",   { BOT_ROLE_DPS,    "destruction", "Destro Lock" } },

                // Horde raid fillers (10)
                { "AeolaSpring",   { BOT_ROLE_HEALER, "restoration", "Resto Druid" } },
                { "ElyraFrost",    { BOT_ROLE_HEALER, "discipline",  "Disc Priest" } },
                { "DraknStorm",    { BOT_ROLE_HEALER, "restoration", "Resto Shaman" } },
                { "DroknClaw",     { BOT_ROLE_DPS,    "feral",       "Feral Druid" } },
                { "VarmenAsh",     { BOT_ROLE_DPS,    "shadow",      "Shadow Priest" } },
                { "OkkerCold",     { BOT_ROLE_DPS,    "frost",       "Frost Mage" } },
                { "GarokRavage",   { BOT_ROLE_DPS,    "fury",        "Fury Warrior" } },
                { "MoshiBlade",    { BOT_ROLE_DPS,    "combat",      "Combat Rogue" } },
                { "MorgrenRot",    { BOT_ROLE_DPS,    "affliction",  "Affliction Lock" } },
                { "VarokWolf",     { BOT_ROLE_DPS,    "marksman",    "MM Hunter" } },
            };
            return kTable;
        }

        // Lower-case an ASCII string in-place. Used for the case-insensitive
        // fallback lookup since cmangos normalises names (e.g. "BromStone" ->
        // "Bromstone") before storing them in the player cache.
        std::string ToLower(const std::string& s)
        {
            std::string out = s;
            for (size_t i = 0; i < out.size(); ++i)
                out[i] = (char)std::tolower((unsigned char)out[i]);
            return out;
        }
    }

    const BotSpecEntry* BotSpecRegistry::Lookup(const std::string& botName)
    {
        const auto& tab = GetTable();

        // Fast path: exact case match (most common — the runtime usually
        // hands us names in their stored case).
        auto it = tab.find(botName);
        if (it != tab.end())
            return &it->second;

        // Slow path: case-insensitive scan. Build an on-demand lower-case
        // index the first time we hit this branch so subsequent lookups
        // are O(1). Static guarantees thread-safe init in C++11+.
        static const auto kLowerIndex = []() {
            std::unordered_map<std::string, const BotSpecEntry*> idx;
            for (const auto& e : GetTable())
                idx.emplace(ToLower(e.first), &e.second);
            return idx;
        }();

        auto lit = kLowerIndex.find(ToLower(botName));
        return (lit != kLowerIndex.end()) ? lit->second : nullptr;
    }

}}  // namespace ai::solocommander
