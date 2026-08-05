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

void ApplyStingingNettle(Spell* spell);

struct spell_hunter_wyvern_sting : public SpellScript
{
    void OnSetTargetMap(Spell* /*spell*/, SpellEffectIndex /*effIdx*/, uint32& /*targetMode*/, float& /*radius*/, uint32& /*unMaxTargets*/, bool& selectClosestTargets) const override
    {
        selectClosestTargets = true;
    }
};

struct spell_hunter_refocus : public SpellScript
{
    bool OnEffectExecute(Spell* spell, SpellEffectIndex /*effIdx*/) const override
    {
        Player* player = spell->m_caster->ToPlayer();
        if (!player)
            return false;

        SpellCooldowns cooldowns = player->GetSpellCooldownMap();
        for (const auto& cooldown : cooldowns)
        {
            SpellEntry const* spellInfo = sSpellMgr.GetSpellEntry(cooldown.first);
            if (spellInfo && spellInfo->IsFitToFamily<SPELLFAMILY_HUNTER, CF_HUNTER_ARCANE_SHOT, CF_HUNTER_MULTI_SHOT, CF_HUNTER_VOLLEY, CF_HUNTER_AIMED_SHOT>() &&
                    spellInfo->Id != spell->m_spellInfo->Id && spellInfo->GetRecoveryTime() > 0)
                player->RemoveSpellCooldown(cooldown.first, true);
        }

        return false;
    }
};

struct spell_hunter_readiness : public SpellScript
{
    bool OnEffectExecute(Spell* spell, SpellEffectIndex effIdx) const override
    {
        if (effIdx != EFFECT_INDEX_0)
            return true;

        Player* player = spell->m_caster->ToPlayer();
        if (!player)
            return false;

        SpellCooldowns cooldowns = player->GetSpellCooldownMap();
        for (auto itr = cooldowns.begin(); itr != cooldowns.end();)
        {
            SpellEntry const* spellInfo = sSpellMgr.GetSpellEntry(itr->first);
            if (spellInfo && spellInfo->SpellFamilyName == SPELLFAMILY_HUNTER &&
                    spellInfo->Id != spell->m_spellInfo->Id && spellInfo->GetRecoveryTime() > 0)
                player->RemoveSpellCooldown((itr++)->first, true);
            else
                ++itr;
        }

        return false;
    }
};

// Deprecated in patch 1.17.2
struct spell_hunter_counterattack : public SpellScript
{
    SpellCastResult OnCheckCast(Spell* spell, bool /*strict*/) const override
    {
        if (!spell->m_casterUnit || spell->m_targets.getUnitTargetGuid() == spell->m_casterUnit->GetReactiveTarget(REACTIVE_HUNTER_PARRY))
            return SPELL_CAST_OK;

        return SPELL_FAILED_BAD_TARGETS;
    }
};

struct spell_hunter_mongoose_bite : public SpellScript
{
    void OnAfterHit(Spell* spell) const override
    {
        ApplyStingingNettle(spell);
    }

    void OnEffectDamageCalculate(Spell* spell, SpellEffectIndex /*effIdx*/, float& damage) const override
    {
        Unit* target = spell->GetUnitTarget();
        if (!spell->m_casterUnit || !target || damage <= 0.0f)
            return;

        float weaponDamagePercentMod = 1.0f;
        bool normalized = false;
        for (int j = 0; j < MAX_EFFECT_INDEX; ++j)
        {
            switch (spell->m_spellInfo->Effect[j])
            {
                case SPELL_EFFECT_NORMALIZED_WEAPON_DMG:
                    normalized = true;
                    break;
                case SPELL_EFFECT_WEAPON_PERCENT_DAMAGE:
                    weaponDamagePercentMod *= float(spell->CalculateDamage(SpellEffectIndex(j), target)) / 100.0f;
                    break;
                default:
                    break;
            }
        }

        if (weaponDamagePercentMod <= 0.0f || !spell->m_casterUnit->HaveOffhandWeapon() || spell->m_casterUnit->GetWeaponDamageCount(OFF_ATTACK) == 0)
            return;

        for (uint8 i = 0; i < spell->m_casterUnit->GetWeaponDamageCount(OFF_ATTACK); ++i)
        {
            if (target->IsImmuneToDamage(GetSchoolMask(spell->m_casterUnit->GetWeaponDamageSchool(OFF_ATTACK, i))))
                continue;

            damage += int32(spell->m_casterUnit->CalculateDamage(OFF_ATTACK, normalized, i) * weaponDamagePercentMod);
        }
    }
};

struct spell_hunter_stinging_nettle_fire_trap : public SpellScript
{
    void OnAfterHit(Spell* spell) const override
    {
        ApplyStingingNettle(spell);
    }
};

