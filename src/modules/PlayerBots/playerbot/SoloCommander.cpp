//
// SoloCommander — Server-side support for the SoloCommander addon (Phase 1a).
//
// See SoloCommander.h for the protocol overview and runtime/ADDON_DESIGN.md
// for the broader UX design.
//

#include "playerbot/playerbot.h"
#include "SoloCommander.h"
#include "BotSpecRegistry.h"
#include "RaidEncounterRegistry.h"
#include "BotActionLog.h"
#include "PlayerbotAI.h"
#include "playerbot/PlayerbotFactory.h"
#include "playerbot/ServerFacade.h"
#include "playerbot/AiFactory.h"
#include "strategy/actions/ChangeTalentsAction.h"
#include "Chat/Chat.h"
#include "BotState.h"
#include "strategy/Engine.h"
#include "Objects/Creature.h"
#include "Objects/Player.h"
#include "ObjectMgr.h"

#include <sstream>

namespace ai
{
namespace solocommander
{
    const char* const RESPONSE_PREFIX = "SCBOT\t";
    const char* const COMMAND_PREFIX  = "sc:";

    // Thread-local "what was the last bot AI phase this thread was in?"
    // Set by SC_PHASE macro in the bot AI hot path; read by the crash
    // handler in Master.cpp on unhandled exception. See SoloCommander.h.
    __declspec(thread) const char* gLastPhaseTag = "(no phase set)";
    __declspec(thread) const char* gLastPhaseBotName = "(no bot)";

    // ---------------------------------------------------------------------
    // Small string helpers (kept local — playerbot's Helpers.h is heavyweight
    // and we don't need split() / trim() to be reusable across the module).
    // ---------------------------------------------------------------------
    namespace
    {
        std::string Trim(std::string s)
        {
            size_t start = s.find_first_not_of(" \t\n\r");
            if (start == std::string::npos) return "";
            size_t end = s.find_last_not_of(" \t\n\r");
            return s.substr(start, end - start + 1);
        }

        std::vector<std::string> SplitOn(const std::string& s, char sep)
        {
            std::vector<std::string> out;
            std::string current;
            for (char c : s)
            {
                if (c == sep) { out.push_back(Trim(current)); current.clear(); }
                else { current.push_back(c); }
            }
            if (!current.empty()) out.push_back(Trim(current));
            return out;
        }

        // Format payload with the response prefix in one place so we don't
        // forget it. Empty payloads are returned as just the prefix + "OK".
        std::string Resp(const std::string& body)
        {
            std::string out = RESPONSE_PREFIX;
            out += body.empty() ? std::string("OK") : body;
            return out;
        }

        // Best-effort percent calc that won't divide by zero.
        uint32 Pct(uint32 num, uint32 denom)
        {
            return denom > 0 ? (num * 100 / denom) : 0;
        }
    }

    // ---------------------------------------------------------------------
    // Role preset table.
    //
    // Each preset maps to a comma-separated list of "+strategy" / "-strategy"
    // tokens that PlayerbotAI::ChangeStrategy understands directly. The exact
    // strategy names are cmangos-defined (search strategy/<class>/*Strategy.cpp).
    //
    // These are deliberately conservative starting points — Phase 1a's goal
    // is to wire the dispatch path. Tuning the precise strategy mix per role
    // is empirical work that lands during smoke testing.
    // ---------------------------------------------------------------------
    namespace
    {
        struct RolePreset
        {
            const char* name;
            const char* tokens;
        };

        const RolePreset kRolePresets[] = {
            { "tank",    "+tank,+threat,-dps,-flee" },
            { "otank",   "+tank,+threat,-dps,-flee" },
            { "healer",  "+heal,+dispel,+cure,-dps,-aoe" },
            { "melee",   "+dps,+melee,+pull,-heal,-tank" },
            { "caster",  "+dps,+caster,-heal,-tank,-melee" },
            { "cc",      "+cc,-aoe" },
            { "support", "+buff,+heal,+dispel,-aoe" },
        };
        const size_t kRolePresetsCount = sizeof(kRolePresets) / sizeof(kRolePresets[0]);
    }

