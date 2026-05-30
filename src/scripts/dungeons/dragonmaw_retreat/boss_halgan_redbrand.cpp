#include "scriptPCH.h"
#include "dragonmaw_retreat.h"

enum
{
    SPELL_CURSE_OF_AGONY    = 11711,
    SPELL_MIND_FLAY         = 17311,
    SPELL_PSYCHIC_SCREAM    = 22884,
};

struct boss_halgan_redbrandAI : public ScriptedAI
{
    explicit boss_halgan_redbrandAI(Creature* pCreature) : ScriptedAI(pCreature)
    {
        Reset();
    }

    uint32 m_uiCurseOfAgonyTimer;
    uint32 m_uiMindFlayTimer;

    bool m_bUsedPsychicScream;

    void Reset() override
    {
        m_uiCurseOfAgonyTimer = urand(2 * IN_MILLISECONDS, 4 * IN_MILLISECONDS);
        m_uiMindFlayTimer = urand(5 * IN_MILLISECONDS, 7 * IN_MILLISECONDS);

        m_bUsedPsychicScream = false;
    }

    void Aggro(Unit* /*pWho*/) override
    {
        DoScriptText(SAY_DRAGONMAW_HALGAN_AGGRO, m_creature);
    }

    void JustDied(Unit* /*pKiller*/) override
    {
        DoScriptText(SAY_DRAGONMAW_HALGAN_DEATH, m_creature);
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

        if (!m_bUsedPsychicScream && m_creature->GetHealthPercent() <= 50.0f)
        {
            if (DoCastSpellIfCan(m_creature->GetVictim(), SPELL_PSYCHIC_SCREAM) == CAST_OK)
            {
                m_bUsedPsychicScream = true;
                DoScriptText(SAY_DRAGONMAW_HALGAN_HALF_HEALTH, m_creature);
            }
        }

        if (m_uiCurseOfAgonyTimer < uiDiff)
        {
            if (Unit* pTarget = SelectRandomHostileTarget(SPELL_CURSE_OF_AGONY))
            {
                if (DoCastSpellIfCan(pTarget, SPELL_CURSE_OF_AGONY) == CAST_OK)
                    m_uiCurseOfAgonyTimer = urand(14 * IN_MILLISECONDS, 18 * IN_MILLISECONDS);
            }
        }
        else
            m_uiCurseOfAgonyTimer -= uiDiff;

        if (m_uiMindFlayTimer < uiDiff)
        {
            if (Unit* pTarget = SelectRandomHostileTarget(SPELL_MIND_FLAY))
            {
                if (DoCastSpellIfCan(pTarget, SPELL_MIND_FLAY) == CAST_OK)
                    m_uiMindFlayTimer = urand(10 * IN_MILLISECONDS, 14 * IN_MILLISECONDS);
            }
        }
        else
            m_uiMindFlayTimer -= uiDiff;

        DoMeleeAttackIfReady();
    }
};

CreatureAI* GetAI_boss_halgan_redbrand(Creature* pCreature)
{
    return new boss_halgan_redbrandAI(pCreature);
}

void AddSC_boss_halgan_redbrand()
{
    Script* pNewscript = new Script;
    pNewscript->Name = "boss_halgan_redbrand";
    pNewscript->GetAI = &GetAI_boss_halgan_redbrand;
    pNewscript->RegisterSelf();
}
