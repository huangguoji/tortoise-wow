#include "scriptPCH.h"
#include "frostmane_hollow.h"

namespace
{
bool IsHailarRitualist(Creature const* creature)
{
    if (creature->GetMapId() != MAP_FROSTMANE_HOLLOW)
        return false;

    switch (creature->GetGUIDLow())
    {
        case GUID_HAILAR_RITUALIST_1:
        case GUID_HAILAR_RITUALIST_2:
        case GUID_HAILAR_RITUALIST_3:
        case GUID_HAILAR_RITUALIST_4:
        case GUID_HAILAR_RITUALIST_5:
            return true;
        default:
            return false;
    }
}

struct boss_hailar_the_frigidAI : public ScriptedAI
{
    explicit boss_hailar_the_frigidAI(Creature* creature) : ScriptedAI(creature)
    {
        Reset();
    }

    void Reset() override
    {
        m_usedHalfHealthLine = false;
    }

    void Aggro(Unit* /*who*/) override
    {
        DoScriptText(SAY_HAILAR_AGGRO, m_creature);
    }

    void JustDied(Unit* /*killer*/) override
    {
        DoScriptText(SAY_HAILAR_DEATH, m_creature);
    }

    void UpdateAI(uint32 diff) override
    {
        if (!m_usedHalfHealthLine && m_creature->IsInCombat() && m_creature->GetHealthPercent() <= 50.0f)
        {
            m_usedHalfHealthLine = true;
            DoScriptText(SAY_HAILAR_HALF_HEALTH, m_creature);
        }

        ScriptedAI::UpdateAI(diff);
    }

private:
    bool m_usedHalfHealthLine;
};

struct npc_frostmane_ritualistAI : public ScriptedAI
{
    explicit npc_frostmane_ritualistAI(Creature* creature) :
        ScriptedAI(creature),
        m_isHailarRitualist(IsHailarRitualist(creature))
    {
        Reset();
    }

    void Reset() override
    {
        if (!m_isHailarRitualist)
            return;

        m_creature->SetReactState(REACT_PASSIVE);
        SetCombatMovement(false);
        SetMeleeAttack(false);
        m_channelRetryTimer = 500;
    }

    void MoveInLineOfSight(Unit* who) override
    {
        if (!m_isHailarRitualist)
            ScriptedAI::MoveInLineOfSight(who);
    }

    void AttackedBy(Unit* attacker) override
    {
        if (!m_isHailarRitualist)
            ScriptedAI::AttackedBy(attacker);
    }

    void AttackStart(Unit* who) override
    {
        if (!m_isHailarRitualist)
            ScriptedAI::AttackStart(who);
    }

    void UpdateAI(uint32 diff) override
    {
        if (!m_isHailarRitualist)
        {
            ScriptedAI::UpdateAI(diff);
            return;
        }

        if (m_creature->IsNonMeleeSpellCasted(false))
            return;

        if (m_channelRetryTimer > diff)
        {
            m_channelRetryTimer -= diff;
            return;
        }

        if (Creature* hailar = m_creature->FindNearestCreature(NPC_HAILAR_THE_FRIGID, 100.0f, true))
        {
            if (DoCastSpellIfCan(hailar, SPELL_FROSTMANE_RITUAL, CF_FORCE_CAST) == CAST_OK)
            {
                m_channelRetryTimer = 5 * IN_MILLISECONDS;
                return;
            }
        }

        m_channelRetryTimer = 1 * IN_MILLISECONDS;
    }

private:
    bool const m_isHailarRitualist;
    uint32 m_channelRetryTimer;
};

CreatureAI* GetAI_boss_hailar_the_frigid(Creature* creature)
{
    return new boss_hailar_the_frigidAI(creature);
}

CreatureAI* GetAI_npc_frostmane_ritualist(Creature* creature)
{
    return new npc_frostmane_ritualistAI(creature);
}
}

void AddSC_boss_hailar_the_frigid()
{
    Script* script = new Script;
    script->Name = "boss_hailar_the_frigid";
    script->GetAI = &GetAI_boss_hailar_the_frigid;
    script->RegisterSelf();

    script = new Script;
    script->Name = "npc_frostmane_ritualist";
    script->GetAI = &GetAI_npc_frostmane_ritualist;
    script->RegisterSelf();
}
