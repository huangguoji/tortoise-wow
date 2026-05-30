#include "scriptPCH.h"
#include "dragonmaw_retreat.h"

enum
{
    SPELL_INTIMIDATING_ROAR = 8715,
};

struct boss_gowlfangAI : public ScriptedAI
{
    explicit boss_gowlfangAI(Creature* pCreature) : ScriptedAI(pCreature)
    {
        Reset();
    }

    bool m_bUsedIntimidatingRoar;

    void Reset() override
    {
        m_bUsedIntimidatingRoar = false;
    }

    void Aggro(Unit* /*pWho*/) override
    {
        DoScriptText(SAY_DRAGONMAW_GOWLFANG_AGGRO, m_creature);
    }

    void JustDied(Unit* /*pKiller*/) override
    {
        DoScriptText(SAY_DRAGONMAW_GOWLFANG_DEATH, m_creature);
    }

    void UpdateAI(const uint32 /*uiDiff*/) override
    {
        if (!m_creature->SelectHostileTarget() || !m_creature->GetVictim())
            return;

        if (!m_bUsedIntimidatingRoar && m_creature->GetHealthPercent() <= 50.0f)
        {
            if (DoCastSpellIfCan(m_creature, SPELL_INTIMIDATING_ROAR) == CAST_OK)
            {
                m_bUsedIntimidatingRoar = true;
                DoScriptText(SAY_DRAGONMAW_GOWLFANG_HALF_HEALTH, m_creature);
                DoResetThreat();
            }
        }

        DoMeleeAttackIfReady();
    }
};

CreatureAI* GetAI_boss_gowlfang(Creature* pCreature)
{
    return new boss_gowlfangAI(pCreature);
}

void AddSC_boss_gowlfang()
{
    Script* pNewscript = new Script;
    pNewscript->Name = "boss_gowlfang";
    pNewscript->GetAI = &GetAI_boss_gowlfang;
    pNewscript->RegisterSelf();
}
