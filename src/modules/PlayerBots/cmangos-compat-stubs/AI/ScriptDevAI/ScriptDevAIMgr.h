// cmangos/playerbots compat stub — Penqle has no ScriptDevAIMgr.h.
// cmangos uses ScriptDevAIMgr as a registry/dispatcher for ScriptDev2 scripts.
// Penqle has its own ScriptMgr (src/game/ScriptMgr.h) that handles the same
// territory but with a different API surface.
// Phase 3: PlayerbotAI.cpp + GossipHelloAction.cpp pull this header
// unconditionally. Identify which ScriptDevAIMgr methods they call, then map
// to Penqle's ScriptMgr equivalents.
#pragma once
