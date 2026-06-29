#include "scriptPCH.h"
#include "blackfathom_deeps.h"

namespace
{
enum VelthelaxxSpells
{
    SPELL_MIND_FLAY       = 18807,
    SPELL_HOWL_OF_TERROR  = 5484,
    SPELL_ENRAGE          = 8269,
};

enum VelthelaxxTexts : uint32
{
    SAY_VELTHELAXX_AGGRO       = 6253001,
    SAY_VELTHELAXX_HALF_HEALTH = 6253002,
    SAY_VELTHELAXX_DEATH       = 6253003,
};

enum VelthelaxxCreatures : uint32
{
    WOJI_DB_GUID = 2589822,
};

struct boss_velthelaxx_the_defilerAI : public ScriptedAI
{
    explicit boss_velthelaxx_the_defilerAI(Creature* creature) : ScriptedAI(creature)
    {
        Reset();
    }

    void Reset() override
    {
        m_idleChannelTimer = 1 * IN_MILLISECONDS;
        m_mindFlayTimer = 1 * IN_MILLISECONDS;
        m_halfHealthLineDone = false;
        m_howlOfTerrorDone = false;
        m_enrageDone = false;
        m_allowIdleChannel = true;
        m_idleChannelActive = false;
    }

    void JustRespawned() override
    {
        ScriptedAI::JustRespawned();
        m_allowIdleChannel = true;
        m_idleChannelTimer = 1 * IN_MILLISECONDS;
    }

    void JustReachedHome() override
    {
        Reset();
    }

    void Aggro(Unit* /*who*/) override
    {
        m_allowIdleChannel = false;
        StopIdleChannel();
        DoScriptText(SAY_VELTHELAXX_AGGRO, m_creature);
    }

    void JustDied(Unit* /*killer*/) override
    {
        StopIdleChannel();
        DoScriptText(SAY_VELTHELAXX_DEATH, m_creature);
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

        if (!m_howlOfTerrorDone && m_creature->GetHealthPercent() <= 50.0f)
        {
            if (!m_halfHealthLineDone)
            {
                DoScriptText(SAY_VELTHELAXX_HALF_HEALTH, m_creature);
                m_halfHealthLineDone = true;
            }

            if (DoCastSpellIfCan(m_creature->GetVictim(), SPELL_HOWL_OF_TERROR) == CAST_OK)
            {
                m_howlOfTerrorDone = true;
                return;
            }
        }

        if (!m_enrageDone && m_creature->GetHealthPercent() <= 20.0f)
        {
            if (DoCastSpellIfCan(m_creature, SPELL_ENRAGE) == CAST_OK)
            {
                m_enrageDone = true;
                return;
            }
        }

        if (m_mindFlayTimer <= diff)
        {
            if (DoCastSpellIfCan(m_creature->GetVictim(), SPELL_MIND_FLAY) == CAST_OK)
            {
                m_mindFlayTimer = urand(16 * IN_MILLISECONDS, 19 * IN_MILLISECONDS);
                return;
            }
        }
        else
            m_mindFlayTimer -= diff;

        DoMeleeAttackIfReady();
    }

private:
    Creature* GetWoji() const
    {
        if (Creature* woji = m_creature->GetMap()->GetCreature(ObjectGuid(HIGHGUID_UNIT, NPC_WOJI_THE_TOAD, WOJI_DB_GUID)))
            return woji;

        return m_creature->FindNearestCreature(NPC_WOJI_THE_TOAD, 100.0f, true);
    }

    void UpdateIdleChannel(uint32 diff)
    {
        if (!m_allowIdleChannel)
            return;

        if (m_idleChannelActive)
        {
            if (m_creature->GetUInt32Value(UNIT_CHANNEL_SPELL) == SPELL_TARGETED_ARCANE_CHANNELING)
                return;

            m_idleChannelActive = false;
        }

        if (m_idleChannelTimer > diff)
        {
            m_idleChannelTimer -= diff;
            return;
        }

        if (Creature* woji = GetWoji())
        {
            if (StartIdleChannel(woji))
            {
                m_idleChannelTimer = 5 * IN_MILLISECONDS;
                return;
            }
        }

        m_idleChannelTimer = 1 * IN_MILLISECONDS;
    }

    bool m_allowIdleChannel;
    bool m_idleChannelActive;

    bool StartIdleChannel(Creature* woji)
    {
        m_creature->SetChannelObjectGuid(woji->GetObjectGuid());
        m_creature->SetUInt32Value(UNIT_CHANNEL_SPELL, SPELL_TARGETED_ARCANE_CHANNELING);

        if (!woji->HasAura(SPELL_TARGETED_ARCANE_CHANNELING))
            woji->AddAura(SPELL_TARGETED_ARCANE_CHANNELING, 0, m_creature);

        m_idleChannelActive = true;
        return true;
    }

    void StopIdleChannel()
    {
        if (m_creature->GetUInt32Value(UNIT_CHANNEL_SPELL) == SPELL_TARGETED_ARCANE_CHANNELING)
        {
            m_creature->SetChannelObjectGuid(ObjectGuid());
            m_creature->SetUInt32Value(UNIT_CHANNEL_SPELL, 0);
            m_creature->DirectSendPublicValueUpdate({ UNIT_FIELD_CHANNEL_OBJECT, UNIT_FIELD_CHANNEL_OBJECT + 1, UNIT_CHANNEL_SPELL });
        }

        if (Creature* woji = GetWoji())
            woji->RemoveAurasDueToSpell(SPELL_TARGETED_ARCANE_CHANNELING);

        m_idleChannelActive = false;
    }

    bool m_halfHealthLineDone;
    bool m_howlOfTerrorDone;
    bool m_enrageDone;
    uint32 m_idleChannelTimer;
    uint32 m_mindFlayTimer;
};

CreatureAI* GetAI_boss_velthelaxx_the_defiler(Creature* creature)
{
    return new boss_velthelaxx_the_defilerAI(creature);
}
}

void AddSC_boss_velthelaxx_the_defiler()
{
    Script* script = new Script;
    script->Name = "boss_velthelaxx_the_defiler";
    script->GetAI = &GetAI_boss_velthelaxx_the_defiler;
    script->RegisterSelf();
}
