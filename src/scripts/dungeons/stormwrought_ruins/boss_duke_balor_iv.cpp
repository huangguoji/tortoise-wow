#include "scriptPCH.h"
#include "stormwrought_ruins.h"

struct boss_duke_balor_ivAI : public ScriptedAI
{
    explicit boss_duke_balor_ivAI(Creature* pCreature) : ScriptedAI(pCreature)
    {
        Reset();
    }

    uint32 m_uiMindFlayTimer;
    uint32 m_uiPsychicScreamTimer;
    uint32 m_uiWailingDeadTimer;
    uint32 m_uiDarkStrikeTimer;

    void Reset() override
    {
        m_uiMindFlayTimer = 5000;
        m_uiPsychicScreamTimer = urand(15000, 20000);
        m_uiWailingDeadTimer = urand(10000, 15000);
        m_uiDarkStrikeTimer = urand(7000, 10000);
    }

    void Aggro(Unit* /*pWho*/) override
    {
        DoScriptText(SAY_STORMWROUGHT_DUKE_BALOR_AGGRO, m_creature);
    }

    void JustDied(Unit* /*pKiller*/) override
    {
        DoScriptText(SAY_STORMWROUGHT_DUKE_BALOR_DEATH, m_creature);
    }

    void UpdateAI(uint32 uiDiff) override
    {
        if (!m_creature->SelectHostileTarget() || !m_creature->GetVictim())
            return;

        if (m_uiMindFlayTimer <= uiDiff)
        {
            if (DoCastSpellIfCan(m_creature->GetVictim(), SPELL_MIND_FLAY) == CAST_OK)
                m_uiMindFlayTimer = urand(25000, 30000);
        }
        else
            m_uiMindFlayTimer -= uiDiff;

        if (m_uiPsychicScreamTimer <= uiDiff)
        {
            if (DoCastSpellIfCan(m_creature, SPELL_PSYCHIC_SCREAM) == CAST_OK)
                m_uiPsychicScreamTimer = urand(15000, 20000);
        }
        else
            m_uiPsychicScreamTimer -= uiDiff;

        if (m_uiWailingDeadTimer <= uiDiff)
        {
            if (DoCastSpellIfCan(m_creature, SPELL_WAILING_DEAD) == CAST_OK)
                m_uiWailingDeadTimer = urand(10000, 15000);
        }
        else
            m_uiWailingDeadTimer -= uiDiff;

        if (m_uiDarkStrikeTimer <= uiDiff)
        {
            if (DoCastSpellIfCan(m_creature->GetVictim(), SPELL_DARK_STRIKE) == CAST_OK)
                m_uiDarkStrikeTimer = urand(16000, 22000);
        }
        else
            m_uiDarkStrikeTimer -= uiDiff;

        DoMeleeAttackIfReady();
    }
};

CreatureAI* GetAI_boss_duke_balor_iv(Creature* pCreature)
{
    return new boss_duke_balor_ivAI(pCreature);
}

void AddSC_boss_duke_balor_iv()
{
    Script* pNewscript;

    pNewscript = new Script;
    pNewscript->Name = "boss_duke_balor_iv";
    pNewscript->GetAI = &GetAI_boss_duke_balor_iv;
    pNewscript->RegisterSelf();
}
