#include "scriptPCH.h"
#include "stormwrought_ruins.h"

struct boss_deathlord_tidebaneAI : public ScriptedAI
{
    explicit boss_deathlord_tidebaneAI(Creature* pCreature) : ScriptedAI(pCreature)
    {
        Reset();
    }

    uint32 m_uiRainOfFireTimer;
    uint32 m_uiDarkPlagueTimer;
    uint32 m_uiWailOfSoulsTimer;

    void Reset() override
    {
        m_uiRainOfFireTimer = urand(9000, 13000);
        m_uiDarkPlagueTimer = urand(4000, 7000);
        m_uiWailOfSoulsTimer = urand(16000, 22000);
    }

    void Aggro(Unit* /*pWho*/) override
    {
        DoScriptText(SAY_STORMWROUGHT_TIDEBANE_AGGRO, m_creature);
    }

    void JustDied(Unit* /*pKiller*/) override
    {
        DoScriptText(SAY_STORMWROUGHT_TIDEBANE_DEATH, m_creature);
    }

    Unit* SelectRandomTarget(uint32 uiSpellId)
    {
        return m_creature->SelectAttackingTarget(ATTACKING_TARGET_RANDOM, 0, uiSpellId, SELECT_FLAG_IN_LOS | SELECT_FLAG_NO_TOTEM);
    }

    void UpdateAI(uint32 uiDiff) override
    {
        if (!m_creature->SelectHostileTarget() || !m_creature->GetVictim())
            return;

        if (m_uiRainOfFireTimer <= uiDiff)
        {
            if (Unit* pTarget = SelectRandomTarget(SPELL_RAIN_OF_FIRE))
            {
                if (DoCastSpellIfCan(pTarget, SPELL_RAIN_OF_FIRE) == CAST_OK)
                    m_uiRainOfFireTimer = urand(18000, 24000);
            }
        }
        else
            m_uiRainOfFireTimer -= uiDiff;

        if (m_uiDarkPlagueTimer <= uiDiff)
        {
            if (Unit* pTarget = SelectRandomTarget(SPELL_DARK_PLAGUE))
            {
                if (DoCastSpellIfCan(pTarget, SPELL_DARK_PLAGUE) == CAST_OK)
                    m_uiDarkPlagueTimer = urand(10000, 14000);
            }
        }
        else
            m_uiDarkPlagueTimer -= uiDiff;

        if (m_uiWailOfSoulsTimer <= uiDiff)
        {
            if (DoCastSpellIfCan(m_creature, SPELL_WAIL_OF_SOULS) == CAST_OK)
            {
                DoScriptText(SAY_STORMWROUGHT_TIDEBANE_WAIL, m_creature);
                m_uiWailOfSoulsTimer = urand(22000, 30000);
            }
        }
        else
            m_uiWailOfSoulsTimer -= uiDiff;

        DoMeleeAttackIfReady();
    }
};

CreatureAI* GetAI_boss_deathlord_tidebane(Creature* pCreature)
{
    return new boss_deathlord_tidebaneAI(pCreature);
}

void AddSC_boss_deathlord_tidebane()
{
    Script* pNewscript;

    pNewscript = new Script;
    pNewscript->Name = "boss_deathlord_tidebane";
    pNewscript->GetAI = &GetAI_boss_deathlord_tidebane;
    pNewscript->RegisterSelf();
}
