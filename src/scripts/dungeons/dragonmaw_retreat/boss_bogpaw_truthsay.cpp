#include "scriptPCH.h"
#include "dragonmaw_retreat.h"

enum
{
    SPELL_ENTANGLING_ROOTS  = 22415,
    SPELL_LIGHTNING_CLOUD   = 19513,
    SPELL_HEALING_WAVE      = 10395,
};

struct boss_bogpaw_truthsayAI : public ScriptedAI
{
    explicit boss_bogpaw_truthsayAI(Creature* pCreature) : ScriptedAI(pCreature)
    {
        Reset();
    }

    uint32 m_uiEntanglingRootsTimer;
    uint32 m_uiLightningCloudTimer;
    uint32 m_uiHealingWaveTimer;

    void Reset() override
    {
        m_uiEntanglingRootsTimer = urand(5 * IN_MILLISECONDS, 7 * IN_MILLISECONDS);
        m_uiLightningCloudTimer = urand(2 * IN_MILLISECONDS, 4 * IN_MILLISECONDS);
        m_uiHealingWaveTimer = urand(6 * IN_MILLISECONDS, 8 * IN_MILLISECONDS);
    }

    Unit* SelectRandomHostileTarget(uint32 uiSpellId)
    {
        if (Unit* pTarget = m_creature->SelectAttackingTarget(ATTACKING_TARGET_RANDOM, 0, uiSpellId, SELECT_FLAG_IN_LOS | SELECT_FLAG_NO_TOTEM))
            return pTarget;

        return m_creature->GetVictim();
    }

    Unit* SelectHealingTarget()
    {
        Unit* pTarget = nullptr;

        if (Creature* pGowlfang = m_creature->FindNearestCreature(NPC_GOWLFANG, 80.0f, true))
        {
            if (pGowlfang->GetHealthPercent() <= 50.0f)
                pTarget = pGowlfang;
        }

        if (m_creature->GetHealthPercent() <= 50.0f && (!pTarget || m_creature->GetHealthPercent() < pTarget->GetHealthPercent()))
            pTarget = m_creature;

        return pTarget;
    }

    void UpdateAI(const uint32 uiDiff) override
    {
        if (!m_creature->SelectHostileTarget() || !m_creature->GetVictim())
            return;

        if (m_uiHealingWaveTimer < uiDiff)
        {
            if (Unit* pTarget = SelectHealingTarget())
            {
                if (DoCastSpellIfCan(pTarget, SPELL_HEALING_WAVE) == CAST_OK)
                    m_uiHealingWaveTimer = urand(8 * IN_MILLISECONDS, 11 * IN_MILLISECONDS);
            }
            else
                m_uiHealingWaveTimer = urand(2 * IN_MILLISECONDS, 3 * IN_MILLISECONDS);
        }
        else
            m_uiHealingWaveTimer -= uiDiff;

        if (m_uiLightningCloudTimer < uiDiff)
        {
            if (Unit* pTarget = SelectRandomHostileTarget(SPELL_LIGHTNING_CLOUD))
            {
                if (DoCastSpellIfCan(pTarget, SPELL_LIGHTNING_CLOUD) == CAST_OK)
                    m_uiLightningCloudTimer = urand(18 * IN_MILLISECONDS, 24 * IN_MILLISECONDS);
            }
        }
        else
            m_uiLightningCloudTimer -= uiDiff;

        if (m_uiEntanglingRootsTimer < uiDiff)
        {
            if (Unit* pTarget = SelectRandomHostileTarget(SPELL_ENTANGLING_ROOTS))
            {
                if (DoCastSpellIfCan(pTarget, SPELL_ENTANGLING_ROOTS) == CAST_OK)
                    m_uiEntanglingRootsTimer = urand(12 * IN_MILLISECONDS, 15 * IN_MILLISECONDS);
            }
        }
        else
            m_uiEntanglingRootsTimer -= uiDiff;

        DoMeleeAttackIfReady();
    }
};

CreatureAI* GetAI_boss_bogpaw_truthsay(Creature* pCreature)
{
    return new boss_bogpaw_truthsayAI(pCreature);
}

void AddSC_boss_bogpaw_truthsay()
{
    Script* pNewscript = new Script;
    pNewscript->Name = "boss_bogpaw_truthsay";
    pNewscript->GetAI = &GetAI_boss_bogpaw_truthsay;
    pNewscript->RegisterSelf();
}
