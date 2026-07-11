#include "scriptPCH.h"

enum NorthwindJousting
{
    NPC_JOUSTING_SPECTATOR    = 62452,
    NPC_JOUSTER_REGINALD      = 62691,
    NPC_JOUSTER_RUDOLF        = 62692,
    NPC_JOUSTER_ESTANOR       = 62693,
    NPC_JOUSTER_BLACK_KNIGHT  = 62694,
    NPC_JOUSTER_HALFAR        = 62695,
    NPC_JOUSTER_FERDINAND     = 62696,
    NPC_JOUSTER_BARNABAS      = 62702,
    NPC_JOUSTER_LUMIERE       = 62703,
    NPC_JOUSTER_FRITZ         = 62704,
    NPC_JOUSTER_LORENZ        = 62705,

    SPELL_JOUSTING_SLAM       = 42035,

    EVENT_INTERVAL_MS         = 90 * MINUTE * IN_MILLISECONDS,
    SUMMON_EVENT_LIFETIME     = 60 * IN_MILLISECONDS,
    CHARGE_START_TIME_MS      = 24500,
    CHARGE_DURATION_MS        = 4251,
    SLAM_LEAD_TIME_MS         = 500,
    HIT_PAUSE_MS              = 3 * IN_MILLISECONDS,

    GOSSIP_ACTION_START_RANDOM = GOSSIP_ACTION_INFO_DEF + 1,
    GOSSIP_ACTION_START_VARIANT = GOSSIP_ACTION_INFO_DEF + 100,

    GM_SECURITY_JOUSTING_TEST  = 5,

    YELL_DUKE_INTRO_LORENZ_FRITZ     = 6216210,
    YELL_DUKE_INTRO_LUMIERE_BARNABAS = 6216211,
    YELL_DUKE_INTRO_REGINALD_RUDOLF  = 6216212,
    YELL_DUKE_INTRO_BLACK_HALFAR     = 6216213,
    YELL_DUKE_INTRO_FERDINAND_ESTANOR = 6216214,
    YELL_DUKE_DETERMINE              = 6216215,
    YELL_DUKE_ONWARD                 = 6216216,
    YELL_DUKE_VICTORY_LORENZ         = 6216217,
    YELL_DUKE_VICTORY_LUMIERE        = 6216218,
    YELL_DUKE_VICTORY_REGINALD       = 6216219,
    YELL_DUKE_VICTORY_BLACK_KNIGHT   = 6216220,
    YELL_DUKE_VICTORY_FERDINAND      = 6216221,
    YELL_DUKE_GLORY_LORENZ           = 6216222,
    YELL_DUKE_GLORY_LUMIERE          = 6216223,
    YELL_DUKE_GLORY_REGINALD         = 6216224,
    YELL_DUKE_GLORY_BLACK_KNIGHT     = 6216225,
    YELL_DUKE_GLORY_FERDINAND        = 6216226,
    SAY_SPECTATOR_DUST              = 6269001,
    SAY_SPECTATOR_CHARGE            = 6269002,
    SAY_SPECTATOR_PRAISE            = 6269003
};

static uint32 const JOUSTING_SPECTATOR_GUID = 2589385;

struct JoustPosition
{
    float x;
    float y;
    float z;
    float o;
};

struct JoustParticipant
{
    JoustPosition spawn;
    JoustPosition ready;
    JoustPosition charge;
};

struct JoustingVariation
{
    char const* name;
    uint32 winnerEntry;
    uint32 defeatedEntry;
    uint32 dukeIntro;
    uint32 dukeDetermine;
    uint32 dukeOnward;
    uint32 dukeVictory;
    uint32 dukeGlory;
    uint32 spectatorDust;
    uint32 spectatorCharge;
    uint32 spectatorPraise;
};

static JoustParticipant const sWinnerPath =
{
    {-7554.149902f, 1074.199951f, 50.003101f, 5.452280f},
    {-7534.890137f, 1064.020020f, 49.945896f, 4.729720f},
    {-7534.830078f, 1030.020020f, 49.185257f, 4.729720f}
};

static JoustParticipant const sDefeatedPath =
{
    {-7519.589844f,  985.340027f, 47.424301f, 2.242360f},
    {-7540.109863f,  996.851990f, 48.820190f, 1.558290f},
    {-7540.229980f, 1029.890015f, 49.327801f, 1.558290f}
};

static JoustPosition const sWinnerVictory = {-7534.549805f, 996.187012f, 48.697422f, 4.729720f};
static JoustPosition const sWinnerExit = {-7519.589844f, 985.340027f, 48.402988f, 5.529270f};

