// cmangos/playerbots compat stub — Penqle has no WorldState.h.
// cmangos uses WorldState for cross-server-tick state tracking (rolling
// events, weekly resets, etc.). Penqle stores equivalent state in World.cpp
// directly + GameEventMgr.
// Phase 3: PlayerbotAIConfig.cpp + RandomPlayerbotMgr.cpp pull this header
// unconditionally. Identify what specific WorldState symbols they reference,
// then either (a) port the matching API to a Penqle adapter, or (b) gate the
// using code with #ifdef CMANGOS_WORLDSTATE that we don't define.
#pragma once
