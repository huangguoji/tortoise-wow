#pragma once
//
// SoloCommander — Server-side support for the SoloCommander addon.
// Phase 1 of the addon implementation (sprint12). See runtime/ADDON_DESIGN.md.
//
// Sits on top of cmangos/playerbots' existing addon-message infrastructure
// (PlayerbotAI::HandleRemoteCommand). Listens for the "sc:" sub-prefix on
// debug-channel addon messages from the bot's master and dispatches the
// SCBOT protocol vocabulary (heartbeat, role presets, strategy toggles,
// threshold tuning, emergency hold).
//
// Also emits proactive heartbeats from the bot's UpdateAI tick so the addon
// has live HP/MP/target/strategy/threat data without polling.
//
// Wire format (addon-msg payloads, all CHAT_MSG_ADDON / LANG_ADDON):
//
//   master -> bot          bot -> master
//   ----------------       ---------------------
//   "sc:hb"                "SCBOT\tHB HP=64;MP=78;TGT=Lucifron;STRAT=heal,dispel;THR=42"
//   "sc:setrole healer"    "SCBOT\tACK setrole healer"
//   "sc:setstrat +heal,-pull"  "SCBOT\tACK setstrat"
//   "sc:setthresh heal=80;flee=15"  "SCBOT\tACK setthresh"
//   "sc:holdall"           "SCBOT\tACK holdall"
//   "sc:debug"             "SCBOT\tDBG STACK=...|LASTACT=...|LASTFAIL=..."
//   "sc:version"           "SCBOT\tVER 1"
//
// Vanilla 1.12 client has no addon-prefix registration API, so the client-
// side Lua filters on the literal "SCBOT\t" string at the head of any
// CHAT_MSG_ADDON event. The "SCBOT" prefix lives only in our payloads,
// not in the protocol layer.
//
// The "sc:" prefix on inbound messages was chosen so the existing security
// gate in PlayerbotAI::HandleCommand still applies (commands route through
// HandleRemoteCommand, which is master-only).
//

#include "Common.h"
#include "Log.h"
#include <string>

class Player;
class PlayerbotAI;

// SC_LOG: greppable structured-log macro for the bot AI hot path. Goes
// through sLog.outDetail so it lands in the standard server log file.
// Filter with `grep "\[SC\]"` to see all bot-related debug output.
//
// Use it liberally — the bot lifecycle has many phases (login, master
// link, level sync, gear assignment, talent assignment, encounter
// detection, heartbeat, logout cleanup) and each one is a possible
// crash site. SC_LOG before each non-trivial operation gives us a
// last-line-before-crash readout for free.
#define SC_LOG(fmt, ...) sLog.outDetail("[SC] " fmt, ##__VA_ARGS__)

// SC_PHASE: stamp the current "what is this bot AI thread doing" tag
// into a thread-local buffer. The crash handler in Master.cpp reads
// this on unhandled exception and writes it into the crash log line so
// we know which phase the crashing thread was in. ZERO normal-time log
// noise — only fires on crash.
//
// Format: SC_PHASE("phase tag", botName) — e.g. SC_PHASE("UpdateAI.entry", bot->GetName())
//
// Use a string literal for the tag (we store a pointer, not a copy) and
// a const char* for the bot name (also stored as pointer — we count on
// the bot's name being a stable allocation for the duration of the
// tick, which it is for cmangos's Player::m_name).
namespace ai { namespace solocommander {
    // Thread-local last-phase string. The crash handler reads this
    // from any thread that crashes. `__declspec(thread)` is the MSVC
    // equivalent of C++11 `thread_local` and works in C++14 too.
    extern __declspec(thread) const char* gLastPhaseTag;
    extern __declspec(thread) const char* gLastPhaseBotName;
}}

#define SC_PHASE(tag, botName) do { \
    ai::solocommander::gLastPhaseTag = (tag); \
    ai::solocommander::gLastPhaseBotName = (botName); \
} while (0)

namespace ai
{
namespace solocommander
{
    // Bumped when the protocol vocabulary changes incompatibly.
    constexpr uint32 SCBOT_PROTOCOL_VERSION = 1;