static JoustingVariation const aJoustingVariations[] =
{
    {
        "Sir Lorenz of Theramore vs Fritz of Kul Tiras",
        NPC_JOUSTER_LORENZ,
        NPC_JOUSTER_FRITZ,
        YELL_DUKE_INTRO_LORENZ_FRITZ,
        YELL_DUKE_DETERMINE,
        YELL_DUKE_ONWARD,
        YELL_DUKE_VICTORY_LORENZ,
        YELL_DUKE_GLORY_LORENZ,
        SAY_SPECTATOR_DUST,
        SAY_SPECTATOR_CHARGE,
        SAY_SPECTATOR_PRAISE
    },
    {
        "Lumiere of Dalaran vs Sir Barnabas of Strom",
        NPC_JOUSTER_LUMIERE,
        NPC_JOUSTER_BARNABAS,
        YELL_DUKE_INTRO_LUMIERE_BARNABAS,
        YELL_DUKE_DETERMINE,
        YELL_DUKE_ONWARD,
        YELL_DUKE_VICTORY_LUMIERE,
        YELL_DUKE_GLORY_LUMIERE,
        SAY_SPECTATOR_DUST,
        SAY_SPECTATOR_CHARGE,
        SAY_SPECTATOR_PRAISE
    },
    {
        "Sir Reginald of Stormwind vs Rudolf of Strom",
        NPC_JOUSTER_REGINALD,
        NPC_JOUSTER_RUDOLF,
        YELL_DUKE_INTRO_REGINALD_RUDOLF,
        YELL_DUKE_DETERMINE,
        YELL_DUKE_ONWARD,
        YELL_DUKE_VICTORY_REGINALD,
        YELL_DUKE_GLORY_REGINALD,
        SAY_SPECTATOR_DUST,
        SAY_SPECTATOR_CHARGE,
        SAY_SPECTATOR_PRAISE
    },
    {
        "The Black Knight vs Halfar of Kul Tiras",
        NPC_JOUSTER_BLACK_KNIGHT,
        NPC_JOUSTER_HALFAR,
        YELL_DUKE_INTRO_BLACK_HALFAR,
        YELL_DUKE_DETERMINE,
        YELL_DUKE_ONWARD,
        YELL_DUKE_VICTORY_BLACK_KNIGHT,
        YELL_DUKE_GLORY_BLACK_KNIGHT,
        SAY_SPECTATOR_DUST,
        SAY_SPECTATOR_CHARGE,
        SAY_SPECTATOR_PRAISE
    },
    {
        "Ferdinand of Theramore vs Estanor of Dalaran",
        NPC_JOUSTER_FERDINAND,
        NPC_JOUSTER_ESTANOR,
        YELL_DUKE_INTRO_FERDINAND_ESTANOR,
        YELL_DUKE_DETERMINE,
        YELL_DUKE_ONWARD,
        YELL_DUKE_VICTORY_FERDINAND,
        YELL_DUKE_GLORY_FERDINAND,
        SAY_SPECTATOR_DUST,
        SAY_SPECTATOR_CHARGE,
        SAY_SPECTATOR_PRAISE
    }
};

struct npc_duke_sherwoodAI : public ScriptedAI
{
    npc_duke_sherwoodAI(Creature* pCreature) :
        ScriptedAI(pCreature),
        m_eventTimer(EVENT_INTERVAL_MS),
        m_elapsed(0),
        m_step(0),
        m_variationIndex(0),
        m_eventActive(false)
    {
        m_creature->SetActiveObjectState(true);
        // Debug GM gossip. Re-enable these with the gossip block if needed.
        // m_creature->SetFlag(UNIT_NPC_FLAGS, UNIT_NPC_FLAG_GOSSIP);
        Reset();
    }

    ObjectGuid m_winnerGuid;
    ObjectGuid m_defeatedGuid;
    uint32 m_eventTimer;
    uint32 m_elapsed;
    uint8 m_step;
    uint8 m_variationIndex;
    bool m_eventActive;

    void Reset() override
    {
        if (m_eventActive)
            CleanupSummons();

        m_winnerGuid.Clear();
        m_defeatedGuid.Clear();
        m_eventTimer = EVENT_INTERVAL_MS;
        m_elapsed = 0;
        m_step = 0;
        m_variationIndex = 0;
        m_eventActive = false;
    }

    void AttackStart(Unit* /*pWho*/) override {}

    void UpdateAI(uint32 const uiDiff) override
    {
        if (m_eventActive)
        {
            m_elapsed += uiDiff;
            while (m_eventActive && RunDueStep())
            {
            }
            return;
        }

        if (m_eventTimer > uiDiff)
        {
            m_eventTimer -= uiDiff;
            return;
        }

        StartRandomEvent();
    }

    bool IsEventActive() const { return m_eventActive; }

    char const* GetVariationName(uint8 index) const
    {
        return index < GetVariationCount() ? aJoustingVariations[index].name : "";
    }

