#pragma once
//
// RaidEncounterRegistry — server-side data table mapping known raid-boss
// creature IDs to bot AI strategy bundles. Lets bots automatically adopt
// per-encounter behaviour (e.g. Hunter prepares Tranquilizing Shot the
// moment Magmadar is targeted) without hand-coded per-fight scripting.
//
// Mirrors runtime/raid-strategies/<boss>.md for the human-readable spec.
//
// Data source: Wowhead-derived guides in
//   E:\Wow Private\Class & raid data\Raids\
// Coverage: Molten Core (10 bosses), Blackwing Lair (8 bosses), Onyxia (1).
//
// Per-role strategy lists are deliberately conservative — they're the
// "bot AI MVP" for each encounter. More fight-specific behaviour
// (positioning, kite, swap-tank, etc.) lives in the .md docs and will
// require dedicated trigger / action classes that read this data.
//

#include "Common.h"
#include "ChatHelper.h"  // BotRoles
#include <string>
#include <unordered_map>
#include <vector>

namespace ai
{
namespace solocommander
{
    // A single per-role strategy bundle for one encounter. The strategies
    // string is the cmangos `+a,+b,-c` syntax that ChangeStrategy accepts.
    struct EncounterRoleStrategy
    {
        BotRoles role;             // BOT_ROLE_TANK / HEALER / DPS, or BOT_ROLE_NONE for "all roles"
        const char* classMatch;    // optional: lower-case class name to match (e.g. "hunter"), or "" for any
        const char* strategies;    // CSV "+heal,+dispel curse,..." applied via PlayerbotAI::ChangeStrategy
        const char* note;          // human-readable note for logs / debug
    };

    struct RaidEncounter
    {
        uint32 creatureId;
        const char* name;
        const char* raid;          // "Molten Core" / "Blackwing Lair" / "Onyxia's Lair"
        std::vector<EncounterRoleStrategy> bundles;
    };

    class RaidEncounterRegistry
    {
    public:
        // Look up a boss by creature ID. Returns nullptr if not a registered
        // raid boss.
        static const RaidEncounter* Lookup(uint32 creatureId);

        // Get the list of strategies that apply to a given (role, class) pair
        // for the given encounter. Returns empty string if no bundle matches.
        // The result is a CSV ready to hand to ChangeStrategy.
        static std::string ResolveStrategies(const RaidEncounter* enc,
                                              BotRoles role,
                                              const std::string& className);
    };

}}  // namespace ai::solocommander
