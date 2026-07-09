#include "scriptPCH.h"

enum
{
    NPC_BASALTHAR = 65020,
    NPC_SMOLDARIS = 65021,
};

struct boss_twin_golemAI : public ScriptedAI
{
    boss_twin_golemAI(Creature* pCreature) : ScriptedAI(pCreature) { }

    void Reset() override { }

    uint32 GetTwinEntry() const
    {
        return m_creature->GetEntry() == NPC_BASALTHAR ? NPC_SMOLDARIS : NPC_BASALTHAR;
    }

    Creature* GetTwin(bool bAlive = true) const
    {
        return m_creature->FindNearestCreature(GetTwinEntry(), 100.0f, bAlive);
    }

    void Aggro(Unit* pWho) override
    {
        m_creature->SetInCombatWithZone();

        if (Creature* pTwin = GetTwin())
        {
            pTwin->SetInCombatWithZone();
            if (!pTwin->GetVictim())
                pTwin->AI()->AttackStart(pWho);
        }
    }

    void DamageTaken(Unit* /*pDealer*/, uint32& uiDamage) override
    {
        Creature* pTwin = GetTwin();
        if (!pTwin)
            return;

        if (uiDamage >= m_creature->GetHealth())
        {
            pTwin->SetHealth(1);
            return;
        }

        float fDamagePercent = float(uiDamage) / float(m_creature->GetMaxHealth());
        uint32 uiTwinDamage = uint32(fDamagePercent * float(pTwin->GetMaxHealth()));
        uiTwinDamage = std::min(uiTwinDamage, pTwin->GetHealth() - 1);
        pTwin->SetHealth(pTwin->GetHealth() - uiTwinDamage);
        pTwin->CountDamageTaken(uiTwinDamage, true);
    }

    void HealedBy(Unit* /*pHealer*/, uint32& uiAmount) override
    {
        if (Creature* pTwin = GetTwin())
        {
            float fHealPercent = float(uiAmount) / float(m_creature->GetMaxHealth());
            uint32 uiTwinHeal = uint32(fHealPercent * float(pTwin->GetMaxHealth()));
            pTwin->SetHealth(std::min(pTwin->GetHealth() + uiTwinHeal, pTwin->GetMaxHealth()));
        }
    }

    void JustDied(Unit* pKiller) override
    {
        if (Creature* pTwin = GetTwin())
            pKiller->Kill(pTwin, nullptr, false);
    }

    void UpdateAI(const uint32 uiDiff) override
    {
        if (!m_creature->SelectHostileTarget() || !m_creature->GetVictim())
            return;

        if (!m_CreatureSpells.empty())
            UpdateSpellsList(uiDiff);

        DoMeleeAttackIfReady();
    }
};

CreatureAI* GetAI_boss_twin_golem(Creature* pCreature)
{
    return new boss_twin_golemAI(pCreature);
}

void AddSC_boss_twin_golems()
{
    Script* pNewScript = new Script;
    pNewScript->Name = "boss_twin_golem";
    pNewScript->GetAI = &GetAI_boss_twin_golem;
    pNewScript->RegisterSelf();
}
