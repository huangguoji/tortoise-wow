#include "scriptPCH.h"
#include "stormwrought_ruins.h"

namespace
{
struct ThreatCopier : public ThreatListProcesser
{
    ThreatCopier(Creature* pSource, Creature* pDestination) : m_pSource(pSource), m_pDestination(pDestination) {}

    bool Process(Unit* pUnit) override
    {
        if (!pUnit || !pUnit->IsAlive())
            return false;

        float fThreat = m_pSource->GetThreatManager().getThreat(pUnit, true);
        if (fThreat <= 0.0f)
            fThreat = 1.0f;

        pUnit->SetInCombatWith(m_pDestination);
        m_pDestination->GetThreatManager().addThreatDirectly(pUnit, fThreat);
        return false;
    }

    Creature* m_pSource;
    Creature* m_pDestination;
};

void SetCreatureAttackable(Creature* pCreature, bool bAttackable)
{
    if (!pCreature)
        return;

    if (bAttackable)
    {
        pCreature->RemoveFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NOT_ATTACKABLE_1 | UNIT_FLAG_NON_ATTACKABLE_2);
        pCreature->SetReactState(REACT_AGGRESSIVE);
    }
    else
    {
        pCreature->SetFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NOT_ATTACKABLE_1 | UNIT_FLAG_NON_ATTACKABLE_2);
        pCreature->SetReactState(REACT_PASSIVE);
        pCreature->AttackStop();
        pCreature->ClearTarget();
    }
}
}

struct boss_ighalforAI : public ScriptedAI
{
    explicit boss_ighalforAI(Creature* pCreature) : ScriptedAI(pCreature)
    {
        m_pInstance = static_cast<ScriptedInstance*>(pCreature->GetInstanceData());
        Reset();
    }

    ScriptedInstance* m_pInstance;
    uint64 m_uiMergothidGUID;
    std::vector<ObjectGuid> m_vSummons;

    bool m_bPhaseTriggered;
    bool m_bChanneling;
    bool m_bImpsFiveSeconds;
    bool m_bImpsTwentySeconds;
    bool m_bImpsThirtyFiveSeconds;
    bool m_bGreatMergothidLine;
    bool m_bMergothidSummoned;
    bool m_bMergothidReleased;
    bool m_bTempleLine;

    uint32 m_uiDeathCoilTimer;
    uint32 m_uiFlamestrikeTimer;
    uint32 m_uiCurseOfAgonyTimer;
    uint32 m_uiChannelTimer;

    void Reset() override
    {
        m_uiMergothidGUID = 0;
        m_bPhaseTriggered = false;
        m_bChanneling = false;
        m_bImpsFiveSeconds = false;
        m_bImpsTwentySeconds = false;
        m_bImpsThirtyFiveSeconds = false;
        m_bGreatMergothidLine = false;
        m_bMergothidSummoned = false;
        m_bMergothidReleased = false;
        m_bTempleLine = false;
        m_uiDeathCoilTimer = 12000;
        m_uiFlamestrikeTimer = 7000;
        m_uiCurseOfAgonyTimer = 18000;
        m_uiChannelTimer = 0;

        StopChanneling();
        SetCreatureAttackable(m_creature, true);
        DespawnSummons();
    }

    void Aggro(Unit* /*pWho*/) override
    {
        DoScriptText(SAY_STORMWROUGHT_IGHALFOR_AGGRO, m_creature);
    }

    void MoveInLineOfSight(Unit* pWho) override
    {
        if (!m_bTempleLine && !m_creature->IsInCombat() && pWho && pWho->IsPlayer() && pWho->IsWithinDistInMap(m_creature, 80.0f) && pWho->IsWithinLOSInMap(m_creature))
        {
            m_bTempleLine = true;
            DoScriptText(SAY_STORMWROUGHT_IGHALFOR_TEMPLE, m_creature);
        }

        ScriptedAI::MoveInLineOfSight(pWho);
    }

    void AttackedBy(Unit* pAttacker) override
    {
        if (m_bPhaseTriggered && !m_bMergothidReleased)
            return;

        ScriptedAI::AttackedBy(pAttacker);
    }

    void AttackStart(Unit* pWho) override
    {
        if (m_bPhaseTriggered && !m_bMergothidReleased)
            return;

        ScriptedAI::AttackStart(pWho);
    }

    void JustDied(Unit* /*pKiller*/) override
    {
        DespawnSummons();
    }

    void EnterEvadeMode() override
    {
        StopChanneling();
        DespawnSummons();
        ScriptedAI::EnterEvadeMode();
    }

    void CastDeathCoil(uint32 uiDiff)
    {
        if (m_uiDeathCoilTimer <= uiDiff)
        {
            if (Unit* pTarget = m_creature->SelectAttackingTarget(ATTACKING_TARGET_RANDOM, 0, SPELL_DEATH_COIL, SELECT_FLAG_IN_LOS | SELECT_FLAG_NO_TOTEM))
            {
                if (DoCastSpellIfCan(pTarget, SPELL_DEATH_COIL) == CAST_OK)
                    m_uiDeathCoilTimer = 22000;
            }
        }
        else
            m_uiDeathCoilTimer -= uiDiff;
    }

