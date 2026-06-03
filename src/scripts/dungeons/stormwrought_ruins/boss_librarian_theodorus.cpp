#include "scriptPCH.h"
#include "stormwrought_ruins.h"

struct boss_librarian_theodorusAI : public ScriptedAI
{
    explicit boss_librarian_theodorusAI(Creature* pCreature) : ScriptedAI(pCreature)
    {
        Reset();
    }

    uint32 m_uiConeOfColdTimer;
    uint32 m_uiFrostboltTimer;
    uint32 m_uiBlizzardTimer;
    bool m_bSummonedElemental;

    void Reset() override
    {
        m_uiConeOfColdTimer = 1000;
        m_uiFrostboltTimer = urand(4000, 6000);
        m_uiBlizzardTimer = urand(10000, 14000);
        m_bSummonedElemental = false;
    }

    void Aggro(Unit* /*pWho*/) override
    {
        DoScriptText(SAY_STORMWROUGHT_THEODORUS_AGGRO, m_creature);
    }

    void JustDied(Unit* /*pKiller*/) override
    {
        DoScriptText(SAY_STORMWROUGHT_THEODORUS_DEATH, m_creature);
    }

    void UpdateAI(uint32 uiDiff) override
    {
        if (!m_creature->SelectHostileTarget() || !m_creature->GetVictim())
            return;

        if (!m_bSummonedElemental && m_creature->GetHealthPercent() <= 50.0f)
        {
            if (DoCastSpellIfCan(m_creature, SPELL_SUMMON_THEODORUS_ELEMENTAL, CF_TRIGGERED) == CAST_OK)
            {
                DoScriptText(SAY_STORMWROUGHT_THEODORUS_ELEMENTAL, m_creature);
                m_bSummonedElemental = true;
            }
        }

        if (m_uiConeOfColdTimer <= uiDiff)
        {
            if (DoCastSpellIfCan(m_creature->GetVictim(), SPELL_CONE_OF_COLD) == CAST_OK)
                m_uiConeOfColdTimer = urand(15000, 22000);
        }
        else
            m_uiConeOfColdTimer -= uiDiff;

        if (m_uiFrostboltTimer <= uiDiff)
        {
            if (Unit* pTarget = m_creature->SelectAttackingTarget(ATTACKING_TARGET_RANDOM, 0, SPELL_FROSTBOLT, SELECT_FLAG_IN_LOS | SELECT_FLAG_NO_TOTEM))
            {
                if (DoCastSpellIfCan(pTarget, SPELL_FROSTBOLT) == CAST_OK)
                    m_uiFrostboltTimer = urand(6000, 9000);
            }
        }
        else
            m_uiFrostboltTimer -= uiDiff;

        if (m_uiBlizzardTimer <= uiDiff)
        {
            if (Unit* pTarget = m_creature->SelectAttackingTarget(ATTACKING_TARGET_RANDOM, 0, SPELL_THEODORUS_BLIZZARD, SELECT_FLAG_IN_LOS | SELECT_FLAG_NO_TOTEM))
            {
                if (DoCastSpellIfCan(pTarget, SPELL_THEODORUS_BLIZZARD) == CAST_OK)
                    m_uiBlizzardTimer = urand(16000, 22000);
            }
        }
        else
            m_uiBlizzardTimer -= uiDiff;

        DoMeleeAttackIfReady();
    }
};

CreatureAI* GetAI_boss_librarian_theodorus(Creature* pCreature)
{
    return new boss_librarian_theodorusAI(pCreature);
}

void AddSC_boss_librarian_theodorus()
{
    Script* pNewscript;

    pNewscript = new Script;
    pNewscript->Name = "boss_librarian_theodorus";
    pNewscript->GetAI = &GetAI_boss_librarian_theodorus;
    pNewscript->RegisterSelf();
}
