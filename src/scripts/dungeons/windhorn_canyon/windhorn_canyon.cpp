#include "scriptPCH.h"
#include "windhorn_canyon.h"

namespace
{
struct boss_chieftain_shalk_blackwindAI : public ScriptedAI
{
    explicit boss_chieftain_shalk_blackwindAI(Creature* creature) :
        ScriptedAI(creature),
        m_pollForFlameAura(false)
    {
        Reset();
    }

    void Reset() override
    {
        m_summonedBloodguard = false;
        m_underwaterCheckTimer = 250;
    }

    void Aggro(Unit* /*who*/) override
    {
        DoScriptText(SAY_SHALK_AGGRO, m_creature);
    }

    void JustDied(Unit* /*killer*/) override
    {
        DoScriptText(SAY_SHALK_DEATH, m_creature);
    }

    void UpdateAI(uint32 diff) override
    {
        if (!m_summonedBloodguard && m_creature->IsInCombat() && m_creature->GetHealthPercent() <= 50.0f)
        {
            DoScriptText(SAY_SHALK_HALF_HEALTH, m_creature);

            if (Creature* guard = m_creature->SummonCreature(
                    NPC_BLACKWIND_BLOODGUARD,
                    -7546.968750f, -3762.458008f, 282.898224f, 5.498519f,
                    TEMPSUMMON_TIMED_OR_DEAD_DESPAWN, 300000, true))
            {
                DoScriptText(SAY_SHALK_BLOODGUARD_SPAWN, guard);

                if (Unit* target = m_creature->SelectAttackingTarget(ATTACKING_TARGET_RANDOM, 0))
                    guard->AI()->AttackStart(target);
            }

            m_summonedBloodguard = true;
        }

        if (m_creature->IsInCombat())
            m_pollForFlameAura = true;

        if (m_pollForFlameAura && m_underwaterCheckTimer <= diff)
        {
            m_pollForFlameAura = false;

            Map::PlayerList const& players = m_creature->GetMap()->GetPlayers();
            for (const auto& itr : players)
            {
                Player* player = itr.getSource();
                if (!player || !player->HasAura(SPELL_FLAME_OF_SHALK))
                    continue;

                if (player->IsUnderwater())
                    player->RemoveAurasDueToSpell(SPELL_FLAME_OF_SHALK);
                else
                    m_pollForFlameAura = true;
            }

            m_underwaterCheckTimer = 500;
        }
        else if (m_pollForFlameAura)
            m_underwaterCheckTimer -= diff;

        ScriptedAI::UpdateAI(diff);
    }

private:
    bool m_summonedBloodguard;
    bool m_pollForFlameAura;
    uint32 m_underwaterCheckTimer;
};

struct npc_windhorn_storm_guardianAI : public ScriptedAI
{
    explicit npc_windhorn_storm_guardianAI(Creature* creature) : ScriptedAI(creature) {}

    void Reset() override {}

    void JustDied(Unit* killer) override
    {
        Unit* target = m_creature->SelectAttackingTarget(ATTACKING_TARGET_RANDOM, 0);
        if (!target && killer && killer->IsAlive())
            target = killer;

        static constexpr float offsets[3][2] =
        {
            { 1.50f,  0.00f },
            {-0.75f,  1.30f },
            {-0.75f, -1.30f },
        };

        for (const auto& offset : offsets)
        {
            if (Creature* residue = m_creature->SummonCreature(
                    NPC_STORM_RESIDUE,
                    m_creature->GetPositionX() + offset[0],
                    m_creature->GetPositionY() + offset[1],
                    m_creature->GetPositionZ(),
                    m_creature->GetOrientation(),
                    TEMPSUMMON_TIMED_OR_DEAD_DESPAWN, 300000, true))
            {
                if (target)
                    residue->AI()->AttackStart(target);
            }
        }
    }
};

CreatureAI* GetAI_boss_chieftain_shalk_blackwind(Creature* creature)
{
    return new boss_chieftain_shalk_blackwindAI(creature);
}

CreatureAI* GetAI_npc_windhorn_storm_guardian(Creature* creature)
{
    return new npc_windhorn_storm_guardianAI(creature);
}
}

void AddSC_windhorn_canyon()
{
    Script* script = new Script;
    script->Name = "boss_chieftain_shalk_blackwind";
    script->GetAI = &GetAI_boss_chieftain_shalk_blackwind;
    script->RegisterSelf();

    script = new Script;
    script->Name = "npc_windhorn_storm_guardian";
    script->GetAI = &GetAI_npc_windhorn_storm_guardian;
    script->RegisterSelf();
}