struct spell_hunter_improved_mend_pet : public AuraScript
{
    std::optional<SpellAuraProcResult> OnProc(Unit* owner, Unit* victim, uint32 /*damage*/, int32 /*originalAmount*/, Aura* aura, SpellEntry const* /*procSpell*/, uint32 /*procFlag*/, uint32 /*procEx*/, uint32 /*cooldown*/) override
    {
        if (!victim || !victim->IsAlive())
            return SPELL_AURA_PROC_FAILED;

        if (!roll_chance_i(aura->GetModifier()->m_amount))
            return SPELL_AURA_PROC_FAILED;

        owner->CastSpell(victim, 24406, true, nullptr, aura);
        return SPELL_AURA_PROC_OK;
    }
};

void ApplyStingingNettle(Spell* spell)
{
    Unit* target = spell->GetUnitTarget();
    Player* caster = ToPlayer(spell->GetAffectiveCaster());
    if (!target || !target->IsAlive() || !caster)
        return;

    uint32 nettleSpellId = 0;
    if (caster->HasSpell(51580))
        nettleSpellId = 51580;
    else if (caster->HasSpell(51579))
        nettleSpellId = 51579;

    if (!nettleSpellId)
        return;

    int32 durationPct = 0;
    if (SpellEntry const* nettleInfo = sSpellMgr.GetSpellEntry(nettleSpellId))
        durationPct = nettleInfo->CalculateSimpleValue(EFFECT_INDEX_0);

    if (durationPct <= 0)
        durationPct = nettleSpellId == 51580 ? 40 : 20;

    uint32 const serpentFirstRank = sSpellMgr.GetFirstSpellInChain(1978);
    uint32 serpentId = caster->HasSpell(serpentFirstRank) ? serpentFirstRank : 0;

    struct HighestSerpentWorker
    {
        Player* player;
        uint32& spellId;

        void operator()(uint32 id) const
        {
            if (player->HasSpell(id))
                spellId = id;
        }
    };

    HighestSerpentWorker worker{ caster, serpentId };
    sSpellMgr.doForHighRanks(serpentFirstRank, worker);

    SpellEntry const* serpentInfo = serpentId ? sSpellMgr.GetSpellEntry(serpentId) : nullptr;
    if (!serpentInfo)
        return;

    int32 const baseDuration = serpentInfo->CalculateDuration(caster);
    if (baseDuration <= 0)
        return;

    int32 newDuration = std::max(1, (baseDuration * durationPct) / 100);

    if (SpellAuraHolder* existing = target->GetSpellAuraHolder(serpentId))
        if (existing->GetCasterGuid() == caster->GetObjectGuid() && existing->GetAuraDuration() > newDuration)
            return;

    if (SpellAuraHolder* holder = target->AddAura(serpentId, 0, caster))
    {
        holder->SetAuraMaxDuration(newDuration);
        holder->SetAuraDuration(newDuration);
    }
}

struct spell_hunter_frost_trap_aura : public AuraScript
{
    void OnPeriodicTrigger(Aura* aura, Unit* /*caster*/, Unit* target, WorldObject* /*targetObject*/, SpellEntry const*& spellInfo) override
    {
        Unit* caster = aura->GetCaster();
        if (!caster)
        {
            spellInfo = nullptr;
            return;
        }

        caster->ProcDamageAndSpell(target, PROC_FLAG_ON_TRAP_ACTIVATION, PROC_FLAG_NONE, PROC_EX_NORMAL_HIT, 1, 1, BASE_ATTACK, aura->GetSpellProto());
        spellInfo = nullptr;
    }
};

struct spell_hunter_eyes_of_the_beast : public AuraScript
{
    void OnAfterApply(Aura* aura, bool apply) override
    {
        Unit* caster = aura->GetCaster();
        if (!caster)
            return;

        if (caster->HasSpell(19557))
        {
            if (apply)
                caster->CastSpell(caster, 45662, true);
            else
                caster->RemoveAurasDueToSpell(45662);
        }
        else if (caster->HasSpell(19558))
        {
            if (apply)
                caster->CastSpell(caster, 45663, true);
            else
                caster->RemoveAurasDueToSpell(45663);
        }
    }
};
}

void AddSC_hunter_spell_scripts()
{
    RegisterSpellScript("spell_hunter_wyvern_sting", &GetSpellScript<spell_hunter_wyvern_sting>);
    RegisterSpellScript("spell_hunter_refocus", &GetSpellScript<spell_hunter_refocus>);
    RegisterSpellScript("spell_hunter_readiness", &GetSpellScript<spell_hunter_readiness>);
    RegisterSpellScript("spell_hunter_counterattack", &GetSpellScript<spell_hunter_counterattack>);
    RegisterSpellScript("spell_hunter_mongoose_bite", &GetSpellScript<spell_hunter_mongoose_bite>);
    RegisterSpellScript("spell_hunter_stinging_nettle_fire_trap", &GetSpellScript<spell_hunter_stinging_nettle_fire_trap>);
    RegisterAuraScript("spell_hunter_improved_mend_pet", &GetAuraScript<spell_hunter_improved_mend_pet>);
    RegisterAuraScript("spell_hunter_frost_trap_aura", &GetAuraScript<spell_hunter_frost_trap_aura>);
    RegisterAuraScript("spell_hunter_eyes_of_the_beast", &GetAuraScript<spell_hunter_eyes_of_the_beast>);
}
