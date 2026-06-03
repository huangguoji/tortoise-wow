#include "scriptPCH.h"
#include "stormwrought_ruins.h"

struct boss_lady_drazareAI : public ScriptedAI
{
    explicit boss_lady_drazareAI(Creature* pCreature) : ScriptedAI(pCreature)
    {
        Reset();
    }

    uint32 m_uiDarkSeductionTimer;
    uint32 m_uiDrazareEmbraceAuraTimer;

    void Reset() override
    {
        m_uiDarkSeductionTimer = urand(19000, 20000);
        m_uiDrazareEmbraceAuraTimer = urand(12000, 24000);
    }

    void Aggro(Unit* /*pWho*/) override
    {
        DoScriptText(SAY_STORMWROUGHT_DRAZARE_AGGRO, m_creature);
    }

    void JustDied(Unit* /*pKiller*/) override
    {
        DoScriptText(SAY_STORMWROUGHT_DRAZARE_DEATH, m_creature);
    }

    bool CastRandomPlayerSpell(uint32 uiSpellId)
    {
        if (Unit* pTarget = m_creature->SelectAttackingTarget(ATTACKING_TARGET_RANDOM, 0, uiSpellId, SELECT_FLAG_PLAYER | SELECT_FLAG_IN_LOS | SELECT_FLAG_NO_TOTEM))
            return DoCastSpellIfCan(pTarget, uiSpellId) == CAST_OK;

        return false;
    }

    void UpdateAI(uint32 uiDiff) override
    {
        if (!m_creature->SelectHostileTarget() || !m_creature->GetVictim())
            return;

        if (m_uiDarkSeductionTimer <= uiDiff)
        {
            if (CastRandomPlayerSpell(SPELL_DARK_SEDUCTION))
            {
                DoScriptText(SAY_STORMWROUGHT_DRAZARE_SEDUCTION, m_creature);
                m_uiDarkSeductionTimer = urand(39000, 58000);
            }
        }
        else
            m_uiDarkSeductionTimer -= uiDiff;

        if (m_uiDrazareEmbraceAuraTimer <= uiDiff)
        {
            // 44053 expires into 44052 through SpellAuraHolder::HandleCastOnAuraRemoval.
            if (CastRandomPlayerSpell(SPELL_DRAZARE_EMBRACE_AURA))
                m_uiDrazareEmbraceAuraTimer = urand(12000, 24000);
        }
        else
            m_uiDrazareEmbraceAuraTimer -= uiDiff;

        DoMeleeAttackIfReady();
    }
};

CreatureAI* GetAI_boss_lady_drazare(Creature* pCreature)
{
    return new boss_lady_drazareAI(pCreature);
}

void AddSC_boss_lady_drazare()
{
    Script* pNewscript;

    pNewscript = new Script;
    pNewscript->Name = "boss_lady_drazare";
    pNewscript->GetAI = &GetAI_boss_lady_drazare;
    pNewscript->RegisterSelf();
}
