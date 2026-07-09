#include "scriptPCH.h"
#include "DynamicObject.h"

enum
{
    NPC_IMAGE_OF_THANE          = 57643,

    SPELL_ARCANE_RUPTURE        = 52722,
    SPELL_ARCANE_BARRAGE        = 52723,
    SPELL_RUNE_OF_POWER_1       = 52724,
    SPELL_RUNE_OF_DETONATION    = 52725,
    SPELL_RUNE_OF_COMBUSTION    = 52726,
    SPELL_MIRROR_IMAGE          = 52727,
    SPELL_RUNE_OF_POWER_2       = 52729,

    SAY_AGGRO                   = 5764201,
    SAY_DEATH                   = 5764202,
    SAY_MIRROR_IMAGE            = 5764203,
    SAY_RUNE_OF_POWER           = 5764204,
    SAY_RUNE_OF_DETONATION      = 5764205,
    SAY_RUNE_OF_COMBUSTION      = 5764206,
    RAID_MSG_RUNE_OF_COMBUSTION = 5764207,
    RAID_MSG_RUNE_OF_DETONATION = 5764208,
};

struct thane_arcane_casterAI : public ScriptedAI
{
    thane_arcane_casterAI(Creature* pCreature) : ScriptedAI(pCreature) { }

    uint32 m_uiArcaneRuptureTimer;
    uint32 m_uiArcaneBarrageTimer;

    void ResetArcaneTimers()
    {
        m_uiArcaneRuptureTimer = 5000;
        m_uiArcaneBarrageTimer = 10000;
    }

    void UpdateArcaneAbilities(uint32 uiDiff)
    {
        if (m_uiArcaneRuptureTimer <= uiDiff)
        {
            if (DoCastSpellIfCan(m_creature->GetVictim(), SPELL_ARCANE_RUPTURE) == CAST_OK)
                m_uiArcaneRuptureTimer = 6000;
        }
        else
            m_uiArcaneRuptureTimer -= uiDiff;

        if (m_uiArcaneBarrageTimer <= uiDiff)
        {
            if (Unit* pTarget = m_creature->SelectAttackingTarget(ATTACKING_TARGET_RANDOM, 0, SPELL_ARCANE_BARRAGE))
            {
                if (DoCastSpellIfCan(pTarget, SPELL_ARCANE_BARRAGE) == CAST_OK)
                    m_uiArcaneBarrageTimer = 16000;
            }
        }
        else
            m_uiArcaneBarrageTimer -= uiDiff;
    }
};

struct boss_sorcerer_thane_thaurissanAI : public thane_arcane_casterAI
{
    boss_sorcerer_thane_thaurissanAI(Creature* pCreature) : thane_arcane_casterAI(pCreature)
    {
        Reset();
    }

    uint32 m_uiRaidRuneTimer;
    uint32 m_uiInitialRuneOfPowerTimer;
    uint32 m_uiRuneOfPowerRefreshTimer;
    uint8 m_uiRuneOfPowerStage;
    bool m_bNextRuneIsCombustion;
    bool m_bMirrorImageCast;
    ObjectGuid m_imageGuid;

    void Reset() override
    {
        if (Creature* pImage = m_creature->GetMap()->GetCreature(m_imageGuid))
            pImage->ForcedDespawn();

        m_creature->RemoveDynObject(SPELL_RUNE_OF_POWER_1);
        m_creature->RemoveDynObject(SPELL_RUNE_OF_POWER_2);
        m_imageGuid.Clear();
        ResetArcaneTimers();
        m_uiRaidRuneTimer = 20000;
        m_uiInitialRuneOfPowerTimer = 5000;
        m_uiRuneOfPowerRefreshTimer = 0;
        m_uiRuneOfPowerStage = 0;
        m_bNextRuneIsCombustion = false;
        m_bMirrorImageCast = false;
    }

    void RefreshRuneOfPower(uint32 uiSpellId)
    {
        for (uint8 uiEffect = EFFECT_INDEX_0; uiEffect < MAX_EFFECT_INDEX; ++uiEffect)
        {
            std::vector<DynamicObject*> dynamicObjects;
            m_creature->GetDynObjects(uiSpellId, SpellEffectIndex(uiEffect), dynamicObjects);
            for (DynamicObject* pDynamicObject : dynamicObjects)
                pDynamicObject->Delay(-2000);
        }
    }

    void CastRuneOfPower()
    {
        DoScriptText(SAY_RUNE_OF_POWER, m_creature);

        m_creature->RemoveDynObject(SPELL_RUNE_OF_POWER_1);
        m_creature->RemoveDynObject(SPELL_RUNE_OF_POWER_2);
        m_creature->CastSpell(m_creature, SPELL_RUNE_OF_POWER_1, true);
        m_creature->CastSpell(m_creature, SPELL_RUNE_OF_POWER_2, true);
        m_uiRuneOfPowerRefreshTimer = 7000;
    }

    void CastMirrorImage()
    {
        if (DoCastSpellIfCan(m_creature, SPELL_MIRROR_IMAGE) != CAST_OK)
            return;

        DoScriptText(SAY_MIRROR_IMAGE, m_creature);
        if (Creature* pImage = m_creature->SummonCreature(NPC_IMAGE_OF_THANE,
            m_creature->GetPositionX(), m_creature->GetPositionY(), m_creature->GetPositionZ(),
            m_creature->GetOrientation(), TEMPSUMMON_CORPSE_DESPAWN, 0))
        {
            m_imageGuid = pImage->GetObjectGuid();
        }

        m_bMirrorImageCast = true;
    }