    void CastFlamestrike(uint32 uiDiff)
    {
        if (m_uiFlamestrikeTimer <= uiDiff)
        {
            if (Unit* pTarget = m_creature->SelectAttackingTarget(ATTACKING_TARGET_RANDOM, 0, SPELL_FLAMESTRIKE, SELECT_FLAG_IN_LOS | SELECT_FLAG_NO_TOTEM))
            {
                if (DoCastSpellIfCan(pTarget, SPELL_FLAMESTRIKE) == CAST_OK)
                    m_uiFlamestrikeTimer = 24000;
            }
        }
        else
            m_uiFlamestrikeTimer -= uiDiff;
    }

    void CastCurseOfAgony(uint32 uiDiff)
    {
        if (m_uiCurseOfAgonyTimer <= uiDiff)
        {
            if (Unit* pTarget = m_creature->SelectAttackingTarget(ATTACKING_TARGET_RANDOM, 0, SPELL_CURSE_OF_AGONY, SELECT_FLAG_IN_LOS | SELECT_FLAG_NO_TOTEM))
            {
                if (DoCastSpellIfCan(pTarget, SPELL_CURSE_OF_AGONY) == CAST_OK)
                    m_uiCurseOfAgonyTimer = 26000;
            }
        }
        else
            m_uiCurseOfAgonyTimer -= uiDiff;
    }

    void StartPhaseTransition()
    {
        m_bPhaseTriggered = true;
        SetCreatureAttackable(m_creature, false);

        DoScriptText(SAY_STORMWROUGHT_IGHALFOR_CHANNEL, m_creature);

        m_creature->GetMotionMaster()->Clear();
        m_creature->AttackStop(true);
        m_creature->NearTeleportTo(m_creature->GetHomePosition());

        StartChanneling();
    }

    void StartChanneling()
    {
        m_bChanneling = true;
        m_uiChannelTimer = 0;
        m_creature->GetMotionMaster()->Clear();
        m_creature->AttackStop(true);
        m_creature->ClearTarget();
        m_creature->StopMoving();

        StartChannelingSpell();
    }

    void StartChannelingSpell()
    {
        if (GameObject* pAltar = GetNetherAltar())
        {
            m_creature->SetFacingToObject(pAltar);
        }

        m_creature->CastSpell(m_creature, SPELL_IGHALFOR_CHANNELING, true);
    }

    void StopChanneling()
    {
        m_creature->InterruptNonMeleeSpells(false);
        m_creature->RemoveAurasDueToSpell(SPELL_IGHALFOR_CHANNELING);
    }

    GameObject* GetNetherAltar()
    {
        if (!m_pInstance)
            return nullptr;

        return m_pInstance->instance->GetGameObject(m_pInstance->GetData64(DATA_NETHER_ALTAR_GUID));
    }

    void SpawnImpWave()
    {
        GameObject* pAltar = GetNetherAltar();
        if (!pAltar)
            return;

        static const float afOffset[3][2] =
        {
            { 1.5f, 0.0f },
            { -0.75f, 1.3f },
            { -0.75f, -1.3f }
        };

        for (uint8 i = 0; i < 3; ++i)
        {
            if (Creature* pImp = m_creature->SummonCreature(NPC_NETHER_IMP, pAltar->GetPositionX() + afOffset[i][0], pAltar->GetPositionY() + afOffset[i][1], pAltar->GetPositionZ(), pAltar->GetOrientation(), TEMPSUMMON_TIMED_OR_CORPSE_DESPAWN, 2 * MINUTE * IN_MILLISECONDS))
            {
                CopyThreatTo(pImp);
                m_vSummons.push_back(pImp->GetObjectGuid());
            }
        }
    }

    void SpawnMergothid()
    {
        GameObject* pAltar = GetNetherAltar();
        if (!pAltar)
            return;

        if (Creature* pMergothid = m_creature->GetMap()->SummonCreature(NPC_MERGOTHID, pAltar->GetPositionX(), pAltar->GetPositionY(), pAltar->GetPositionZ(), pAltar->GetOrientation(), TEMPSUMMON_DEAD_DESPAWN))
        {
            pMergothid->SetWorldMask(m_creature->GetWorldMask());
            SetCreatureAttackable(pMergothid, false);
            m_uiMergothidGUID = pMergothid->GetGUID();
        }
    }

