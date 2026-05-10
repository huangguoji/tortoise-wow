#pragma once

// BotDiagnostics — opt-in observability for the playerbots subsystem.
//
// Provides two macros used liberally throughout the bot module + selected
// host hooks:
//
//   SC_LOG(fmt, ...)
//     [BOT]-prefixed structured log line emitted via sLog.outDetail. Filter
//     with `grep "\[BOT\]" info.log` to see all bot-related debug output.
//     Compile-time presence is unconditional; runtime emission is gated on
//     AiPlayerbot.EnableActionLog (default off). When the flag is off, the
//     macro expands to a single conditional that the optimizer collapses
//     to ~zero cost.
//
//   SC_PHASE(tag, botName)
//     Stamps the current "what is this bot AI thread doing" tag into a
//     thread-local. The crash handler in mangosd's Master.cpp reads this
//     on unhandled exception to report which phase the crashing thread
//     was in. ZERO normal-time log noise (no I/O) — only ever read on
//     crash. Also gated on the EnableActionLog flag so the TLS write
//     doesn't fire when diagnostics are off.
//
// Naming note: the SC_LOG / SC_PHASE names are kept for source-history
// continuity with the original Sprint 12 (sc-overnight) instrumentation.
// The runtime prefix is "[BOT]" — the macros are generic, not tied to the
// SoloCommander solo-play feature they were originally born alongside.

#include "Log.h"

namespace ai { namespace botdiag {
    // True iff AiPlayerbot.EnableActionLog is set in aiplayerbot.conf.
    // Implemented in BotDiagnostics.cpp to avoid an include cycle on
    // PlayerbotAIConfig.h.
    bool IsActionLogEnabled();

    // Thread-local last-phase tags. The crash handler reads these from
    // any thread that crashes. __declspec(thread) is MSVC's TLS spec;
    // matches the C++11 thread_local intent.
    extern __declspec(thread) const char* gLastPhaseTag;
    extern __declspec(thread) const char* gLastPhaseBotName;
}}

#define SC_LOG(fmt, ...) do { \
    if (ai::botdiag::IsActionLogEnabled()) \
        sLog.outDetail("[BOT] " fmt, ##__VA_ARGS__); \
} while (0)

#define SC_PHASE(tag, botName) do { \
    if (ai::botdiag::IsActionLogEnabled()) { \
        ai::botdiag::gLastPhaseTag     = (tag); \
        ai::botdiag::gLastPhaseBotName = (botName); \
    } \
} while (0)
