#include "scriptPCH.h"
#include "dragonmaw_retreat.h"

enum
{
    SPELL_SOUL_DOMINATION    = 52042,
    SPELL_WITHERING_SOUL     = 52044,
    SPELL_DEBILITATED_SOUL   = 52045,
    SPELL_EMPOWERED_SOUL     = 52046,
};

struct boss_zuluhed_the_whackedAI : public ScriptedAI
{
    explicit boss_zuluhed_the_whackedAI(Creature* pCreature) : ScriptedAI(pCreature)
    {
        Reset();
    }

    uint32 m_uiSoulDominationTimer;
    uint32 m_uiSoulSequenceTimer;

    bool m_bSoulSequenceStarted;
    bool m_bSoulSequenceFinished;

    void Reset() override
    {
        m_uiSoulDominationTimer = urand(6 * IN_MILLISECONDS, 8 * IN_MILLISECONDS);
        m_uiSoulSequenceTimer = 0;

        m_bSoulSequenceStarted = false;
        m_bSoulSequenceFinished = false;
    }

    void Aggro(Unit* /*pWho*/) override
    {
        DoScriptText(SAY_DRAGONMAW_ZULUHED_AGGRO, m_creature);
    }

    void JustDied(Unit* /*pKiller*/) override
    {
        DoScriptText(SAY_DRAGONMAW_ZULUHED_DEATH, m_creature);
    }

    void UpdateAI(const uint32 uiDiff) override
    {
        if (!m_creature->SelectHostileTarget() || !m_creature->GetVictim())
            return;

        if (m_uiSoulDominationTimer < uiDiff)
        {
            Unit* pTarget = m_creature->SelectAttackingTarget(ATTACKING_TARGET_RANDOM, 0, SPELL_SOUL_DOMINATION, SELECT_FLAG_PLAYER | SELECT_FLAG_IN_LOS | SELECT_FLAG_NO_TOTEM);
            if (!pTarget)
                pTarget = m_creature->GetVictim();

            if (DoCastSpellIfCan(pTarget, SPELL_SOUL_DOMINATION) == CAST_OK)
            {
                DoScriptText(SAY_DRAGONMAW_ZULUHED_SOUL_DOMINATION, m_creature);
                m_uiSoulDominationTimer = urand(14 * IN_MILLISECONDS, 18 * IN_MILLISECONDS);
            }
        }
        else
            m_uiSoulDominationTimer -= uiDiff;

        if (!m_bSoulSequenceStarted && m_creature->GetHealthPercent() <= 50.0f)
        {
            if (DoCastSpellIfCan(m_creature, SPELL_WITHERING_SOUL) == CAST_OK)
            {
                m_bSoulSequenceStarted = true;
                m_uiSoulSequenceTimer = 6500;
                DoScriptText(SAY_DRAGONMAW_ZULUHED_WITHERING_SOUL, m_creature);
            }
        }

        if (m_bSoulSequenceStarted && !m_bSoulSequenceFinished)
        {
            if (m_uiSoulSequenceTimer < uiDiff)
            {
                if (DoCastSpellIfCan(m_creature, SPELL_DEBILITATED_SOUL) == CAST_OK)
                {
                    m_creature->CastSpell(m_creature, SPELL_EMPOWERED_SOUL, true);
                    m_bSoulSequenceFinished = true;
                }
            }
            else
                m_uiSoulSequenceTimer -= uiDiff;
        }

        DoMeleeAttackIfReady();
    }
};

CreatureAI* GetAI_boss_zuluhed_the_whacked(Creature* pCreature)
{
    return new boss_zuluhed_the_whackedAI(pCreature);
}

void AddSC_boss_zuluhed_the_whacked()
{
    Script* pNewscript = new Script;
    pNewscript->Name = "boss_zuluhed_the_whacked";
    pNewscript->GetAI = &GetAI_boss_zuluhed_the_whacked;
    pNewscript->RegisterSelf();
}
