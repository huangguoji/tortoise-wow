#pragma once
//
// BotSpecRegistry — server-side mapping from named bot characters to their
// intended role + spec, mirroring runtime/bot-builds/ROSTER.md.
//
// Used by SoloCommander::DoLevelSync to assign deterministic talents at
// summon time. Without this, cmangos's PlayerbotFactory would randomize
// talents, so a "Holy Paladin" bot could end up Retribution after a level
// sync. The registry pins the role (and where appropriate a spec hint)
// per bot name so the assignment is stable across summons.
//
// The role is mapped to one of cmangos's BotRoles values:
//   BOT_ROLE_TANK   = 0x01
//   BOT_ROLE_HEALER = 0x02
//   BOT_ROLE_DPS    = 0x04
// (See ChatHelper.h.)
//
// The spec hint is an optional case-sensitive substring that, if matched
// against a premade-path name in cmangos's TalentPath catalogue, narrows
// the selection. If the hint doesn't match any premade, we fall back to
// role-based random selection (same as cmangos's default behaviour).
//
// Adding a new named bot to ROSTER.md? Add a row here too. Build needed.
//

#include "Common.h"
#include "ChatHelper.h"
#include <string>
#include <unordered_map>

namespace ai
{
namespace solocommander
{
    struct BotSpecEntry
    {
        BotRoles role;             // Tank / Healer / DPS
        const char* specHint;      // optional cmangos premade-path filter, or "" for any-of-role
        const char* specLabel;     // human label for logs (e.g., "Holy", "Prot", "Shadow")
    };

    class BotSpecRegistry
    {
    public:
        // Look up a registered bot. Returns nullptr if not in the registry,
        // in which case callers should fall back to cmangos's default
        // role/spec auto-detection.
        static const BotSpecEntry* Lookup(const std::string& botName);
    };

}}  // namespace ai::solocommander