    void Aggro(Unit* /*pWho*/) override
    {
        DoScriptText(SAY_AGGRO, m_creature);
    }

    void JustSummoned(Creature* pSummoned) override
    {
        if (pSummoned->GetEntry() != NPC_IMAGE_OF_THANE)
            return;

        pSummoned->SetInCombatWithZone();
        if (Unit* pTarget = m_creature->SelectAttackingTarget(ATTACKING_TARGET_RANDOM, 0))
            pSummoned->AI()->AttackStart(pTarget);
    }

    void JustDied(Unit* /*pKiller*/) override
    {
        if (Creature* pImage = m_creature->GetMap()->GetCreature(m_imageGuid))
            pImage->ForcedDespawn();

        m_imageGuid.Clear();
        m_creature->RemoveDynObject(SPELL_RUNE_OF_POWER_1);
        m_creature->RemoveDynObject(SPELL_RUNE_OF_POWER_2);
        DoScriptText(SAY_DEATH, m_creature);
    }

    void UpdateAI(const uint32 uiDiff) override
    {
        if (!m_creature->SelectHostileTarget() || !m_creature->GetVictim())
            return;

        if (m_uiRuneOfPowerRefreshTimer)
        {
            if (m_uiRuneOfPowerRefreshTimer <= uiDiff)
            {
                RefreshRuneOfPower(SPELL_RUNE_OF_POWER_1);
                RefreshRuneOfPower(SPELL_RUNE_OF_POWER_2);
                m_uiRuneOfPowerRefreshTimer = 2000;
            }
            else
                m_uiRuneOfPowerRefreshTimer -= uiDiff;
        }

        if (m_uiRuneOfPowerStage == 0)
        {
            if (m_uiInitialRuneOfPowerTimer <= uiDiff)
            {
                CastRuneOfPower();
                m_uiInitialRuneOfPowerTimer = 0;
                m_uiRuneOfPowerStage = 1;
            }
            else
                m_uiInitialRuneOfPowerTimer -= uiDiff;
        }
        else if (m_uiRuneOfPowerStage == 1 && m_creature->GetHealthPercent() <= 75.0f)
        {
            CastRuneOfPower();
            m_uiRuneOfPowerStage = 2;
        }
        else if (m_uiRuneOfPowerStage == 2 && m_creature->GetHealthPercent() <= 50.0f)
        {
            CastRuneOfPower();
            m_uiRuneOfPowerStage = 3;
        }
        else if (m_uiRuneOfPowerStage == 3 && m_creature->GetHealthPercent() <= 25.0f)
        {
            CastRuneOfPower();
            m_uiRuneOfPowerStage = 4;
        }

        if (!m_bMirrorImageCast && m_creature->GetHealthPercent() <= 50.0f)
            CastMirrorImage();

        if (m_uiRaidRuneTimer <= uiDiff)
        {
            uint32 uiSpell = m_bNextRuneIsCombustion ? SPELL_RUNE_OF_COMBUSTION : SPELL_RUNE_OF_DETONATION;
            if (DoCastSpellIfCan(m_creature, uiSpell) == CAST_OK)
            {
                if (m_bNextRuneIsCombustion)
                {
                    DoScriptText(SAY_RUNE_OF_COMBUSTION, m_creature);
                    DoScriptText(RAID_MSG_RUNE_OF_COMBUSTION, m_creature);
                }
                else
                {
                    DoScriptText(SAY_RUNE_OF_DETONATION, m_creature);
                    DoScriptText(RAID_MSG_RUNE_OF_DETONATION, m_creature);
                }

                m_bNextRuneIsCombustion = !m_bNextRuneIsCombustion;
                m_uiRaidRuneTimer = 20000;
            }
        }
        else
            m_uiRaidRuneTimer -= uiDiff;

        UpdateArcaneAbilities(uiDiff);
        DoMeleeAttackIfReady();
    }
};

struct npc_image_of_sorcerer_thaneAI : public thane_arcane_casterAI
{
    npc_image_of_sorcerer_thaneAI(Creature* pCreature) : thane_arcane_casterAI(pCreature)
    {
        Reset();
    }

    void Reset() override
    {
        ResetArcaneTimers();
    }

    void UpdateAI(const uint32 uiDiff) override
    {
        if (!m_creature->SelectHostileTarget() || !m_creature->GetVictim())
            return;

        UpdateArcaneAbilities(uiDiff);
        DoMeleeAttackIfReady();
    }
};

CreatureAI* GetAI_boss_sorcerer_thane_thaurissan(Creature* pCreature)
{
    return new boss_sorcerer_thane_thaurissanAI(pCreature);
}

CreatureAI* GetAI_npc_image_of_sorcerer_thane(Creature* pCreature)
{
    return new npc_image_of_sorcerer_thaneAI(pCreature);
}

void AddSC_boss_thane()
{
    Script* pNewScript = new Script;
    pNewScript->Name = "boss_sorcerer_thane_thaurissan";
    pNewScript->GetAI = &GetAI_boss_sorcerer_thane_thaurissan;
    pNewScript->RegisterSelf();

    pNewScript = new Script;
    pNewScript->Name = "npc_image_of_sorcerer_thane";
    pNewScript->GetAI = &GetAI_npc_image_of_sorcerer_thane;
    pNewScript->RegisterSelf();
}