    // ---------------------------------------------------------------------
    // Public API
    // ---------------------------------------------------------------------

    bool Commander::ClaimsCommand(const std::string& command)
    {
        return command.rfind(COMMAND_PREFIX, 0) == 0;
    }

    std::string Commander::HandleCommand(PlayerbotAI* ai, const std::string& command)
    {
        if (!ai)
            return Resp("ERR no-ai");

        // Strip the "sc:" prefix.
        std::string body = command.substr(strlen(COMMAND_PREFIX));
        body = Trim(body);

        // Split verb from args at the first whitespace.
        std::string verb, args;
        size_t sp = body.find(' ');
        if (sp == std::string::npos) { verb = body; }
        else { verb = body.substr(0, sp); args = Trim(body.substr(sp + 1)); }

        if (verb == "version")
        {
            std::ostringstream out;
            out << "VER " << SCBOT_PROTOCOL_VERSION;
            return Resp(out.str());
        }

        if (verb == "hb")
        {
            return Resp("HB " + BuildHeartbeatBody(ai));
        }

        if (verb == "debug")
        {
            return Resp("DBG " + BuildDebugStateBody(ai));
        }

        if (verb == "setrole")
        {
            if (args.empty())
                return Resp("ERR setrole requires preset");
            return Resp(ApplyRolePreset(ai, args)
                ? std::string("ACK setrole ") + args
                : std::string("ERR unknown-preset ") + args);
        }

        if (verb == "setstrat")
        {
            if (args.empty())
                return Resp("ERR setstrat requires tokens");
            uint32 n = ApplyStrategyTokens(ai, args);
            std::ostringstream out; out << "ACK setstrat n=" << n;
            return Resp(out.str());
        }

        if (verb == "holdall")
        {
            // Emergency hold: stop pulling, stop attacking, drop combat
            // strategies. We layer a "-engage" token onto every state so the
            // bot's combat AI stops auto-acquiring targets, plus "+stay" so
            // movement halts. A subsequent "releaseall" restores the inverse.
            ApplyStrategyTokens(ai, "-engage,-pull,-aoe,-melee,+stay");
            return Resp("ACK holdall");
        }

        if (verb == "releaseall")
        {
            ApplyStrategyTokens(ai, "+engage,+pull,-stay");
            return Resp("ACK releaseall");
        }

        std::ostringstream out; out << "ERR unknown-verb " << verb;
        return Resp(out.str());
    }

