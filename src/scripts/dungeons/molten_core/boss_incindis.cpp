#include "scriptPCH.h"

enum
{
    NPC_SMALL_INCENDIC_EGG = 52146,
    NPC_LARGE_INCENDIC_EGG = 52147,

    SPELL_QUAKING_STOMP    = 42036,
    SPELL_FIRE_NOVA        = 42037,
    SPELL_MOLTEN_BITE      = 42040,
    SPELL_SMALL_HATCH      = 42042,
    SPELL_LARGE_HATCH      = 42044,
};

struct boss_incindisAI : public ScriptedAI
{
    boss_incindisAI(Creature* pCreature) : ScriptedAI(pCreature)
    {
        Reset();
    }

    uint32 m_uiMoltenBiteTimer;
    uint32 m_uiQuakingStompTimer;
    uint32 m_uiFireNovaTimer;
    bool m_bEggsSummoned;
    bool m_bFireNovaPending;
    GuidList m_lEggGuids;

    void Reset() override
    {
        for (const auto& guid : m_lEggGuids)
        {
            if (Creature* pEgg = m_creature->GetMap()->GetCreature(guid))
                pEgg->ForcedDespawn();
        }

        m_lEggGuids.clear();
        m_uiMoltenBiteTimer = 6000;
        m_uiQuakingStompTimer = urand(24000, 30000);
        m_uiFireNovaTimer = 0;
        m_bEggsSummoned = false;
        m_bFireNovaPending = false;
    }

    void JustSummoned(Creature* pSummoned) override
    {
        if (pSummoned->GetEntry() == NPC_SMALL_INCENDIC_EGG || pSummoned->GetEntry() == NPC_LARGE_INCENDIC_EGG)
            m_lEggGuids.push_back(pSummoned->GetObjectGuid());
    }

    void SummonedCreatureJustDied(Creature* pSummoned) override
    {
        m_lEggGuids.remove(pSummoned->GetObjectGuid());
    }

    void SummonEgg(uint32 uiEntry)
    {
        float fX, fY, fZ;
        m_creature->GetRandomPoint(m_creature->GetPositionX(), m_creature->GetPositionY(),
            m_creature->GetPositionZ(), 15.0f, fX, fY, fZ);
        m_creature->SummonCreature(uiEntry, fX, fY, fZ, frand(0.0f, 2.0f * M_PI_F),
            TEMPSUMMON_CORPSE_DESPAWN, 0);
    }

    void UpdateAI(const uint32 uiDiff) override
    {
        if (!m_creature->SelectHostileTarget() || !m_creature->GetVictim())
            return;

        if (!m_bEggsSummoned && m_creature->GetHealthPercent() <= 50.0f)
        {
            SummonEgg(NPC_LARGE_INCENDIC_EGG);
            SummonEgg(NPC_SMALL_INCENDIC_EGG);
            SummonEgg(NPC_SMALL_INCENDIC_EGG);
            m_bEggsSummoned = true;
        }

        if (m_bFireNovaPending)
        {
            if (m_uiFireNovaTimer <= uiDiff)
            {
                if (DoCastSpellIfCan(m_creature, SPELL_FIRE_NOVA) == CAST_OK)
                {
                    m_bFireNovaPending = false;
                    m_uiFireNovaTimer = 0;
                }
            }
            else
                m_uiFireNovaTimer -= uiDiff;
        }

        if (m_uiQuakingStompTimer <= uiDiff)
        {
            if (DoCastSpellIfCan(m_creature, SPELL_QUAKING_STOMP) == CAST_OK)
            {
                m_uiQuakingStompTimer = urand(24000, 30000);
                m_uiFireNovaTimer = 1000;
                m_bFireNovaPending = true;
            }
        }
        else
            m_uiQuakingStompTimer -= uiDiff;

        if (m_uiMoltenBiteTimer <= uiDiff)
        {
            // Molten Bite triggers Molten Armor (42039) through its second effect.
            if (DoCastSpellIfCan(m_creature->GetVictim(), SPELL_MOLTEN_BITE) == CAST_OK)
                m_uiMoltenBiteTimer = 6000;
        }
        else
            m_uiMoltenBiteTimer -= uiDiff;

        DoMeleeAttackIfReady();
    }
};

struct npc_incendic_eggAI : public ScriptedAI
{
    npc_incendic_eggAI(Creature* pCreature) : ScriptedAI(pCreature)
    {
        m_creature->SetReactState(REACT_PASSIVE);
        Reset();
    }

    bool m_bHatched;

    void Reset() override
    {
        m_creature->SetReactState(REACT_PASSIVE);
        m_bHatched = false;
    }

    void AttackStart(Unit* /*pWho*/) override { }
    void MoveInLineOfSight(Unit* /*pWho*/) override { }

    void UpdateAI(const uint32 /*uiDiff*/) override
    {
        if (m_bHatched)
            return;

        uint32 uiSpell = m_creature->GetEntry() == NPC_LARGE_INCENDIC_EGG ? SPELL_LARGE_HATCH : SPELL_SMALL_HATCH;
        if (DoCastSpellIfCan(m_creature, uiSpell) == CAST_OK)
            m_bHatched = true;
    }
};

CreatureAI* GetAI_boss_incindis(Creature* pCreature)
{
    return new boss_incindisAI(pCreature);
}

CreatureAI* GetAI_npc_incendic_egg(Creature* pCreature)
{
    return new npc_incendic_eggAI(pCreature);
}

void AddSC_boss_incindis()
{
    Script* pNewScript = new Script;
    pNewScript->Name = "boss_incindis";
    pNewScript->GetAI = &GetAI_boss_incindis;
    pNewScript->RegisterSelf();

    pNewScript = new Script;
    pNewScript->Name = "npc_incendic_egg";
    pNewScript->GetAI = &GetAI_npc_incendic_egg;
    pNewScript->RegisterSelf();
}
