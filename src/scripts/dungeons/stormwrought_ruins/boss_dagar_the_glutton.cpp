#include "scriptPCH.h"
#include "stormwrought_ruins.h"

struct boss_dagar_the_gluttonAI : public ScriptedAI
{
    explicit boss_dagar_the_gluttonAI(Creature* pCreature) : ScriptedAI(pCreature)
    {
        Reset();
    }

    uint32 m_uiConsumeFleshTimer;

    void Reset() override
    {
        m_uiConsumeFleshTimer = urand(6000, 8000);
    }

    void Aggro(Unit* /*pWho*/) override
    {
        DoScriptText(SAY_STORMWROUGHT_DAGAR_AGGRO, m_creature);
    }

    void JustDied(Unit* /*pKiller*/) override
    {
        DoScriptText(SAY_STORMWROUGHT_DAGAR_DEATH, m_creature);
    }

    void UpdateAI(uint32 uiDiff) override
    {
        if (!m_creature->SelectHostileTarget() || !m_creature->GetVictim())
            return;

        if (m_uiConsumeFleshTimer <= uiDiff)
        {
            if (DoCastSpellIfCan(m_creature->GetVictim(), SPELL_CONSUME_FLESH) == CAST_OK)
                m_uiConsumeFleshTimer = urand(12000, 16000);
        }
        else
            m_uiConsumeFleshTimer -= uiDiff;

        DoMeleeAttackIfReady();
    }
};

CreatureAI* GetAI_boss_dagar_the_glutton(Creature* pCreature)
{
    return new boss_dagar_the_gluttonAI(pCreature);
}

void AddSC_boss_dagar_the_glutton()
{
    Script* pNewscript;

    pNewscript = new Script;
    pNewscript->Name = "boss_dagar_the_glutton";
    pNewscript->GetAI = &GetAI_boss_dagar_the_glutton;
    pNewscript->RegisterSelf();
}