    void Commander::TickHeartbeat(PlayerbotAI* ai, uint32 elapsedMs)
    {
        if (!ai) return;
        Player* bot = ai->GetBot();
        Player* master = ai->GetMaster();
        if (!bot || !master) return;
        if (master == bot) return;            // self-bots have nobody to notify

        // One-shot pending level/gear sync (Phase 1c). OnBotSummoned only
        // sets the flag — we run the actual sync from here, the first tick
        // when both bot and master are non-null. This deferral works around
        // a cmangos timing bug: at OnBotLogin (where my OnBotSummoned hook
        // runs), the bot's master link is not yet established for `.bot
        // create`-flow bots — SetMaster runs LATER in the same code path.
        // So if I synchronously read GetMaster() in OnBotSummoned, it
        // returns null and the sync skips. By the next UpdateAI tick, the
        // master link is set and we get past the early-exit above.
        if (ai->scboteLevelSyncPending)
        {
            SC_LOG("level sync pending->trigger bot=%s master=%s",
                   bot->GetName(), master->GetName());
            ai->scboteLevelSyncPending = false;
            DoLevelSync(ai);
            SC_LOG("level sync done bot=%s", bot->GetName());
        }

        // Auto-teleport: if scboteTeleportPending is set AND master is on
        // a different map/zone OR > 200 yards away, teleport the bot to
        // master's position. This makes `.bot add` a one-stop shop —
        // bot comes online and is immediately interactable. Sprint12
        // (sc-overnight) 2026-05-07.
        if (ai->scboteTeleportPending)
        {
            ai->scboteTeleportPending = false;
            const bool sameMap   = bot->GetMapId() == master->GetMapId();
            const bool sameInst  = bot->GetInstanceId() == master->GetInstanceId();
            float dx = bot->GetPositionX() - master->GetPositionX();
            float dy = bot->GetPositionY() - master->GetPositionY();
            float dist2d = sqrtf(dx * dx + dy * dy);
            if (!sameMap || !sameInst || dist2d > 200.0f)
            {
                // Snapshot pre-teleport state so we can correlate this with
                // a subsequent ghost-state event in the log: if TeleportTo
                // returns true but bot never lands (no UpdateSessions ACK
                // log line follows), we can prove the worldport schedule
                // was lost between MapManager and our HandleTeleportAck.
                SC_LOG("auto-teleport ENTRY bot=%s guid=%u -> master=%s "
                       "botMap=%u masterMap=%u sameInst=%d dist2d=%.1f botInWorld=%d",
                       bot->GetName(), bot->GetGUIDLow(), master->GetName(),
                       bot->GetMapId(), master->GetMapId(),
                       (int)sameInst, dist2d, (int)bot->IsInWorld());

                bool tpRet = bot->TeleportTo(master->GetMapId(),
                                             master->GetPositionX(),
                                             master->GetPositionY(),
                                             master->GetPositionZ(),
                                             master->GetOrientation());

                // After TeleportTo: for far teleport, the actual map removal
                // is deferred to MapManager::ExecuteScheduledFarTeleports
                // (runs after current map update completes). So at this
                // exact instant the bot is usually still in old map and
                // IsBeingTeleportedFar may be false. The ScheduleFarTeleport
                // queue is what gets it picked up. If tpRet=true but the
                // bot never enters teleport state, the schedule was rejected
                // (CanPlayerEnter / CanEnter returned false).
                SC_LOG("auto-teleport EXIT bot=%s tpRet=%d isBeingTeleportedFar=%d "
                       "isBeingTeleportedNear=%d isInWorld=%d mapId=%u",
                       bot->GetName(), (int)tpRet,
                       (int)bot->IsBeingTeleportedFar(),
                       (int)bot->IsBeingTeleportedNear(),
                       (int)bot->IsInWorld(),
                       bot->GetMapId());

                if (!tpRet)
                {
                    sLog.outError("[SC] auto-teleport FAILED bot=%s tpRet=false — "
                                  "MapManager::CanPlayerEnter rejected (instance lock? BG? invalid coord?). "
                                  "Bot will stay on its saved-position map; user must manually teleport or .bot remove.",
                                  bot->GetName());
                }
            }
            else
            {
                SC_LOG("auto-teleport skipped bot=%s — already with master (dist=%.1f)",
                       bot->GetName(), dist2d);
            }
        }

        // Heartbeat cadence — bumps the accumulator and emits when due.
        ai->scboteHeartbeatAcc += elapsedMs;

        bool inCombat = bot->IsInCombat();
        // Detect combat-state transitions and write a STATE snapshot when
        // they fire — those are the most informative moments for behavior
        // analysis (what HP/target/auras did the bot have when combat
        // started/ended?). scboteLastCombatLogged is sticky across ticks
        // but cheap.
        if (ai->scboteLastCombatLogged != (int)inCombat)
        {
            BotActionLog::LogState(ai, inCombat ? "combat-enter" : "combat-exit");
            ai->scboteLastCombatLogged = (int)inCombat;
        }

        uint32 cadence = inCombat ? HEARTBEAT_CADENCE_MS_COMBAT : HEARTBEAT_CADENCE_MS_NONCOMBAT;

        if (ai->scboteHeartbeatAcc < cadence)
            return;

        ai->scboteHeartbeatAcc = 0;
        SC_LOG("heartbeat send bot=%s master=%s combat=%d",
               bot->GetName(), master->GetName(), (int)inCombat);
        SendToMaster(ai, Resp("HB " + BuildHeartbeatBody(ai)));

        // Periodic state snapshot to the per-bot action log (every
        // heartbeat tick — 2s in combat, 5s out of combat).
        BotActionLog::LogState(ai, "heartbeat");
    }

