#include "scriptPCH.h"

namespace
{
template <class T>
SpellScript* GetSpellScript(SpellEntry const*)
{
    return new T();
}

template <class T>
AuraScript* GetAuraScript(SpellEntry const*)
{
    return new T();
}

void RegisterSpellScript(char const* name, SpellScript* (*getter)(SpellEntry const*))
{
    Script* script = new Script;
    script->Name = name;
    script->GetSpellScript = getter;
    script->RegisterSelf();
}

void RegisterAuraScript(char const* name, AuraScript* (*getter)(SpellEntry const*))
{
    Script* script = new Script;
    script->Name = name;
    script->GetAuraScript = getter;
    script->RegisterSelf();
}

struct spell_lunar_shift : public SpellScript
{
    bool OnEffectExecute(Spell* spell, SpellEffectIndex /*effIdx*/) const override
    {
        Unit* target = spell->GetUnitTarget();
        if (!spell->m_casterUnit || !target)
            return false;

        if (target->HasAura(51080))
        {
            target->RemoveAurasDueToSpell(51080);
            target->AddAura(51081, 0, spell->m_casterUnit);
        }
        else if (target->HasAura(51081))
        {
            target->RemoveAurasDueToSpell(51081);
            target->AddAura(51080, 0, spell->m_casterUnit);
        }
        else
            return false;

        spell->m_casterUnit->GetThreatManager().modifyThreatPercent(target, -100);
        spell->m_casterUnit->CastSpell(spell->m_casterUnit, 51085, true);
        return false;
    }
};

struct spell_flock_of_ravens : public SpellScript
{
    bool OnEffectExecute(Spell* spell, SpellEffectIndex effIdx) const override
    {
        if (!spell->m_casterUnit)
            return false;

        int32 count = spell->m_spellInfo->EffectBasePoints[effIdx];
        std::list<Player*> players;
        spell->m_casterUnit->GetAlivePlayerListInRange(spell->m_casterUnit, players, 100.0f);
        for (Player* player : players)
        {
            if (count-- < 0)
                break;

            float x;
            float y;
            float z;
            player->GetPosition(x, y, z);
            player->GetRandomPoint(x, y, z, 5.0f, x, y, z);
            if (Creature* raven = spell->m_casterUnit->SummonCreature(spell->m_spellInfo->EffectMiscValue[effIdx], x, y, z, player->GetOrientation(), TEMPSUMMON_TIMED_OR_CORPSE_DESPAWN, 30000))
                raven->AI()->AttackStart(player);
        }

        return false;
    }
};

struct spell_owl_gaze : public AuraScript
{
    void OnAfterApply(Aura* aura, bool apply) override
    {
        if (apply || aura->GetRemoveMode() != AURA_REMOVE_BY_EXPIRE)
            return;

        Unit* target = aura->GetTarget();
        Unit* caster = aura->GetCaster();
        if (!caster)
            return;

        if (target->HasAura(51080))
        {
            target->RemoveAurasDueToSpell(51080);
            target->AddAura(51081, 0, caster);
        }
        else if (target->HasAura(51081))
        {
            target->RemoveAurasDueToSpell(51081);
            target->AddAura(51080, 0, caster);
        }
    }
};
}

enum
{
    SPELL_BLUE_MOON_AURA = 51080,
    SPELL_RED_MOON_AURA = 51081,
    SPELL_WORGEN_DIMENSION = 51096,

    NPC_RED_OWL = 59997,
    NPC_BLUE_OWL = 59998,
};

struct boss_keeper_gnarlmoonAI : public ScriptedAI
{
    boss_keeper_gnarlmoonAI(Creature* pCreature) : ScriptedAI(pCreature)
    {
        Reset();
    }

    bool m_firstIntermission;
    bool m_secondIntermission;
    uint32 m_deadOwlCount;

    void Reset() override
    {
        m_firstIntermission = false;
        m_secondIntermission = false;
        m_deadOwlCount = 0;
    }

    void EnterCombat(Unit* pVictim) override
    {
        m_creature->MonsterYell("DEBUG: Enter Combat");

        std::list<Player*> players;
        m_creature->GetAlivePlayerListInRange(m_creature, players, 100.0f);

        uint32 count = 0;
        for (Player* pPlayer : players)
        {
            if (count++ > (players.size() / 2))
                pPlayer->AddAura(SPELL_BLUE_MOON_AURA, 0, m_creature);
            else
                pPlayer->AddAura(SPELL_RED_MOON_AURA, 0, m_creature);
        }
    }

