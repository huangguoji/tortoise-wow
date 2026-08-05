#include "scriptPCH.h"

namespace
{
struct ResonanceCascadeProcData
{
    uint32 count = 0;
    time_t lastProc = 0;
};

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

struct spell_mage_arcane_missiles : public SpellScript
{
    SpellCastResult OnCheckCast(Spell* spell, bool /*strict*/) const override
    {
        return spell->m_targets.getUnitTarget() == spell->m_caster ? SPELL_FAILED_BAD_TARGETS : SPELL_CAST_OK;
    }
};

struct spell_mage_cold_snap : public SpellScript
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
            if (spellInfo && spellInfo->SpellFamilyName == SPELLFAMILY_MAGE &&
                    (spellInfo->GetSpellSchoolMask() & SPELL_SCHOOL_MASK_FROST) &&
                    spellInfo->Id != spell->m_spellInfo->Id && spellInfo->GetRecoveryTime() > 0)
                player->RemoveSpellCooldown((itr++)->first, true);
            else
                ++itr;
        }

        return false;
    }
};

struct spell_mage_magic_absorption : public AuraScript
{
    std::optional<SpellAuraProcResult> OnProc(Unit* owner, Unit* /*victim*/, uint32 /*damage*/, int32 /*originalAmount*/, Aura* aura, SpellEntry const* /*procSpell*/, uint32 /*procFlag*/, uint32 /*procEx*/, uint32 /*cooldown*/) override
    {
        if (owner->GetPowerType() != POWER_MANA)
            return SPELL_AURA_PROC_FAILED;

        int32 mana = aura->GetModifier()->m_amount * owner->GetMaxPower(POWER_MANA) / 100;
        owner->CastCustomSpell(owner, 29442, &mana, nullptr, nullptr, true, nullptr, aura);
        return SPELL_AURA_PROC_OK;
    }
};

struct spell_mage_master_of_elements : public AuraScript
{
    std::optional<SpellAuraProcResult> OnProc(Unit* owner, Unit* /*victim*/, uint32 /*damage*/, int32 /*originalAmount*/, Aura* aura, SpellEntry const* procSpell, uint32 /*procFlag*/, uint32 /*procEx*/, uint32 /*cooldown*/) override
    {
        if (!procSpell)
            return SPELL_AURA_PROC_FAILED;

        int32 cost = procSpell->manaCost + procSpell->ManaCostPercentage * owner->GetCreateMana() / 100;
        int32 mana = cost * aura->GetModifier()->m_amount / 100;
        if (mana <= 0)
            return SPELL_AURA_PROC_FAILED;

        owner->CastCustomSpell(owner, 29077, &mana, nullptr, nullptr, true, nullptr, aura);
        return SPELL_AURA_PROC_OK;
    }
};

struct spell_mage_ignite : public AuraScript
{
    std::optional<SpellAuraProcResult> OnProc(Unit* owner, Unit* victim, uint32 damage, int32 /*originalAmount*/, Aura* aura, SpellEntry const* /*procSpell*/, uint32 /*procFlag*/, uint32 /*procEx*/, uint32 /*cooldown*/) override
    {
        if (!victim)
            return SPELL_AURA_PROC_FAILED;

        uint32 totalDamage = damage;
        if (Spell* spell = owner->GetCurrentSpell(CURRENT_GENERIC_SPELL))
            totalDamage += spell->GetAbsorbedDamage();

        float pct = 0.0f;
        switch (aura->GetId())
        {
            case 11119: pct = 0.04f; break;
            case 11120: pct = 0.08f; break;
            case 12846: pct = 0.12f; break;
            case 12847: pct = 0.16f; break;
            case 12848: pct = 0.20f; break;
        }

        int32 basepoints = int32(pct * totalDamage);
        if (Aura* igniteAura = victim ? victim->GetAura(12654, EFFECT_INDEX_0) : nullptr)
        {
            Modifier* igniteModifier = igniteAura->GetModifier();
            SpellAuraHolder* igniteHolder = igniteAura->GetHolder();
            int32 tickDamage = igniteModifier->m_amount;

            if (igniteAura->GetStackAmount() < 5)
            {
                tickDamage += basepoints;
                igniteHolder->ModStackAmount(1);
                igniteModifier->m_amount = tickDamage;
                igniteAura->ApplyModifier(true, true, false);
            }
            else
                igniteHolder->SetStackAmount(5);

            igniteHolder->Refresh(igniteAura->GetCaster(), victim, igniteHolder);
            return SPELL_AURA_PROC_OK;
        }

        owner->CastCustomSpell(victim, 12654, &basepoints, nullptr, nullptr, true, nullptr, aura);
        return SPELL_AURA_PROC_OK;
    }
};

