#pragma once
//
// BotActionLog — per-bot detailed action log.
//
// Sprint12 (sc-overnight) 2026-05-05. Built to support iterative bot-AI
// debugging: every bot we `.bot add` or `.rndbot add` opens its own log
// file at `logs/bots/<botname>_<sessionId>_<date>.log` and writes a
// timestamped event stream covering lifecycle, ticks, actions, spells,
// state changes, target changes, aura changes. The user can then grep
// a single bot's behavior post-session without untangling it from the
// main server log.
//
// Wire format: one event per line, `[timestamp] [TAG] key=val key=val`.
// Tags are uppercase fixed labels so `grep -E "\[CAST_"` etc. is easy.
//
// Hook points (where we call BotActionLog::* from):
//   - SoloCommander::OnBotSummoned       → Open + LIFECYCLE LOGIN
//   - PlayerbotMgr::LogoutPlayerBot      → LIFECYCLE LOGOUT + Close
//   - SoloCommander::TickHeartbeat       → periodic STATE snapshot
//   - Engine::LogAction                  → tee existing PUSH:/A: lines
//   - PlayerbotAI::CastSpell             → CAST_START
//   - PlayerbotAI::HandleCastFailed      → CAST_FAIL with reason
//   - Player::AddAura / RemoveAura       → AURA_APPLY / AURA_REMOVE
//                                          (filtered: skip <3s buffs +
//                                           combat-passive auras to keep
//                                           the file readable)
//
// Files are auto-flushed on every line so a crash doesn't lose the trail.
// Runtime cost: ~1-2 µs/line + the file-write itself; acceptable for
// debug-mode work, off in production via aiplayerbot.conf flag.
//

#include "Common.h"
#include <cstdio>
#include <unordered_map>

class Player;
class PlayerbotAI;

namespace ai { namespace solocommander {

class BotActionLog
{
public:
    // Open a log file for this bot. Idempotent (returns existing handle
    // if already open). Creates `logs/bots/` if missing. Returns null on
    // I/O error (caller must tolerate null and skip logging).
    static std::FILE* Open(PlayerbotAI* ai);

    // Close + flush + remove from the registry. Safe to call on a bot
    // that was never opened.
    static void Close(PlayerbotAI* ai);

    // Get the open handle for this bot, or null if not open. Cheap.
    static std::FILE* GetHandle(PlayerbotAI* ai);

    // Lower-level write: timestamped, tag-prefixed, fflush'd. Format
    // follows printf semantics. Safe to call when log isn't open
    // (no-op).
    static void Write(PlayerbotAI* ai, const char* tag, const char* fmt, ...);

    // Convenience: snapshot the bot's current state to the log. Called
    // periodically (every ~2s from TickHeartbeat) and on combat-state
    // change. Includes HP%, MP%, combat flag, target name+guid, active
    // strategy list, current spell-cast (if any), aura count.
    static void LogState(PlayerbotAI* ai, const char* reason);

    // Convenience: log a spell-cast attempt with its target.
    static void LogCastStart(PlayerbotAI* ai, uint32 spellId, ObjectGuid targetGuid, uint32 castTimeMs);

    // Convenience: log a spell-cast result. `result` is a SpellCastResult
    // enum value (0 = success, otherwise failure code from SpellMgr).
    // `phase` is "PREPARE" / "CAST" / "EFFECT" — where in the cast
    // pipeline the result fired.
    static void LogCastResult(PlayerbotAI* ai, uint32 spellId, uint8 result, const char* phase);

    // Convenience: log an aura apply/remove. We skip very-short auras
    // (< 3s) and movement-class auras to keep the log readable; pass
    // `force=true` to always log.
    static void LogAuraApply(PlayerbotAI* ai, uint32 spellId, int32 durationMs, ObjectGuid casterGuid, bool force = false);
    static void LogAuraRemove(PlayerbotAI* ai, uint32 spellId, ObjectGuid casterGuid, bool force = false);

    // Convenience: log a target change with optional reason.
    static void LogTargetChange(PlayerbotAI* ai, ObjectGuid oldTarget, ObjectGuid newTarget, const char* reason);

    // Close ALL open bot logs. Called from World shutdown to flush
    // cleanly on graceful stop.
    static void CloseAll();

private:
    // Map keyed by bot character GUID (low). Player* would be unsafe
    // across logout; the GUID stays valid in the map until we Close it.
    static std::unordered_map<uint32, std::FILE*> sFiles;

    // Helper: build the per-bot log path. `<botname>_<sessionId>_<date>.log`.
    static std::string BuildPath(Player* bot);

    // Helper: ensure `logs/bots/` exists. Cheap (CreateDirectory ignores
    // ERROR_ALREADY_EXISTS).
    static void EnsureLogDir();
};

}}  // namespace ai::solocommander