    uint8 GetVariationCount() const
    {
        return uint8(sizeof(aJoustingVariations) / sizeof(aJoustingVariations[0]));
    }

    bool StartRandomEvent()
    {
        return StartEvent(uint8(urand(0, GetVariationCount() - 1)));
    }

    bool StartEvent(uint8 variationIndex)
    {
        if (m_eventActive || variationIndex >= GetVariationCount())
            return false;

        return StartEvent(aJoustingVariations[variationIndex], variationIndex);
    }

    bool RunDueStep()
    {
        static uint32 const stepTimes[] =
        {
            4500,
            16000,
            CHARGE_START_TIME_MS,
            CHARGE_START_TIME_MS + CHARGE_DURATION_MS - SLAM_LEAD_TIME_MS,
            CHARGE_START_TIME_MS + CHARGE_DURATION_MS,
            CHARGE_START_TIME_MS + CHARGE_DURATION_MS + HIT_PAUSE_MS,
            34000,
            36500,
            43000
        };

        if (m_step >= sizeof(stepTimes) / sizeof(stepTimes[0]) || m_elapsed < stepTimes[m_step])
            return false;

        RunStep(m_step++);
        return true;
    }

    bool StartEvent(JoustingVariation const& variation, uint8 variationIndex)
    {
        Creature* pWinner = m_creature->SummonCreature(variation.winnerEntry, sWinnerPath.spawn.x, sWinnerPath.spawn.y, sWinnerPath.spawn.z, sWinnerPath.spawn.o,
            TEMPSUMMON_MANUAL_DESPAWN, SUMMON_EVENT_LIFETIME, true);
        Creature* pDefeated = m_creature->SummonCreature(variation.defeatedEntry, sDefeatedPath.spawn.x, sDefeatedPath.spawn.y, sDefeatedPath.spawn.z, sDefeatedPath.spawn.o,
            TEMPSUMMON_MANUAL_DESPAWN, SUMMON_EVENT_LIFETIME, true);

        if (!pWinner || !pDefeated)
        {
            Despawn(pWinner);
            Despawn(pDefeated);
            m_eventTimer = 30 * IN_MILLISECONDS;
            return false;
        }

        MakePassiveAndInvulnerable(pWinner);
        MakePassiveAndInvulnerable(pDefeated);

        m_winnerGuid = pWinner->GetObjectGuid();
        m_defeatedGuid = pDefeated->GetObjectGuid();
        m_elapsed = 0;
        m_step = 0;
        m_variationIndex = variationIndex;
        m_eventActive = true;

        m_creature->MonsterYell(variation.dukeIntro);
        return true;
    }

    void RunStep(uint8 step)
    {
        Creature* pSpectator = GetSpectator();
        Creature* pWinner = GetWinner();
        Creature* pDefeated = GetDefeated();
        JoustingVariation const& variation = aJoustingVariations[m_variationIndex];

        switch (step)
        {
            case 0:
                MoveCreature(pWinner, sWinnerPath.ready);
                MoveCreature(pDefeated, sDefeatedPath.ready);
                if (pSpectator)
                    pSpectator->MonsterSay(variation.spectatorDust, 0, m_creature);
                break;
            case 1:
                m_creature->MonsterYell(variation.dukeDetermine);
                break;
            case 2:
                m_creature->MonsterYell(variation.dukeOnward);
                if (pSpectator)
                    pSpectator->MonsterSay(variation.spectatorCharge, 0, m_creature);
                MoveCreature(pWinner, sWinnerPath.charge);
                MoveCreature(pDefeated, sDefeatedPath.charge);
                break;
            case 3:
                if (pWinner && pDefeated)
                    pWinner->CastSpell(pDefeated, SPELL_JOUSTING_SLAM, true);
                break;
            case 4:
                if (pWinner && pDefeated && pDefeated->IsAlive())
                    pWinner->Kill(pDefeated, nullptr, false);
                break;
            case 5:
                m_creature->MonsterYell(variation.dukeVictory);
                MoveCreature(pWinner, sWinnerVictory);
                break;
            case 6:
                if (pSpectator)
                    pSpectator->MonsterSay(variation.spectatorPraise, 0, m_creature);
                break;
            case 7:
                m_creature->MonsterYell(variation.dukeGlory);
                MoveCreature(pWinner, sWinnerExit);
                break;
            case 8:
                FinishEvent();
                break;
        }
    }

    void FinishEvent()
    {
        CleanupSummons();

        m_winnerGuid.Clear();
        m_defeatedGuid.Clear();
        m_elapsed = 0;
        m_step = 0;
        m_eventActive = false;
        m_eventTimer = EVENT_INTERVAL_MS;
    }

