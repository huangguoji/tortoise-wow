// cmangos/playerbots compat stub — Penqle has no SpellEffectDefines.h.
// cmangos's bot ahbot/TradeCategory.cpp pulls this inside #ifdef CMANGOS;
// Penqle keeps spell-effect names in SharedDefines.h/SpellMgr.h instead.
// Phase 3: if TradeCategory needs specific effect-id symbols, port them here
// (or rewrite TradeCategory.cpp to use Penqle's SharedDefines.h).
#pragma once
