#include "scriptPCH.h"
#include "stormwrought_ruins.h"

struct boss_eldermaw_the_primordialAI : public ScriptedAI
{
    explicit boss_eldermaw_the_primordialAI(Creature* pCreature) : ScriptedAI(pCreature)
    {
        Reset();
    }

    uint32 m_uiTailSlapTimer;
    bool m_bCalledPrimordial;

    void Reset() override
    {
        m_uiTailSlapTimer = urand(4000, 6000);
        m_bCalledPrimordial = false;
    }

    void UpdateAI(uint32 uiDiff) override
    {
        if (!m_creature->SelectHostileTarget() || !m_creature->GetVictim())
            return;

        if (!m_bCalledPrimordial && m_creature->GetHealthPercent() <= 50.0f)
        {
            m_bCalledPrimordial = true;
            DoCastSpellIfCan(m_creature, SPELL_CALL_OF_THE_PRIMORDIAL, CF_TRIGGERED);
            DoCastSpellIfCan(m_creature, SPELL_PRIMORDIAL_REGENERATION, CF_TRIGGERED);

            for (uint8 i = 0; i < 2; ++i)
            {
                float fAngle = m_creature->GetOrientation() + (i == 0 ? 1.0f : -1.0f);
                float fX = m_creature->GetPositionX() + 2.0f * cos(fAngle);
                float fY = m_creature->GetPositionY() + 2.0f * sin(fAngle);

                if (Creature* pCrocolisk = m_creature->SummonCreature(NPC_ELDERMAW_CROCOLISK, fX, fY, m_creature->GetPositionZ(), m_creature->GetOrientation(), TEMPSUMMON_TIMED_OR_CORPSE_DESPAWN, 60000))
                    pCrocolisk->SetInCombatWithZone();
            }
        }

        if (m_uiTailSlapTimer <= uiDiff)
        {
            if (Unit* pTarget = m_creature->SelectAttackingTarget(ATTACKING_TARGET_RANDOM, 0, SPELL_TAIL_SLAP, SELECT_FLAG_IN_LOS | SELECT_FLAG_IN_MELEE_RANGE | SELECT_FLAG_NO_TOTEM))
            {
                if (DoCastSpellIfCan(pTarget, SPELL_TAIL_SLAP) == CAST_OK)
                    m_uiTailSlapTimer = urand(10000, 12000);
                else
                    m_uiTailSlapTimer = 1000;
            }
            else
                m_uiTailSlapTimer = 1000;
        }
        else
            m_uiTailSlapTimer -= uiDiff;

        DoMeleeAttackIfReady();
    }
};

CreatureAI* GetAI_boss_eldermaw_the_primordial(Creature* pCreature)
{
    return new boss_eldermaw_the_primordialAI(pCreature);
}

void AddSC_boss_eldermaw_the_primordial()
{
    Script* pNewscript;

    pNewscript = new Script;
    pNewscript->Name = "boss_eldermaw_the_primordial";
    pNewscript->GetAI = &GetAI_boss_eldermaw_the_primordial;
    pNewscript->RegisterSelf();
}