    void Commander::OnBotSummoned(PlayerbotAI* ai)
    {
        // Phase 1c entry point. We don't do the level/gear work here
        // synchronously because at OnBotLogin time the master link is
        // typically not established yet. Instead, mark a one-shot pending
        // flag that TickHeartbeat consumes on the next tick when master
        // is guaranteed available.
        if (!ai) return;
        Player* bot = ai->GetBot();
        Player* master = ai ? ai->GetMaster() : nullptr;
        SC_LOG("OnBotSummoned bot=%s master=%s — marking level sync + teleport pending",
               bot ? bot->GetName() : "(null)",
               master ? master->GetName() : "(null)");
        ai->scboteLevelSyncPending = true;
        ai->scboteTeleportPending = true;

        // BotActionLog (sc-overnight 2026-05-05): open the per-bot debug
        // log file at logs/bots/<botname>_<...>.log. Subsequent triggers,
        // actions, casts, state changes, and combat events all tee into
        // this file so we can post-mortem a single bot's behavior without
        // grepping the main server log. Closed in PlayerbotMgr::LogoutPlayerBot.
        BotActionLog::Open(ai);
        BotActionLog::Write(ai, "LIFECYCLE",
                              "event=LOGIN bot=%s guid=%u level=%u race=%u class=%u "
                              "map=%u zone=%u master=%s",
                              bot ? bot->GetName() : "(null)",
                              bot ? bot->GetGUIDLow() : 0u,
                              bot ? bot->GetLevel() : 0u,
                              bot ? bot->getRace() : 0u,
                              bot ? bot->getClass() : 0u,
                              bot ? bot->GetMapId() : 0u,
                              bot ? bot->GetZoneId() : 0u,
                              master ? master->GetName() : "(none)");
        BotActionLog::LogState(ai, "post-summon");
    }

    void Commander::TickEncounter(PlayerbotAI* ai)
    {
        // Detects when the bot's current target is a known raid boss and
        // applies role+class-appropriate strategies. On target change to a
        // non-boss (or no target), the previously-applied strategies are
        // not actively removed by this method -- they're left in place for
        // the duration of the bot's current session.

        if (!ai) return;
        Player* bot = ai->GetBot();
        if (!bot) return;

        // Defensive: only proceed if bot is actually in the world. A bot
        // mid-login or mid-logout has an inconsistent state and dereffing
        // cached pointers can hit dangling memory.
        if (!bot->IsInWorld() || !bot->FindMap()) return;

        // Read the current target by GUID (safer than the cached Unit*),
        // then resolve to a Unit* via map lookup. This guarantees the
        // pointer is alive at the moment we use it.
        ObjectGuid targetGuid = bot->GetSelectionGuid();
        if (!targetGuid) return;

        // Only Creatures (NPCs) can be raid bosses; bail early on players.
        if (!targetGuid.IsCreature()) return;

        Unit* target = bot->GetMap()->GetUnit(targetGuid);
        if (!target) return;
        if (target->GetTypeId() != TYPEID_UNIT) return;
        Creature* creature = static_cast<Creature*>(target);
        uint32 entry = creature->GetEntry();

        // Already applied for this exact creature? Skip the work.
        if (ai->scboteLastEncounterCreatureId == entry)
            return;

        // Look up the encounter. Most targets aren't raid bosses; that's
        // the fast-path early return.
        const RaidEncounter* enc = RaidEncounterRegistry::Lookup(entry);
        if (!enc)
        {
            // Mark that we've checked this entry so we don't re-lookup
            // every tick on the same non-boss target. (Sentinel: entry
            // is non-zero, registry returned null.)
            SC_LOG("encounter check bot=%s target=%u — not in registry",
                   bot->GetName(), entry);
            ai->scboteLastEncounterCreatureId = entry;
            return;
        }
        SC_LOG("encounter MATCH bot=%s target=%u (%s)",
               bot->GetName(), entry, enc->name);

        // We found a registered raid boss AND it's a different one than
        // last tick. Resolve the strategy bundle for this bot's role+class
        // and apply it.
        BotRoles role = AiFactory::GetPlayerRoles(bot);
        std::string className = bot->getClass() == CLASS_WARRIOR ? "warrior" :
                                 bot->getClass() == CLASS_PALADIN ? "paladin" :
                                 bot->getClass() == CLASS_HUNTER  ? "hunter"  :
                                 bot->getClass() == CLASS_ROGUE   ? "rogue"   :
                                 bot->getClass() == CLASS_PRIEST  ? "priest"  :
                                 bot->getClass() == CLASS_SHAMAN  ? "shaman"  :
                                 bot->getClass() == CLASS_MAGE    ? "mage"    :
                                 bot->getClass() == CLASS_WARLOCK ? "warlock" :
                                 bot->getClass() == CLASS_DRUID   ? "druid"   : "";

        std::string strategies = RaidEncounterRegistry::ResolveStrategies(enc, role, className);
        if (!strategies.empty())
        {
            ai->ChangeStrategy(strategies, BotState::BOT_STATE_ALL);
            sLog.outDetail("[SoloCommander] %s engaging %s (%s): applied %s",
                            bot->GetName(), enc->name, enc->raid, strategies.c_str());
        }

        ai->scboteLastEncounterCreatureId = entry;
    }

