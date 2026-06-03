#include "scriptPCH.h"
#include "stormwrought_ruins.h"

struct boss_subjugator_halthas_shadecrestAI : public ScriptedAI
{
    explicit boss_subjugator_halthas_shadecrestAI(Creature* pCreature) : ScriptedAI(pCreature)
    {
        Reset();
    }

    uint32 m_uiDominateMindTimer;
    uint32 m_uiFlamestrikeTimer;
    uint32 m_uiFearTimer;

    void Reset() override
    {
        m_uiDominateMindTimer = urand(12000, 16000);
        m_uiFlamestrikeTimer = 0;
        m_uiFearTimer = urand(18000, 24000);
    }

    void Aggro(Unit* /*pWho*/) override
    {
        DoScriptText(SAY_STORMWROUGHT_HALTHAS_AGGRO, m_creature);
    }

    void JustDied(Unit* /*pKiller*/) override
    {
        DoScriptText(SAY_STORMWROUGHT_HALTHAS_DEATH, m_creature);
    }

    void UpdateAI(uint32 uiDiff) override
    {
        if (!m_creature->SelectHostileTarget() || !m_creature->GetVictim())
            return;

        if (m_uiDominateMindTimer <= uiDiff)
        {
            if (DoCastSpellIfCan(m_creature->GetVictim(), SPELL_DOMINATE_MIND) == CAST_OK)
            {
                DoScriptText(SAY_STORMWROUGHT_HALTHAS_DOMINATE, m_creature);
                m_uiDominateMindTimer = urand(26000, 34000);
            }
        }
        else
            m_uiDominateMindTimer -= uiDiff;

        if (m_uiFlamestrikeTimer <= uiDiff)
        {
            if (DoCastSpellIfCan(m_creature->GetVictim(), SPELL_HALTHAS_FLAMESTRIKE) == CAST_OK)
                m_uiFlamestrikeTimer = urand(12000, 16000);
        }
        else
            m_uiFlamestrikeTimer -= uiDiff;

        if (m_uiFearTimer <= uiDiff)
        {
            if (DoCastSpellIfCan(m_creature, SPELL_HALTHAS_FEAR) == CAST_OK)
                m_uiFearTimer = urand(24000, 32000);
        }
        else
            m_uiFearTimer -= uiDiff;

        DoMeleeAttackIfReady();
    }
};

CreatureAI* GetAI_boss_subjugator_halthas_shadecrest(Creature* pCreature)
{
    return new boss_subjugator_halthas_shadecrestAI(pCreature);
}

void AddSC_boss_subjugator_halthas_shadecrest()
{
    Script* pNewscript;

    pNewscript = new Script;
    pNewscript->Name = "boss_subjugator_halthas_shadecrest";
    pNewscript->GetAI = &GetAI_boss_subjugator_halthas_shadecrest;
    pNewscript->RegisterSelf();
}