struct spell_mage_combustion : public AuraScript
{
    std::optional<SpellAuraProcResult> OnProc(Unit* owner, Unit* victim, uint32 /*damage*/, int32 /*originalAmount*/, Aura* aura, SpellEntry const* /*procSpell*/, uint32 /*procFlag*/, uint32 procEx, uint32 /*cooldown*/) override
    {
        if (!victim)
            return SPELL_AURA_PROC_FAILED;

        if (!owner->HasAura(28682))
        {
            owner->RemoveAurasDueToSpell(11129);
            return SPELL_AURA_PROC_FAILED;
        }

        if (aura->GetHolder()->GetAuraCharges() <= 1 && (procEx & PROC_EX_CRITICAL_HIT))
        {
            owner->RemoveAurasDueToSpell(28682);
            return SPELL_AURA_PROC_OK;
        }

        owner->CastSpell(owner, 28682, true, nullptr, aura);
        return (procEx & PROC_EX_CRITICAL_HIT) ? SPELL_AURA_PROC_OK : SPELL_AURA_PROC_FAILED;
    }
};

struct spell_mage_resonance_cascade : public AuraScript
{
    std::optional<SpellAuraProcResult> OnProc(Unit* owner, Unit* victim, uint32 damage, int32 /*originalAmount*/, Aura* aura, SpellEntry const* procSpell, uint32 /*procFlag*/, uint32 /*procEx*/, uint32 /*cooldown*/) override
    {
        if (!damage || !procSpell || !(procSpell->GetSpellSchoolMask() & SPELL_SCHOOL_MASK_ARCANE) || !victim || !victim->IsAlive())
            return SPELL_AURA_PROC_FAILED;

        int32 duplicatedDamage = int32(damage * (float(aura->GetModifier()->m_amount) / 100.0f));
        if (!duplicatedDamage)
            return SPELL_AURA_PROC_FAILED;

        static std::map<ObjectGuid, ResonanceCascadeProcData> consecutiveProcsMap;
        ResonanceCascadeProcData& procData = consecutiveProcsMap[owner->GetObjectGuid()];
        if ((procData.lastProc + 3) < sWorld.GetGameTime())
            procData.count = 0;

        if (procData.count >= 6)
            return SPELL_AURA_PROC_FAILED;

        procData.lastProc = sWorld.GetGameTime();
        ++procData.count;
        owner->CastCustomSpell(victim, procSpell, &duplicatedDamage, nullptr, nullptr, true);
        return SPELL_AURA_PROC_OK;
    }
};

struct spell_mage_arcane_instability : public AuraScript
{
    std::optional<SpellAuraProcResult> OnProc(Unit* owner, Unit* victim, uint32 damage, int32 /*originalAmount*/, Aura* aura, SpellEntry const* procSpell, uint32 /*procFlag*/, uint32 /*procEx*/, uint32 /*cooldown*/) override
    {
        if (!procSpell || !(procSpell->GetSpellSchoolMask() & SPELL_SCHOOL_MASK_ARCANE) || !victim || !victim->IsAlive() || !damage)
            return SPELL_AURA_PROC_FAILED;

        Player* player = owner->ToPlayer();
        if (!player)
            return SPELL_AURA_PROC_FAILED;

        uint32 manaCost = player->GetCreateMana() ? uint32(player->GetCreateMana() * 2 / 100) : 0;
        if (manaCost && player->GetPower(POWER_MANA) < manaCost)
            return SPELL_AURA_PROC_FAILED;

        int32 bonusDamage = std::max(1u, aura->GetModifier()->m_amount * damage / 100);
        if (manaCost)
            player->ModifyPower(POWER_MANA, -int32(manaCost));

        owner->CastCustomSpell(victim, aura->GetSpellProto()->EffectTriggerSpell[aura->GetEffIndex()], &bonusDamage, nullptr, nullptr, true, nullptr, aura);
        return SPELL_AURA_PROC_OK;
    }
};

struct spell_mage_improved_blizzard : public AuraScript
{
    std::optional<SpellAuraProcResult> OnProc(Unit* owner, Unit* victim, uint32 /*damage*/, int32 /*originalAmount*/, Aura* aura, SpellEntry const* procSpell, uint32 /*procFlag*/, uint32 /*procEx*/, uint32 /*cooldown*/) override
    {
        if (!victim)
            return SPELL_AURA_PROC_FAILED;

        uint32 triggerSpellId = 0;
        switch (aura->GetModifier()->m_miscvalue)
        {
            case 836: triggerSpellId = 12484; break;
            case 988: triggerSpellId = 12485; break;
            case 989: triggerSpellId = 12486; break;
        }

        if (!triggerSpellId)
            return std::nullopt;

        if (!procSpell || procSpell->SpellVisual != 259)
            return SPELL_AURA_PROC_FAILED;

        owner->CastSpell(victim, triggerSpellId, true, nullptr, aura);
        return SPELL_AURA_PROC_OK;
    }
};

