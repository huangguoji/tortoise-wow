#include "scriptPCH.h"
#include "dragonmaw_retreat.h"

enum
{
    SPELL_FIREBALL      = 8401,
    SPELL_FLAME_BREATH  = 16396,
    SPELL_WING_FLAP     = 12882,
};

struct boss_searistraszAI : public ScriptedAI
{
    explicit boss_searistraszAI(Creature* pCreature) : ScriptedAI(pCreature)
    {
        m_pInstance = static_cast<ScriptedInstance*>(pCreature->GetInstanceData());
        Reset();
    }

    ScriptedInstance* m_pInstance;

    uint32 m_uiFireballTimer;
    uint32 m_uiFlameBreathTimer;
    uint32 m_uiWingFlapTimer;

    bool m_bUsedHalfHealthLine;

    void Reset() override
    {
        m_uiFireballTimer = urand(5 * IN_MILLISECONDS, 7 * IN_MILLISECONDS);
        m_uiFlameBreathTimer = urand(2 * IN_MILLISECONDS, 4 * IN_MILLISECONDS);
        m_uiWingFlapTimer = urand(500, 1500);
        m_bUsedHalfHealthLine = false;

        if (!m_pInstance || m_pInstance->GetData(DATA_DRAGONMAW_ENCHANTERS_DEAD) < MAX_DRAGONMAW_ENCHANTERS)
        {
            if (!m_creature->HasAura(SPELL_ENCHANTING_FLAMES))
                m_creature->CastSpell(m_creature, SPELL_ENCHANTING_FLAMES, true);
        }
        else
            m_creature->RemoveAurasDueToSpell(SPELL_ENCHANTING_FLAMES);
    }

    void Aggro(Unit* /*pWho*/) override
    {
        DoScriptText(SAY_DRAGONMAW_SEARISTRASZ_AGGRO, m_creature);
    }

    void JustDied(Unit* /*pKiller*/) override
    {
        DoScriptText(SAY_DRAGONMAW_SEARISTRASZ_DEATH, m_creature);
    }

    void UpdateAI(const uint32 uiDiff) override
    {
        if (!m_creature->SelectHostileTarget() || !m_creature->GetVictim())
            return;

        if (!m_bUsedHalfHealthLine && m_creature->GetHealthPercent() <= 50.0f)
        {
            m_bUsedHalfHealthLine = true;
            DoScriptText(SAY_DRAGONMAW_SEARISTRASZ_HALF_HEALTH, m_creature);
        }

        if (m_uiWingFlapTimer < uiDiff)
        {
            Unit* pTarget = m_creature->SelectAttackingTarget(ATTACKING_TARGET_RANDOM, 0, SPELL_WING_FLAP, SELECT_FLAG_IN_MELEE_RANGE | SELECT_FLAG_NO_TOTEM);
            if (!pTarget)
                pTarget = m_creature->GetVictim();

            if (DoCastSpellIfCan(pTarget, SPELL_WING_FLAP) == CAST_OK)
                m_uiWingFlapTimer = urand(15 * IN_MILLISECONDS, 18 * IN_MILLISECONDS);
        }
        else
            m_uiWingFlapTimer -= uiDiff;

        if (m_uiFlameBreathTimer < uiDiff)
        {
            if (DoCastSpellIfCan(m_creature->GetVictim(), SPELL_FLAME_BREATH) == CAST_OK)
                m_uiFlameBreathTimer = urand(28 * IN_MILLISECONDS, 36 * IN_MILLISECONDS);
        }
        else
            m_uiFlameBreathTimer -= uiDiff;

        if (m_uiFireballTimer < uiDiff)
        {
            Unit* pTarget = m_creature->SelectAttackingTarget(ATTACKING_TARGET_RANDOM, 0, SPELL_FIREBALL, SELECT_FLAG_IN_LOS | SELECT_FLAG_NO_TOTEM);
            if (!pTarget)
                pTarget = m_creature->GetVictim();

            if (DoCastSpellIfCan(pTarget, SPELL_FIREBALL) == CAST_OK)
                m_uiFireballTimer = urand(18 * IN_MILLISECONDS, 23 * IN_MILLISECONDS);
        }
        else
            m_uiFireballTimer -= uiDiff;

        DoMeleeAttackIfReady();
    }
};

CreatureAI* GetAI_boss_searistrasz(Creature* pCreature)
{
    return new boss_searistraszAI(pCreature);
}

void AddSC_boss_searistrasz()
{
    Script* pNewscript = new Script;
    pNewscript->Name = "boss_searistrasz";
    pNewscript->GetAI = &GetAI_boss_searistrasz;
    pNewscript->RegisterSelf();
}