    // Heartbeat cadence in milliseconds. In-combat is more responsive so the
    // addon's HP/threat indicators stay accurate during a pull; out-of-combat
    // we slow down to avoid filling the chat log between engagements.
    constexpr uint32 HEARTBEAT_CADENCE_MS_COMBAT     = 2000;
    constexpr uint32 HEARTBEAT_CADENCE_MS_NONCOMBAT  = 5000;

    // Outbound payload prefix. The addon-msg body always starts with this
    // literal so the client-side handler can filter without ambiguity.
    extern const char* const RESPONSE_PREFIX;        // "SCBOT\t"

    // Inbound command prefix used inside the cmangos "debug <cmd>" channel.
    // PlayerbotAI::HandleRemoteCommand strips "debug " before we see the rest;
    // we then check for this prefix to claim the message.
    extern const char* const COMMAND_PREFIX;          // "sc:"

    class Commander
    {
    public:
        // Returns true if `command` (already stripped of the "debug " prefix
        // by HandleRemoteCommand) starts with the SoloCommander sub-prefix.
        static bool ClaimsCommand(const std::string& command);

        // Dispatches a claimed command. Returns the response payload to send
        // back to the master as an addon-msg. The returned string already
        // includes the RESPONSE_PREFIX so the caller can hand it directly to
        // the existing BuildChatPacket / SendPacket pipeline.
        //
        // For unknown verbs, returns an "ERR unknown <verb>" response (also
        // prefixed). Never throws.
        static std::string HandleCommand(PlayerbotAI* ai, const std::string& command);

        // Called from PlayerbotAI::UpdateAI every tick. Internally throttles
        // to HEARTBEAT_CADENCE_MS_COMBAT / _NONCOMBAT depending on bot state.
        // Cheap when not due (just an integer compare + add).
        static void TickHeartbeat(PlayerbotAI* ai, uint32 elapsedMs);

        // Hook for Phase 1c (level/gear sync on bot summon). Called from
        // PlayerbotHolder::OnBotLogin after the bot's session is established.
        // Currently a forward declaration; the implementation lands with 1c.
        static void OnBotSummoned(PlayerbotAI* ai);

        // Raid-encounter awareness: detects when the bot's target is a
        // known raid boss (per RaidEncounterRegistry) and applies role+
        // class-appropriate strategies. Removes them on target change.
        // Called every UpdateAI tick from PlayerbotAI::UpdateAI.
        static void TickEncounter(PlayerbotAI* ai);

    private:
        // Build the heartbeat payload (without the "HB " verb / response
        // prefix). Reads HP%, MP%, current target name, active strategy
        // names, threat estimate vs primary target.
        static std::string BuildHeartbeatBody(PlayerbotAI* ai);

        // Full debug-state dump for the addon's Debug panel. Multi-line.
        static std::string BuildDebugStateBody(PlayerbotAI* ai);

        // Phase 1c body: pin bot level to master's, re-equip green-tier
        // gear, top up consumables, heal to full. Called from TickHeartbeat
        // (deferred from OnBotSummoned, see comment there for why).
        static void DoLevelSync(PlayerbotAI* ai);

        // Apply a role preset (tank / otank / healer / melee / caster /
        // cc / support). Maps each preset to a fixed list of +/- strategy
        // tokens and applies them via the existing strategy engines.
        // Returns true on success.
        static bool ApplyRolePreset(PlayerbotAI* ai, const std::string& preset);

        // Parse a comma-separated list of "+name" / "-name" tokens and apply
        // each via the bot's strategy engines (combat / non-combat / dead).
        // Returns the number of tokens successfully applied.
        static uint32 ApplyStrategyTokens(PlayerbotAI* ai, const std::string& csv);

        // Parse a semicolon-separated KV list ("heal=80;drink=30;flee=15")
        // and update the bot's runtime threshold values. Returns count
        // applied.
        static uint32 ApplyThresholds(PlayerbotAI* ai, const std::string& kvList);

        // Send `payload` (which must already include RESPONSE_PREFIX) as a
        // CHAT_MSG_ADDON whisper to the bot's master.
        static void SendToMaster(PlayerbotAI* ai, const std::string& payload);
    };

}}  // namespace ai::solocommander