struct spell_mage_clear_resist_state : public SpellScript
{
    void OnSuccessfulFinish(Spell* spell) const override
    {
        if (spell->m_casterUnit && spell->m_casterUnit->IsPlayer())
            spell->m_casterUnit->ModifyAuraState(AURA_STATE_SPELL_RESISTED, false);
    }
};

struct spell_mage_arcane_rupture : public SpellScript
{
    void OnFinish(Spell* spell, bool ok) const override
    {
        if (!ok || !spell->m_casterUnit || !spell->m_casterUnit->IsPlayer() || !spell->m_casterUnit->HasAura(51961))
            return;

        if (spell->m_spellInfo->powerType == POWER_MANA && spell->GetPowerCost() > 0)
            spell->m_casterUnit->ModifyPower(POWER_MANA, static_cast<int32>(spell->GetPowerCost()));

        spell->m_casterUnit->RemoveAurasDueToSpell(51961);
    }

    void OnSuccessfulFinish(Spell* spell) const override
    {
        if (spell->m_casterUnit && spell->m_casterUnit->IsPlayer())
            spell->m_casterUnit->CastSpell(spell->m_casterUnit, 52502, true);
    }
};

struct spell_mage_arcane_rupture_buff : public AuraScript
{
    void OnAfterApply(Aura* aura, bool apply) override
    {
        if (!apply)
            return;

        SpellModifier* spellMod = aura->GetSpellModifier();
        if (!spellMod)
            return;

        spellMod->mask = UI64LIT(1) << CF_MAGE_ARCANE_MISSILES;

        SpellModifier* damageMod = new SpellModifier(
            SPELLMOD_DAMAGE,
            SpellModType(aura->GetModifier()->m_auraname),
            aura->GetModifier()->m_amount,
            aura->GetId(),
            UI64LIT(1) << CF_MAGE_ARCANE_MISSILES,
            aura->GetHolder()->GetAuraCharges());

        aura->AddExtraSpellModifier(damageMod);
        aura->GetTarget()->ToPlayer()->AddSpellMod(damageMod, true);
    }
};

struct spell_mage_brilliance_aura : public AuraScript
{
    void OnAfterApply(Aura* aura, bool apply) override
    {
        if (!apply || aura->GetModifier()->m_miscvalue != POWER_MANA)
            return;

        UpdateAmount(aura, true);
    }

    void OnPeriodicTick(Aura* aura) override
    {
        if (aura->GetModifier()->m_auraname != SPELL_AURA_MOD_POWER_REGEN ||
                aura->GetModifier()->m_miscvalue != POWER_MANA)
            return;

        UpdateAmount(aura, false);
    }

private:
    static void UpdateAmount(Aura* aura, bool applying)
    {
        Unit* target = aura->GetTarget();
        Player* caster = ToPlayer(aura->GetCaster());
        if (!target || !caster)
            return;

        float percent = aura->GetSpellProto()->CalculateSimpleValue(aura->GetEffIndex());
        if (!applying && caster->GetObjectGuid() != target->GetObjectGuid())
            percent -= std::min(percent, (percent / 3.0f) * (int32(caster->GetDistance2d(target)) / 10));

        float const multiplier = (percent / 100.0f) * 5.0f;
        aura->GetModifier()->m_amount = (caster != target ? caster->GetManaRegen() : caster->GetRegenMPPerSpirit()) * multiplier;
        target->UpdateManaRegen();
    }
};
}

void AddSC_mage_spell_scripts()
{
    RegisterSpellScript("spell_mage_arcane_missiles", &GetSpellScript<spell_mage_arcane_missiles>);
    RegisterSpellScript("spell_mage_cold_snap", &GetSpellScript<spell_mage_cold_snap>);
    RegisterAuraScript("spell_mage_magic_absorption", &GetAuraScript<spell_mage_magic_absorption>);
    RegisterAuraScript("spell_mage_master_of_elements", &GetAuraScript<spell_mage_master_of_elements>);
    RegisterAuraScript("spell_mage_ignite", &GetAuraScript<spell_mage_ignite>);
    RegisterAuraScript("spell_mage_combustion", &GetAuraScript<spell_mage_combustion>);
    RegisterAuraScript("spell_mage_resonance_cascade", &GetAuraScript<spell_mage_resonance_cascade>);
    RegisterAuraScript("spell_mage_arcane_instability", &GetAuraScript<spell_mage_arcane_instability>);
    RegisterAuraScript("spell_mage_improved_blizzard", &GetAuraScript<spell_mage_improved_blizzard>);
    RegisterSpellScript("spell_mage_clear_resist_state", &GetSpellScript<spell_mage_clear_resist_state>);
    RegisterSpellScript("spell_mage_arcane_rupture", &GetSpellScript<spell_mage_arcane_rupture>);
    RegisterAuraScript("spell_mage_arcane_rupture_buff", &GetAuraScript<spell_mage_arcane_rupture_buff>);
    RegisterAuraScript("spell_mage_brilliance_aura", &GetAuraScript<spell_mage_brilliance_aura>);
}
