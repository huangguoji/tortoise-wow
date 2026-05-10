// cmangos/playerbots compat stub — Penqle has no LFGQueue class.
// Penqle's LFG flow is server-side via LFGMgr only; no separate queue class.
// Phase 3: 4 hook sites in cmangos's bot module reference LFGQueue. Decide:
//   (a) backport LFGQueue from cmangos (clean), OR
//   (b) stub LFGQueue::IsPlayerInQueue() with a no-op (lazy), OR
//   (c) strip the queue-aware bot strategies (loses some bot features).
// See bot-deployment-sprint-plan.md "Risks / unknowns" #2.
#pragma once