    void MoveCreature(Creature* pCreature, JoustPosition const& position)
    {
        if (!pCreature)
            return;

        pCreature->SetWalk(false);
        pCreature->GetMotionMaster()->MovePoint(0, position.x, position.y, position.z,
            MOVE_FORCE_DESTINATION | MOVE_STRAIGHT_PATH | MOVE_RUN_MODE, 0.0f, position.o);
    }

    void MakePassiveAndInvulnerable(Creature* pCreature)
    {
        pCreature->SetReactState(REACT_PASSIVE);
        pCreature->SetFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_IMMUNE_TO_PLAYER | UNIT_FLAG_IMMUNE_TO_NPC | UNIT_FLAG_NON_ATTACKABLE_2 | UNIT_FLAG_PACIFIED);
    }

    Creature* GetSpectator()
    {
        return m_creature->GetMap()->GetCreature(ObjectGuid(HIGHGUID_UNIT, uint32(NPC_JOUSTING_SPECTATOR), JOUSTING_SPECTATOR_GUID));
    }

    Creature* GetWinner()
    {
        return m_winnerGuid ? m_creature->GetMap()->GetCreature(m_winnerGuid) : nullptr;
    }

    Creature* GetDefeated()
    {
        return m_defeatedGuid ? m_creature->GetMap()->GetCreature(m_defeatedGuid) : nullptr;
    }

    void CleanupSummons()
    {
        Despawn(GetWinner());
        Despawn(GetDefeated());
    }

    void Despawn(Creature* pCreature)
    {
        if (pCreature)
            pCreature->DespawnOrUnsummon();
    }
};

CreatureAI* GetAI_npc_duke_sherwood(Creature* pCreature)
{
    return new npc_duke_sherwoodAI(pCreature);
}

/*
 * Debug GM gossip for manually testing Northwind jousting variations.
 * To re-enable it, uncomment this block, restore UNIT_NPC_FLAG_GOSSIP in the AI constructor and restore the two pGossip assignments in AddSC_northwind().
 *
static bool HasJoustingTestAccess(Player* pPlayer)
{
    return pPlayer && pPlayer->GetSession() && uint32(pPlayer->GetSession()->GetSecurity()) >= GM_SECURITY_JOUSTING_TEST;
}

bool GossipHello_npc_duke_sherwood(Player* pPlayer, Creature* pCreature)
{
    if (!HasJoustingTestAccess(pPlayer))
        return false;

    npc_duke_sherwoodAI* pAI = dynamic_cast<npc_duke_sherwoodAI*>(pCreature->AI());
    if (!pAI)
        return false;

    if (pAI->IsEventActive())
        pPlayer->ADD_GOSSIP_ITEM(GOSSIP_ICON_CHAT, "Jousting event is already running.", GOSSIP_SENDER_MAIN, GOSSIP_ACTION_INFO_DEF);
    else
    {
        pPlayer->ADD_GOSSIP_ITEM(GOSSIP_ICON_CHAT, "Start random jousting variation.", GOSSIP_SENDER_MAIN, GOSSIP_ACTION_START_RANDOM);

        for (uint8 i = 0; i < pAI->GetVariationCount(); ++i)
        {
            std::string option = "Start joust: ";
            option += pAI->GetVariationName(i);
            pPlayer->ADD_GOSSIP_ITEM(GOSSIP_ICON_CHAT, option.c_str(), GOSSIP_SENDER_MAIN, GOSSIP_ACTION_START_VARIANT + i);
        }
    }

    pPlayer->SEND_GOSSIP_MENU(pPlayer->GetGossipTextId(pCreature), pCreature->GetGUID());
    return true;
}

bool GossipSelect_npc_duke_sherwood(Player* pPlayer, Creature* pCreature, uint32 uiSender, uint32 uiAction)
{
    (void)uiSender;

    if (!HasJoustingTestAccess(pPlayer))
        return false;

    npc_duke_sherwoodAI* pAI = dynamic_cast<npc_duke_sherwoodAI*>(pCreature->AI());
    if (!pAI)
        return false;

    if (uiAction == GOSSIP_ACTION_START_RANDOM)
        pAI->StartRandomEvent();
    else if (uiAction >= GOSSIP_ACTION_START_VARIANT)
        pAI->StartEvent(uint8(uiAction - GOSSIP_ACTION_START_VARIANT));

    pPlayer->CLOSE_GOSSIP_MENU();
    return true;
}
*/

void AddSC_northwind()
{
    Script* pNewScript = new Script;
    pNewScript->Name = "npc_duke_sherwood";
    pNewScript->GetAI = &GetAI_npc_duke_sherwood;
    // Debug GM gossip. Re-enable these with the gossip block if needed.
    // pNewScript->pGossipHello = &GossipHello_npc_duke_sherwood;
    // pNewScript->pGossipSelect = &GossipSelect_npc_duke_sherwood;
    pNewScript->RegisterSelf();
}