    void Commander::DoLevelSync(PlayerbotAI* ai)
    {
        // Auto level + gear sync. Called from TickHeartbeat once master is
        // available (see comment in OnBotSummoned for the deferral rationale).
        //
        // Behavior:
        //   1. If bot's level already matches master's, do nothing. This
        //      preserves any gear the bot has looted across sessions --
        //      a "resummon" without level change should keep yesterday's
        //      upgrades.
        //   2. Otherwise pin level to master's, run cmangos's standard
        //      green-tier re-equip, top up consumables, heal to full.
        //
        // The user explicitly scoped Phase 1c to "level-appropriate and
        // class-appropriate greens" (leveling-dungeon focus, not raid),
        // so we use PlayerbotFactory::EquipGear() which calls
        // InitEquipment(false, false) -- the green-quality default path.
        // Raid-tier matching is deferred to a future phase.

        if (!ai) return;
        Player* bot = ai->GetBot();
        Player* master = ai->GetMaster();
        if (!bot || !master) return;
        if (master == bot) return;       // self-master pool bots: skip

        uint32 oldLevel    = bot->GetLevel();
        uint32 targetLevel = master->GetLevel();

        // No-op resummon: don't strip looted gear.
        if (oldLevel == targetLevel)
            return;

        // Set level. SetLevel handles base-stat / HP / mana / talent-point
        // recalculation. Reset XP so the bot sits cleanly at the new level
        // rather than being on a partial XP bar (which can flicker as the
        // master earns XP if XP scaling is on).
        SC_LOG("DoLevelSync bot=%s old=%u target=%u — setlevel+xp+stats",
               bot->GetName(), oldLevel, targetLevel);
        bot->SetLevel(targetLevel);
        bot->SetUInt32Value(PLAYER_XP, 0);
        bot->SetUInt32Value(PLAYER_NEXT_LEVEL_XP, sObjectMgr.GetXPForLevel(targetLevel));
        bot->InitTalentForLevel();
        bot->UpdateAllStats();
        SC_LOG("DoLevelSync bot=%s — setlevel done, entering spec assignment", bot->GetName());

        // Spec assignment (kBotSpecs registry).
        //
        // Look up the bot's name in BotSpecRegistry. If found, apply the
        // registered role; cmangos's ChangeTalentsAction::AutoSelectTalents
        // will then pick a premade-path talent build matching that role
        // (TANK / HEALER / DPS). If the bot isn't in the registry (e.g.,
        // a non-roster pool bot), we skip — cmangos's default randomisation
        // takes over via PlayerbotFactory::Randomize().
        //
        // We do this BEFORE EquipGear() because gear filtering can take the
        // bot's spec into account when picking weapons / armor priorities.
        const BotSpecEntry* specEntry = BotSpecRegistry::Lookup(bot->GetName());
        if (specEntry)
        {
            SC_LOG("DoLevelSync bot=%s — kBotSpecs hit: %s (role mask 0x%x), resetting talents",
                   bot->GetName(), specEntry->specLabel, (uint32)specEntry->role);
            bot->resetTalents(true);

            SC_LOG("DoLevelSync bot=%s — calling AutoSelectTalents", bot->GetName());
            std::ostringstream specLog;
            ChangeTalentsAction::AutoSelectTalents(bot, &specLog, specEntry->role);
            SC_LOG("DoLevelSync bot=%s spec'd to %s (role mask 0x%x)",
                   bot->GetName(), specEntry->specLabel, (uint32)specEntry->role);
        }
        else
        {
            SC_LOG("DoLevelSync bot=%s — no kBotSpecs entry, skipping spec assignment",
                   bot->GetName());
        }

        // Re-roll gear at the new level. Default itemQuality (0) means
        // EquipGear picks green-tier items; that's the level-appropriate
        // default the user asked for.
        SC_LOG("DoLevelSync bot=%s — entering EquipGear", bot->GetName());
        PlayerbotFactory factory(bot, targetLevel);
        factory.EquipGear();
        SC_LOG("DoLevelSync bot=%s — EquipGear done, starting consumables", bot->GetName());

        // Top up consumables: water, food, potions, oils, ammo, reagents.
        factory.AddConsumes();
        factory.AddFood();
        factory.AddPotions();
        SC_LOG("DoLevelSync bot=%s — consumables done, healing to full", bot->GetName());

        // Heal to full so the bot doesn't summon at a fraction of max HP
        // (the level-up itself updated max-HP but didn't necessarily
        // refill current HP).
        bot->SetHealth(bot->GetMaxHealth());
        Powers powerType = bot->GetPowerType();
        bot->SetPower(powerType, bot->GetMaxPower(powerType));

        // Persist immediately so a server bounce mid-session doesn't lose
        // the level adjustment.
        SC_LOG("DoLevelSync bot=%s — saving to DB", bot->GetName());
        bot->SaveToDB();

        // Tell the master what happened so they know the bot is ready.
        std::ostringstream msg;
        msg << "leveled to " << targetLevel << " and re-geared.";
        ai->TellPlayer(master, msg.str());
        SC_LOG("DoLevelSync bot=%s — DONE, master notified", bot->GetName());
    }