    void BeginIntermission()
    {
        m_creature->MonsterYell("DEBUG: Begin Intermission");
        m_creature->CastSpell(m_creature, SPELL_WORGEN_DIMENSION, true);
        m_deadOwlCount = 0;
        
        if (Creature* pOwl = m_creature->SummonCreature(NPC_BLUE_OWL, -11123.232f, -1849.593f, 165.766f, 5.375f, TEMPSUMMON_TIMED_OR_CORPSE_DESPAWN, 60000))
            pOwl->SetInCombatWithZone();
        if (Creature* pOwl = m_creature->SummonCreature(NPC_BLUE_OWL, -11094.944f, -1827.316f, 165.766f, 5.375f, TEMPSUMMON_TIMED_OR_CORPSE_DESPAWN, 60000))
            pOwl->SetInCombatWithZone();
        if (Creature* pOwl = m_creature->SummonCreature(NPC_RED_OWL, -11101.063f, -1877.609f, 165.766f, 2.194f, TEMPSUMMON_TIMED_OR_CORPSE_DESPAWN, 60000))
            pOwl->SetInCombatWithZone();
        if (Creature* pOwl = m_creature->SummonCreature(NPC_RED_OWL, -11073.061f, -1855.183f, 165.765f, 2.194f, TEMPSUMMON_TIMED_OR_CORPSE_DESPAWN, 60000))
            pOwl->SetInCombatWithZone();

    }

    void EndIntermission()
    {
        m_creature->MonsterYell("DEBUG: End Intermission");
        m_creature->RemoveAurasDueToSpell(SPELL_WORGEN_DIMENSION);
    }

    void SummonedCreatureJustDied(Creature* pSummon) override
    {
        if (pSummon->GetEntry() == NPC_BLUE_OWL || pSummon->GetEntry() == NPC_RED_OWL)
        {
            m_creature->MonsterYell("DEBUG: Owl Died");

            if (++m_deadOwlCount >= 4)
                EndIntermission();
        }
    }

    void UpdateAI(const uint32 uiDiff) override
    {
        if (!m_creature->SelectHostileTarget() || !m_creature->GetVictim())
            return;

        if (!m_firstIntermission && m_creature->GetHealthPercent() < 66.6f)
        {
            m_firstIntermission = true;
            BeginIntermission();
            return;
        }

        if (!m_secondIntermission && m_creature->GetHealthPercent() < 33.3f)
        {
            m_secondIntermission = true;
            BeginIntermission();
            return;
        }

        if (!m_CreatureSpells.empty())
            UpdateSpellsList(uiDiff);

        DoMeleeAttackIfReady();
    }
};

CreatureAI* GetAI_boss_keeper_gnarlmoon(Creature* pCreature)
{
    return new boss_keeper_gnarlmoonAI(pCreature);
}

struct npc_blood_ravenAI : public ScriptedAI
{
    npc_blood_ravenAI(Creature* pCreature) : ScriptedAI(pCreature)
    {
        Reset();
    }

    void Reset() override
    {
    }

    void DamageTaken(Unit* pAttacker, uint32& damage) override
    {
        if (!pAttacker->HasAura(SPELL_BLUE_MOON_AURA))
            damage = 0;
    }
};

CreatureAI* GetAI_npc_blood_raven(Creature* pCreature)
{
    return new npc_blood_ravenAI(pCreature);
}

void AddSC_boss_keeper_gnarlmoon()
{
    Script* newscript;

    newscript = new Script;
    newscript->Name = "npc_keeper_gnarlmoon";
    newscript->GetAI = &GetAI_boss_keeper_gnarlmoon;
    newscript->RegisterSelf();

    newscript = new Script;
    newscript->Name = "npc_blood_raven";
    newscript->GetAI = &GetAI_npc_blood_raven;
    newscript->RegisterSelf();

    RegisterSpellScript("spell_lunar_shift", &GetSpellScript<spell_lunar_shift>);
    RegisterSpellScript("spell_flock_of_ravens", &GetSpellScript<spell_flock_of_ravens>);
    RegisterAuraScript("spell_owl_gaze", &GetAuraScript<spell_owl_gaze>);
}
