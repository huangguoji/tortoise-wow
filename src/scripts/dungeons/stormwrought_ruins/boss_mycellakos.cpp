#include "scriptPCH.h"
#include "stormwrought_ruins.h"

struct boss_mycellakosAI : public ScriptedAI
{
    explicit boss_mycellakosAI(Creature* pCreature) : ScriptedAI(pCreature)
    {
        Reset();
    }

    uint32 m_uiFungusTimer;
    uint32 m_uiFungusDetonateTimer;
    uint32 m_uiDecayingMoldTimer;
    bool m_bFungusActive;
    float m_fFungusX;
    float m_fFungusY;
    float m_fFungusZ;
    float m_fFungusO;

    void Reset() override
    {
        m_uiFungusTimer = 8000;
        m_uiFungusDetonateTimer = 0;
        m_uiDecayingMoldTimer = urand(25000, 30000);
        m_bFungusActive = false;
        m_fFungusX = m_fFungusY = m_fFungusZ = m_fFungusO = 0.0f;
    }

    void SpawnGrowth(float fX, float fY, float fZ, float fO)
    {
        for (uint8 i = 0; i < 2; ++i)
        {
            float fOffset = i == 0 ? 1.0f : -1.0f;
            if (Creature* pGrowth = m_creature->SummonCreature(NPC_GROWTH_OF_MYCELLAKOS, fX + fOffset, fY, fZ, fO, TEMPSUMMON_TIMED_OR_CORPSE_DESPAWN, 60000))
                pGrowth->SetInCombatWithZone();
        }
    }

    void DetonateFungus()
    {
        SpawnGrowth(m_fFungusX, m_fFungusY, m_fFungusZ, m_fFungusO);
        m_creature->CastSpell(m_fFungusX, m_fFungusY, m_fFungusZ, SPELL_VOLATILE_FUNGUS, true);
        m_bFungusActive = false;
    }

    void UpdateAI(uint32 uiDiff) override
    {
        if (!m_creature->SelectHostileTarget() || !m_creature->GetVictim())
            return;

        if (m_bFungusActive)
        {
            if (m_uiFungusDetonateTimer <= uiDiff)
                DetonateFungus();
            else
                m_uiFungusDetonateTimer -= uiDiff;
        }

        if (m_uiFungusTimer <= uiDiff)
        {
            m_fFungusX = m_creature->GetPositionX();
            m_fFungusY = m_creature->GetPositionY();
            m_fFungusZ = m_creature->GetPositionZ();
            m_fFungusO = m_creature->GetOrientation();

            m_creature->SummonGameObject(GO_VOLATILE_FUNGUS, m_fFungusX, m_fFungusY, m_fFungusZ, m_fFungusO, 0.0f, 0.0f, 0.0f, 0.0f, 3);
            m_bFungusActive = true;
            m_uiFungusDetonateTimer = 3000;
            m_uiFungusTimer = urand(15000, 20000);
        }
        else
            m_uiFungusTimer -= uiDiff;

        if (m_uiDecayingMoldTimer <= uiDiff)
        {
            if (DoCastSpellIfCan(m_creature, SPELL_DECAYING_MOLD) == CAST_OK)
                m_uiDecayingMoldTimer = urand(25000, 30000);
        }
        else
            m_uiDecayingMoldTimer -= uiDiff;

        DoMeleeAttackIfReady();
    }
};

CreatureAI* GetAI_boss_mycellakos(Creature* pCreature)
{
    return new boss_mycellakosAI(pCreature);
}

void AddSC_boss_mycellakos()
{
    Script* pNewscript;

    pNewscript = new Script;
    pNewscript->Name = "boss_mycellakos";
    pNewscript->GetAI = &GetAI_boss_mycellakos;
    pNewscript->RegisterSelf();
}