    // ---------------------------------------------------------------------
    // Internals
    // ---------------------------------------------------------------------

    std::string Commander::BuildHeartbeatBody(PlayerbotAI* ai)
    {
        Player* bot = ai->GetBot();
        std::ostringstream out;

        // HP / MP — always emitted, even at 0%, so the addon can detect
        // "dead at 0% HP" vs "alive at 1% HP" via the DEAD flag.
        uint32 hpPct = Pct(bot->GetHealth(), bot->GetMaxHealth());
        out << "HP=" << hpPct;

        uint32 mpMax = bot->GetMaxPower(bot->GetPowerType());
        uint32 mpCur = bot->GetPower(bot->GetPowerType());
        out << ";MP=" << Pct(mpCur, mpMax);

        // Target name (if any). Truncate to avoid blowing the 250-char addon
        // limit when combined with strategy CSV — boss names like "Onyxia"
        // are short but UI-scoped names from raids can be long.
        // Note: can't ternary the Value<Unit*> deref against nullptr — the
        // compiler can't reconcile Value<Unit*>& with nullptr_t. Use an
        // explicit branch instead.
        Unit* target = nullptr;
        if (AiObjectContext* ctx = ai->GetAiObjectContext())
            target = *ctx->GetValue<Unit*>("current target");
        if (target)
        {
            std::string name = target->GetName();
            if (name.size() > 24) name = name.substr(0, 24);
            out << ";TGT=" << name;
        }

        // Strategy list (CSV). Pulled from cmangos's existing introspection.
        // `currentEngine` reflects the active state's engine (combat/non/dead).
        // We use that rather than concatenating all three because the addon
        // only needs to show what's active *now*.
        Engine* engine = ai->GetCurrentEngine();
        if (engine)
        {
            std::string strats = engine->ListStrategies();
            // ListStrategies returns "Strategies: foo, bar, baz"; strip the
            // header so we don't blow the budget.
            const std::string header = "Strategies: ";
            if (strats.rfind(header, 0) == 0)
                strats = strats.substr(header.size());
            // Truncate aggressively to keep room for HP/MP/TGT.
            if (strats.size() > 120) strats = strats.substr(0, 120);
            out << ";STRAT=" << strats;
        }

        if (!sServerFacade.IsAlive(bot))
            out << ";DEAD=1";

        return out.str();
    }

