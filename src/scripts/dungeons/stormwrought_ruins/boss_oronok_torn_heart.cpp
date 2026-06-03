#include "scriptPCH.h"
#include "stormwrought_ruins.h"

struct boss_oronok_torn_heartAI : public ScriptedAI
{
    explicit boss_oronok_torn_heartAI(Creature* pCreature) : ScriptedAI(pCreature)
    {
        Reset();
    }

    uint32 m_uiKnockAwayTimer;
    uint32 m_uiDeathAndDecayTimer;
    uint32 m_uiShadowBoltTimer;

    void Reset() override
    {
        m_uiKnockAwayTimer = urand(8000, 12000);
        m_uiDeathAndDecayTimer = 1000;
        m_uiShadowBoltTimer = urand(5000, 8000);
    }

    void Aggro(Unit* /*pWho*/) override
    {
        DoScriptText(SAY_STORMWROUGHT_ORONOK_AGGRO, m_creature);
    }

    void JustDied(Unit* /*pKiller*/) override
    {
        DoScriptText(SAY_STORMWROUGHT_ORONOK_DEATH, m_creature);
    }

    void UpdateAI(uint32 uiDiff) override
    {
        if (!m_creature->SelectHostileTarget() || !m_creature->GetVictim())
            return;

        if (m_uiKnockAwayTimer <= uiDiff)
        {
            if (DoCastSpellIfCan(m_creature->GetVictim(), SPELL_KNOCK_AWAY) == CAST_OK)
                m_uiKnockAwayTimer = urand(16000, 22000);
        }
        else
            m_uiKnockAwayTimer -= uiDiff;

        if (m_uiDeathAndDecayTimer <= uiDiff)
        {
            if (DoCastSpellIfCan(m_creature->GetVictim(), SPELL_DEATH_AND_DECAY) == CAST_OK)
                m_uiDeathAndDecayTimer = urand(22000, 30000);
        }
        else
            m_uiDeathAndDecayTimer -= uiDiff;

        if (m_uiShadowBoltTimer <= uiDiff)
        {
            if (Unit* pTarget = m_creature->SelectAttackingTarget(ATTACKING_TARGET_RANDOM, 0, SPELL_SHADOW_BOLT, SELECT_FLAG_IN_LOS | SELECT_FLAG_NO_TOTEM))
            {
                if (DoCastSpellIfCan(pTarget, SPELL_SHADOW_BOLT) == CAST_OK)
                    m_uiShadowBoltTimer = urand(7000, 10000);
            }
        }
        else
            m_uiShadowBoltTimer -= uiDiff;

        DoMeleeAttackIfReady();
    }
};

CreatureAI* GetAI_boss_oronok_torn_heart(Creature* pCreature)
{
    return new boss_oronok_torn_heartAI(pCreature);
}

void AddSC_boss_oronok_torn_heart()
{
    Script* pNewscript;

    pNewscript = new Script;
    pNewscript->Name = "boss_oronok_torn_heart";
    pNewscript->GetAI = &GetAI_boss_oronok_torn_heart;
    pNewscript->RegisterSelf();
}