    void FinishChanneling()
    {
        m_bMergothidReleased = true;
        StopChanneling();

        SetCreatureAttackable(m_creature, true);

        if (Unit* pTopThreat = m_creature->SelectAttackingTarget(ATTACKING_TARGET_TOPAGGRO, 0))
            m_creature->AI()->AttackStart(pTopThreat);

        if (Creature* pMergothid = m_creature->GetMap()->GetCreature(ObjectGuid(m_uiMergothidGUID)))
        {
            DoScriptText(SAY_STORMWROUGHT_MERGOTHID_RELEASE, pMergothid);
            SetCreatureAttackable(pMergothid, true);
            CopyThreatTo(pMergothid);
        }
    }

    void CopyThreatTo(Creature* pTarget)
    {
        if (!pTarget)
            return;

        ThreatCopier copier(m_creature, pTarget);
        m_creature->ProcessThreatList(&copier);

        if (Unit* pTopThreat = m_creature->SelectAttackingTarget(ATTACKING_TARGET_TOPAGGRO, 0))
            pTarget->AI()->AttackStart(pTopThreat);
    }

    void DespawnSummons()
    {
        for (ObjectGuid const& guid : m_vSummons)
        {
            if (Creature* pCreature = m_creature->GetMap()->GetCreature(guid))
            {
                if (TemporarySummon* pSummon = dynamic_cast<TemporarySummon*>(pCreature))
                    pSummon->UnSummon();
            }
        }

        m_vSummons.clear();
    }

    void UpdateChannel(uint32 uiDiff)
    {
        m_uiChannelTimer += uiDiff;

        if (!m_bImpsFiveSeconds && m_uiChannelTimer >= 5 * IN_MILLISECONDS)
        {
            m_bImpsFiveSeconds = true;
            SpawnImpWave();
        }

        if (!m_bImpsTwentySeconds && m_uiChannelTimer >= 20 * IN_MILLISECONDS)
        {
            m_bImpsTwentySeconds = true;
            SpawnImpWave();
        }

        if (!m_bImpsThirtyFiveSeconds && m_uiChannelTimer >= 35 * IN_MILLISECONDS)
        {
            m_bImpsThirtyFiveSeconds = true;
            SpawnImpWave();
        }

        if (!m_bGreatMergothidLine && m_uiChannelTimer >= 38 * IN_MILLISECONDS)
        {
            m_bGreatMergothidLine = true;
            DoScriptText(SAY_STORMWROUGHT_IGHALFOR_MERGOTHID, m_creature);
        }

        if (!m_bMergothidSummoned && m_uiChannelTimer >= 40 * IN_MILLISECONDS)
        {
            m_bMergothidSummoned = true;
            SpawnMergothid();
        }

        if (!m_bMergothidReleased && m_uiChannelTimer >= 48 * IN_MILLISECONDS)
            FinishChanneling();
    }

    void UpdateAI(uint32 uiDiff) override
    {
        if (m_bChanneling && !m_bMergothidReleased)
        {
            UpdateChannel(uiDiff);
            return;
        }

        if (!m_creature->SelectHostileTarget() || !m_creature->GetVictim())
            return;

        if (!m_bPhaseTriggered && m_creature->HealthBelowPct(25))
        {
            StartPhaseTransition();
            return;
        }

        if (!m_bPhaseTriggered || m_bMergothidReleased)
        {
            CastDeathCoil(uiDiff);
            CastFlamestrike(uiDiff);
            CastCurseOfAgony(uiDiff);
            DoMeleeAttackIfReady();
        }
    }
};

CreatureAI* GetAI_boss_ighalfor(Creature* pCreature)
{
    return new boss_ighalforAI(pCreature);
}

struct boss_mergothidAI : public ScriptedAI
{
    explicit boss_mergothidAI(Creature* pCreature) : ScriptedAI(pCreature)
    {
        Reset();
    }

    bool m_bMoreSouls;

    void Reset() override
    {
        m_bMoreSouls = false;
    }

    void JustDied(Unit* /*pKiller*/) override
    {
        DoScriptText(SAY_STORMWROUGHT_MERGOTHID_DEATH, m_creature);
    }

    void UpdateAI(uint32 /*uiDiff*/) override
    {
        if (!m_bMoreSouls && m_creature->HealthBelowPct(50))
        {
            m_bMoreSouls = true;
            DoScriptText(SAY_STORMWROUGHT_MERGOTHID_HALF, m_creature);
        }

        if (!m_creature->SelectHostileTarget() || !m_creature->GetVictim())
            return;

        DoMeleeAttackIfReady();
    }
};

CreatureAI* GetAI_boss_mergothid(Creature* pCreature)
{
    return new boss_mergothidAI(pCreature);
}

void AddSC_boss_ighalfor()
{
    Script* pNewscript;

    pNewscript = new Script;
    pNewscript->Name = "boss_ighalfor";
    pNewscript->GetAI = &GetAI_boss_ighalfor;
    pNewscript->RegisterSelf();

    pNewscript = new Script;
    pNewscript->Name = "boss_mergothid";
    pNewscript->GetAI = &GetAI_boss_mergothid;
    pNewscript->RegisterSelf();
}