    std::string Commander::BuildDebugStateBody(PlayerbotAI* ai)
    {
        std::ostringstream out;

        // Strategy stack — full CSV across all three engines, separated by
        // pipes. Addon parses STACK=<combat>|<noncombat>|<dead>.
        out << "STACK=";
        for (uint8 i = 0; i < (uint8)BotState::BOT_STATE_ALL; i++)
        {
            // No public engines[] accessor; the closest read-API is
            // PrintStrategies (writes to chat) or HasStrategy (queries one).
            // We can't enumerate without touching the array. For Phase 1a
            // we settle for the active-engine summary; full enumeration is
            // a tweak in Phase 3 once we add a public accessor.
            if (i > 0) out << "|";
            if (i == (uint8)ai->GetState() && ai->GetCurrentEngine())
            {
                std::string s = ai->GetCurrentEngine()->ListStrategies();
                const std::string header = "Strategies: ";
                if (s.rfind(header, 0) == 0) s = s.substr(header.size());
                if (s.size() > 60) s = s.substr(0, 60);
                out << s;
            }
        }

        // Last action — already exposed by cmangos.
        if (ai->GetCurrentEngine())
        {
            std::string last = ai->GetCurrentEngine()->GetLastAction();
            if (last.size() > 40) last = last.substr(0, 40);
            out << "|LASTACT=" << last;
        }

        // Bot state badge (combat / non-combat / dead).
        out << "|STATE=";
        switch (ai->GetState())
        {
            case BotState::BOT_STATE_COMBAT:     out << "combat"; break;
            case BotState::BOT_STATE_DEAD:       out << "dead"; break;
            case BotState::BOT_STATE_NON_COMBAT: out << "noncombat"; break;
            default:                              out << "unknown"; break;
        }

        return out.str();
    }

    bool Commander::ApplyRolePreset(PlayerbotAI* ai, const std::string& preset)
    {
        for (size_t i = 0; i < kRolePresetsCount; i++)
        {
            if (preset == kRolePresets[i].name)
            {
                ai->ChangeStrategy(kRolePresets[i].tokens, BotState::BOT_STATE_ALL);
                return true;
            }
        }
        return false;
    }

    uint32 Commander::ApplyStrategyTokens(PlayerbotAI* ai, const std::string& csv)
    {
        // ChangeStrategy already accepts a CSV; we just count tokens for the
        // ACK reply so the addon can show "n strategies applied".
        std::vector<std::string> tokens = SplitOn(csv, ',');
        if (tokens.empty()) return 0;
        ai->ChangeStrategy(csv, BotState::BOT_STATE_ALL);
        return (uint32)tokens.size();
    }

    uint32 Commander::ApplyThresholds(PlayerbotAI* /*ai*/, const std::string& /*kvList*/)
    {
        // Phase 3: needs per-bot AiObjectContext threshold storage; for now
        // we accept the verb at the API surface but no-op so the addon can
        // build against a stable contract.
        return 0;
    }

    void Commander::SendToMaster(PlayerbotAI* ai, const std::string& payload)
    {
        Player* bot = ai->GetBot();
        Player* master = ai->GetMaster();
        if (!bot || !master) return;

        // Mirror the existing pattern from PlayerbotAI::HandleCommand
        // (line ~1437 in PlayerbotAI.cpp) — addon-msg whisper from bot to
        // master with LANG_ADDON so the client treats it as opaque.
        WorldPacket data;
        ChatHandler::BuildChatPacket(data, CHAT_MSG_ADDON, payload.c_str(),
                LANG_ADDON, CHAT_TAG_NONE,
                bot->GetObjectGuid(), bot->GetName());
        sServerFacade.SendPacket(master, data);
    }

}}  // namespace ai::solocommander
