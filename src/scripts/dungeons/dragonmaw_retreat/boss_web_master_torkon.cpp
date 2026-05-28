#include "scriptPCH.h"
#include "dragonmaw_retreat.h"

enum
{
    SPELL_VENOM_STING   = 5416,
    SPELL_POISON_CLOUD  = 3815,
};

struct boss_web_master_torkonAI : public ScriptedAI
{
    explicit boss_web_master_torkonAI(Creature* pCreature) : ScriptedAI(pCreature)
    {
        Reset();
    }

    uint32 m_uiVenomStingTimer;
    uint32 m_uiPoisonCloudTimer;

    void Reset() override
    {
        m_uiVenomStingTimer = urand(500, 1500);
        m_uiPoisonCloudTimer = urand(3 * IN_MILLISECONDS, 5 * IN_MILLISECONDS);
    }

    void Aggro(Unit* pWho) override
    {
        SummonCreeper(pWho, 2.5f);
        SummonCreeper(pWho, -2.5f);
    }

    void SummonCreeper(Unit* pTarget, float fSideOffset)
    {
        float fX = m_creature->GetPositionX() + cos(m_creature->GetOrientation() + M_PI_F / 2.0f) * fSideOffset;
        float fY = m_creature->GetPositionY() + sin(m_creature->GetOrientation() + M_PI_F / 2.0f) * fSideOffset;
        float fZ = m_creature->GetPositionZ();

        if (Creature* pCreeper = m_creature->SummonCreature(NPC_CAVERNWEB_CREEPER, fX, fY, fZ, m_creature->GetOrientation(), TEMPSUMMON_TIMED_OR_DEAD_DESPAWN, 2 * MINUTE * IN_MILLISECONDS))
        {
            pCreeper->SetInCombatWithZone();
            if (pTarget)
                pCreeper->AI()->AttackStart(pTarget);
        }
    }

    Unit* SelectRandomHostileTarget(uint32 uiSpellId)
    {
        if (Unit* pTarget = m_creature->SelectAttackingTarget(ATTACKING_TARGET_RANDOM, 0, uiSpellId, SELECT_FLAG_IN_LOS | SELECT_FLAG_NO_TOTEM))
            return pTarget;

        return m_creature->GetVictim();
    }

    void UpdateAI(const uint32 uiDiff) override
    {
        if (!m_creature->SelectHostileTarget() || !m_creature->GetVictim())
            return;

        if (m_uiVenomStingTimer < uiDiff)
        {
            if (DoCastSpellIfCan(m_creature->GetVictim(), SPELL_VENOM_STING) == CAST_OK)
                m_uiVenomStingTimer = urand(10 * IN_MILLISECONDS, 14 * IN_MILLISECONDS);
        }
        else
            m_uiVenomStingTimer -= uiDiff;

        if (m_uiPoisonCloudTimer < uiDiff)
        {
            if (Unit* pTarget = SelectRandomHostileTarget(SPELL_POISON_CLOUD))
            {
                if (DoCastSpellIfCan(pTarget, SPELL_POISON_CLOUD) == CAST_OK)
                    m_uiPoisonCloudTimer = urand(18 * IN_MILLISECONDS, 22 * IN_MILLISECONDS);
            }
        }
        else
            m_uiPoisonCloudTimer -= uiDiff;

        DoMeleeAttackIfReady();
    }
};

CreatureAI* GetAI_boss_web_master_torkon(Creature* pCreature)
{
    return new boss_web_master_torkonAI(pCreature);
}

void AddSC_boss_web_master_torkon()
{
    Script* pNewscript = new Script;
    pNewscript->Name = "boss_web_master_torkon";
    pNewscript->GetAI = &GetAI_boss_web_master_torkon;
    pNewscript->RegisterSelf();
}
