#include "scriptPCH.h"
#include "windhorn_canyon.h"

namespace
{
struct boss_bonespeaker_narlgomAI : public ScriptedAI
{
    explicit boss_bonespeaker_narlgomAI(Creature* creature) : ScriptedAI(creature), m_allowIdleChannel(true)
    {
        Reset();
    }

    void Reset() override
    {
        m_boneArmorTimer = urand(0, 1 * IN_MILLISECONDS);
        m_rainOfFireTimer = urand(1 * IN_MILLISECONDS, 2 * IN_MILLISECONDS);
        m_idleChannelTimer = 1 * IN_MILLISECONDS;
        m_summonedRotag = false;
    }

    void Aggro(Unit* /*who*/) override
    {
        m_allowIdleChannel = false;
        StopIdleChannel();
        DoScriptText(SAY_NARLGOM_AGGRO, m_creature);
    }

    void JustDied(Unit* /*killer*/) override
    {
        DoScriptText(SAY_NARLGOM_DEATH, m_creature);
    }

    void JustReachedHome() override
    {
        m_allowIdleChannel = true;
        m_idleChannelTimer = 1 * IN_MILLISECONDS;
    }

    void JustRespawned() override
    {
        m_allowIdleChannel = true;
        ScriptedAI::JustRespawned();
    }

    void UpdateAI(uint32 diff) override
    {
        if (!m_creature->IsInCombat())
        {
            UpdateIdleChannel(diff);
            return;
        }

        if (!m_creature->SelectHostileTarget() || !m_creature->GetVictim())
            return;

        if (!m_summonedRotag && m_creature->GetHealthPercent() <= 50.0f)
        {
            if (DoCastSpellIfCan(m_creature, SPELL_SUMMON_ROTAG, CF_INTERRUPT_PREVIOUS) == CAST_OK)
            {
                m_summonedRotag = true;
                DoScriptText(SAY_NARLGOM_SUMMON_ROTAG, m_creature);
                return;
            }
        }

        if (m_boneArmorTimer <= diff)
        {
            if (DoCastSpellIfCan(m_creature, SPELL_BONE_ARMOR, CF_AURA_NOT_PRESENT) == CAST_OK)
            {
                m_boneArmorTimer = urand(14 * IN_MILLISECONDS, 31 * IN_MILLISECONDS);
                return;
            }
        }
        else
            m_boneArmorTimer -= diff;

        if (m_rainOfFireTimer <= diff)
        {
            if (DoCastSpellIfCan(m_creature->GetVictim(), SPELL_RAIN_OF_FIRE) == CAST_OK)
            {
                m_rainOfFireTimer = urand(17 * IN_MILLISECONDS, 22 * IN_MILLISECONDS);
                return;
            }
        }
        else
            m_rainOfFireTimer -= diff;

        DoMeleeAttackIfReady();
    }

private:
    Creature* GetChampionRotag() const
    {
        return m_creature->FindNearestCreature(NPC_CHAMPION_ROTAG, 100.0f, true);
    }

    void UpdateIdleChannel(uint32 diff)
    {
        if (!m_allowIdleChannel || m_creature->IsNonMeleeSpellCasted(false))
            return;

        if (m_idleChannelTimer > diff)
        {
            m_idleChannelTimer -= diff;
            return;
        }

        if (Creature* rotag = GetChampionRotag())
        {
            if (DoCastSpellIfCan(rotag, SPELL_TARGETED_ARCANE_CHANNELING, CF_FORCE_CAST) == CAST_OK)
            {
                rotag->SetStandState(UNIT_STAND_STATE_DEAD);
                m_idleChannelTimer = 5 * IN_MILLISECONDS;
                return;
            }
        }

        m_idleChannelTimer = 1 * IN_MILLISECONDS;
    }

    void StopIdleChannel()
    {
        m_creature->InterruptNonMeleeSpells(false, SPELL_TARGETED_ARCANE_CHANNELING);

        if (Creature* rotag = GetChampionRotag())
        {
            rotag->RemoveAurasDueToSpell(SPELL_TARGETED_ARCANE_CHANNELING);
            rotag->CombatStop(true);
            rotag->SetStandState(UNIT_STAND_STATE_DEAD);
        }
    }

    bool m_allowIdleChannel;
    bool m_summonedRotag;
    uint32 m_boneArmorTimer;
    uint32 m_rainOfFireTimer;
    uint32 m_idleChannelTimer;
};

struct npc_champion_rotagAI : public ScriptedAI
{
    explicit npc_champion_rotagAI(Creature* creature) : ScriptedAI(creature)
    {
        Reset();
    }

    void Reset() override
    {
        m_creature->SetReactState(REACT_PASSIVE);
        m_creature->SetStandState(UNIT_STAND_STATE_DEAD);
    }

    void MoveInLineOfSight(Unit* /*who*/) override {}
    void AttackStart(Unit* /*who*/) override {}

    void Aggro(Unit* /*who*/) override
    {
        m_creature->SetStandState(UNIT_STAND_STATE_DEAD);
    }

    void UpdateAI(uint32 /*diff*/) override
    {
        if (m_creature->IsInCombat())
            m_creature->CombatStop(true);

        m_creature->SetStandState(UNIT_STAND_STATE_DEAD);
    }
};

CreatureAI* GetAI_boss_bonespeaker_narlgom(Creature* creature)
{
    return new boss_bonespeaker_narlgomAI(creature);
}

CreatureAI* GetAI_npc_champion_rotag(Creature* creature)
{
    return new npc_champion_rotagAI(creature);
}
}

void AddSC_boss_bonespeaker_narlgom()
{
    Script* script = new Script;
    script->Name = "boss_bonespeaker_narlgom";
    script->GetAI = &GetAI_boss_bonespeaker_narlgom;
    script->RegisterSelf();

    script = new Script;
    script->Name = "npc_champion_rotag";
    script->GetAI = &GetAI_npc_champion_rotag;
    script->RegisterSelf();
}
