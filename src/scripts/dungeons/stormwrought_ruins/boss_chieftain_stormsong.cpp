#include "scriptPCH.h"
#include "stormwrought_ruins.h"

struct boss_chieftain_stormsongAI : public ScriptedAI
{
    explicit boss_chieftain_stormsongAI(Creature* pCreature) : ScriptedAI(pCreature)
    {
        Reset();
    }

    uint32 m_uiForkedLightningTimer;
    uint32 m_uiStunBombTimer;
    uint32 m_uiLightningStrikeTimer;
    bool m_bDidThreatReset;

    void Reset() override
    {
        m_uiForkedLightningTimer = urand(5000, 8000);
        m_uiStunBombTimer = urand(25000, 30000);
        m_uiLightningStrikeTimer = urand(12000, 16000);
        m_bDidThreatReset = false;
    }

    void Aggro(Unit* /*pWho*/) override
    {
        DoScriptText(SAY_STORMWROUGHT_STORMSONG_AGGRO, m_creature);
    }

    void JustDied(Unit* /*pKiller*/) override
    {
        DoScriptText(SAY_STORMWROUGHT_STORMSONG_DEATH, m_creature);
    }

    void UpdateAI(uint32 uiDiff) override
    {
        if (!m_creature->SelectHostileTarget() || !m_creature->GetVictim())
            return;

        if (!m_bDidThreatReset && m_creature->GetHealthPercent() <= 50.0f)
        {
            DoResetThreat();
            m_bDidThreatReset = true;
        }

        if (m_uiForkedLightningTimer <= uiDiff)
        {
            if (DoCastSpellIfCan(m_creature->GetVictim(), SPELL_FORKED_LIGHTNING) == CAST_OK)
                m_uiForkedLightningTimer = urand(11000, 15000);
        }
        else
            m_uiForkedLightningTimer -= uiDiff;

        if (m_uiStunBombTimer <= uiDiff)
        {
            if (Unit* pTarget = m_creature->SelectAttackingTarget(ATTACKING_TARGET_RANDOM, 0, SPELL_STUN_BOMB, SELECT_FLAG_IN_LOS | SELECT_FLAG_NO_TOTEM))
            {
                if (DoCastSpellIfCan(pTarget, SPELL_STUN_BOMB) == CAST_OK)
                    m_uiStunBombTimer = urand(25000, 30000);
            }
        }
        else
            m_uiStunBombTimer -= uiDiff;

        if (m_uiLightningStrikeTimer <= uiDiff)
        {
            if (DoCastSpellIfCan(m_creature->GetVictim(), SPELL_LIGHTNING_STRIKE) == CAST_OK)
                m_uiLightningStrikeTimer = urand(14000, 20000);
        }
        else
            m_uiLightningStrikeTimer -= uiDiff;

        DoMeleeAttackIfReady();
    }
};

CreatureAI* GetAI_boss_chieftain_stormsong(Creature* pCreature)
{
    return new boss_chieftain_stormsongAI(pCreature);
}

void AddSC_boss_chieftain_stormsong()
{
    Script* pNewscript;

    pNewscript = new Script;
    pNewscript->Name = "boss_chieftain_stormsong";
    pNewscript->GetAI = &GetAI_boss_chieftain_stormsong;
    pNewscript->RegisterSelf();
}
