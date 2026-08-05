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

struct spell_priest_power_word_shield : public SpellScript
{
    void OnCast(Spell* spell) const override
    {
        if (spell->m_spellInfo->Id != 27779)
            spell->AddPrecastSpell(6788);
    }
};

struct spell_priest_holy_nova : public SpellScript
{
    void OnCast(Spell* spell) const override
    {
        switch (spell->m_spellInfo->Id)
        {
            case 15237: spell->AddTriggeredSpell(23455); break;
            case 15430: spell->AddTriggeredSpell(23458); break;
            case 15431: spell->AddTriggeredSpell(23459); break;
            case 27799: spell->AddTriggeredSpell(27803); break;
            case 27800: spell->AddTriggeredSpell(27804); break;
            case 27801: spell->AddTriggeredSpell(27805); break;
            default: break;
        }
    }
};

struct spell_priest_inspiration : public AuraScript
{
    std::optional<SpellProcEventTriggerCheck> OnCheckProc(Unit const* /*owner*/, Unit* /*victim*/, SpellAuraHolder* /*holder*/, SpellEntry const* procSpell, uint32 procFlag, uint32 procExtra, WeaponAttackType /*attType*/, bool /*isVictim*/) override
    {
        if (!procSpell)
            return SPELL_PROC_TRIGGER_FAILED;

        bool const validHeal = procSpell->IsFitToFamily<SPELLFAMILY_PRIEST, CF_PRIEST_PRAYER_OF_HEALING, CF_PRIEST_HEAL,
            CF_PRIEST_FLASH_HEAL, CF_PRIEST_GREATER_HEAL>();
        return validHeal && (procExtra & PROC_EX_CRITICAL_HIT) && (procFlag & PROC_FLAG_DEAL_HELPFUL_SPELL) ? SPELL_PROC_TRIGGER_OK : SPELL_PROC_TRIGGER_FAILED;
    }
};

struct spell_priest_vampiric_embrace : public AuraScript
{
    std::optional<SpellAuraProcResult> OnProc(Unit* /*owner*/, Unit* victim, uint32 damage, int32 /*originalAmount*/, Aura* aura, SpellEntry const* /*procSpell*/, uint32 /*procFlag*/, uint32 /*procEx*/, uint32 /*cooldown*/) override
    {
        if (!victim || !victim->IsAlive() || aura->GetCasterGuid() != victim->GetObjectGuid())
            return SPELL_AURA_PROC_FAILED;

        int32 heal = aura->GetModifier()->m_amount * damage / 100;
        victim->CastCustomSpell(victim, 15290, &heal, nullptr, nullptr, true, nullptr, aura);

        constexpr uint32 ManaGainEffectIndex = 1;
        SpellEntry const* improvedRank2Info = sSpellMgr.GetSpellEntry(45558);
        SpellEntry const* improvedRank1Info = sSpellMgr.GetSpellEntry(45557);
        int32 dieSides = improvedRank1Info ? improvedRank1Info->EffectDieSides[ManaGainEffectIndex] : 0;
        uint32 improvedVampiric = victim->HasAura(45558) && improvedRank2Info ? improvedRank2Info->EffectBasePoints[ManaGainEffectIndex] + dieSides : 0;

        if (!improvedVampiric && victim->HasAura(45557) && improvedRank1Info)
            improvedVampiric = improvedRank1Info->EffectBasePoints[ManaGainEffectIndex] + dieSides;

        if (improvedVampiric && damage)
        {
            int32 mana = improvedVampiric * damage / 100;
            if (mana)
                victim->CastCustomSpell(victim, 45966, &mana, nullptr, nullptr, true, nullptr, aura);
        }

        return SPELL_AURA_PROC_OK;
    }
};

struct spell_priest_touch_of_weakness_trigger : public SpellScript
{
    bool OnEffectExecute(Spell* spell, SpellEffectIndex effIdx) const override
    {
        if (effIdx != EFFECT_INDEX_0 || !spell->GetUnitTarget() || !spell->m_triggeredByAuraSpell)
            return false;

        uint32 spellId = 0;
        switch (spell->m_triggeredByAuraSpell->Id)
        {
            case 2652:  spellId = 2943; break;
            case 19261: spellId = 19249; break;
            case 19262: spellId = 19251; break;
            case 19264: spellId = 19252; break;
            case 19265: spellId = 19253; break;
            case 19266: spellId = 19254; break;
            default:
                sLog.outError("spell_priest_touch_of_weakness_trigger: Spell 28598 triggered by unhandled spell %u", spell->m_triggeredByAuraSpell->Id);
                return false;
        }

        spell->m_caster->CastSpell(spell->GetUnitTarget(), spellId, true, nullptr);
        return false;
    }
};

struct spell_priest_pain_spike : public SpellScript
{
    bool OnEffectExecute(Spell* spell, SpellEffectIndex effIdx) const override
    {
        if (effIdx != EFFECT_INDEX_1)
            return true;

        Unit* target = spell->GetUnitTarget();
        if (!target)
            return false;

        int32 healAmount = spell->GetTotalEffectDamage() / 5;
        spell->m_caster->CastCustomSpell(target, 45556, &healAmount, nullptr, nullptr, true);
        return false;
    }
};

struct spell_priest_enlighten : public AuraScript
{
    std::optional<SpellAuraProcResult> OnProc(Unit* owner, Unit* /*victim*/, uint32 /*damage*/, int32 /*originalAmount*/, Aura* aura, SpellEntry const* /*procSpell*/, uint32 /*procFlag*/, uint32 /*procEx*/, uint32 /*cooldown*/) override
    {
        Unit* tutored = aura->GetCaster();
        if (!tutored)
            tutored = owner;

        float percent = 0.0f;
        if (SpellEntry const* dmgSpell = sSpellMgr.GetSpellEntry(51474))
            percent = (dmgSpell->EffectBasePoints[EFFECT_INDEX_0] + 1) / 100.0f;

        if (percent <= 0.0f)
            percent = 0.50f;

        int32 damageAmount = std::max<int32>(1, static_cast<int32>(tutored->GetMaxHealth() * percent));
        owner->SpellNonMeleeDamageLog(tutored, 51474, damageAmount);

        if (tutored->GetObjectGuid() == owner->GetObjectGuid())
            owner->CastSpell(owner, 51473, true, nullptr, aura);
        else
        {
            owner->CastSpell(owner, 51472, true, nullptr, aura);
            owner->CastSpell(tutored, 51472, true, nullptr, aura);
        }

        return SPELL_AURA_PROC_OK;
    }
};

struct spell_priest_enlighten_link : public AuraScript
{
    void OnAfterApply(Aura* aura, bool apply) override
    {
        Unit* caster = aura->GetCaster();
        if (!caster)
            return;

        if (apply)
        {
            aura->GetTarget()->CastSpell(caster, 51475, true, nullptr, nullptr, aura->GetTarget()->GetObjectGuid());
            return;
        }

        caster->RemoveAurasDueToSpell(51475);
    }
};

struct spell_priest_shadowguard : public AuraScript
{
    std::optional<SpellAuraProcResult> OnProc(Unit* owner, Unit* victim, uint32 /*damage*/, int32 /*originalAmount*/, Aura* aura, SpellEntry const* /*procSpell*/, uint32 /*procFlag*/, uint32 /*procEx*/, uint32 /*cooldown*/) override
    {
        if (!victim)
            return SPELL_AURA_PROC_FAILED;

        uint32 spellId = 0;
        switch (aura->GetId())
        {
            case 18137: spellId = 28377; break;
            case 19308: spellId = 28378; break;
            case 19309: spellId = 28379; break;
            case 19310: spellId = 28380; break;
            case 19311: spellId = 28381; break;
            case 19312: spellId = 28382; break;
        }

        if (!spellId)
            return SPELL_AURA_PROC_FAILED;

        owner->CastSpell(victim, spellId, true, nullptr, aura);
        return SPELL_AURA_PROC_OK;
    }
};

struct spell_priest_blessed_recovery : public AuraScript
{
    std::optional<SpellAuraProcResult> OnProc(Unit* owner, Unit* /*victim*/, uint32 damage, int32 /*originalAmount*/, Aura* aura, SpellEntry const* /*procSpell*/, uint32 /*procFlag*/, uint32 /*procEx*/, uint32 /*cooldown*/) override
    {
        uint32 spellId = aura->GetId() == 27811 ? 27813 : aura->GetId() == 27815 ? 27817 : 27818;
        int32 heal = damage * aura->GetModifier()->m_amount / 100 / 3;
        owner->CastCustomSpell(owner, spellId, &heal, nullptr, nullptr, true, nullptr, aura);
        return SPELL_AURA_PROC_OK;
    }
};

// Deprecated in patch 1.17.2
struct spell_priest_elunes_grace : public AuraScript
{
    std::optional<SpellAuraProcResult> OnProc(Unit* owner, Unit* /*victim*/, uint32 /*damage*/, int32 /*originalAmount*/, Aura* aura, SpellEntry const* /*procSpell*/, uint32 /*procFlag*/, uint32 /*procEx*/, uint32 /*cooldown*/) override
    {
        int32 mana = owner->GetStat(STAT_AGILITY);
        owner->CastCustomSpell(owner, aura->GetSpellProto()->EffectTriggerSpell[aura->GetEffIndex()], &mana, nullptr, nullptr, true);
        return SPELL_AURA_PROC_OK;
    }
};

// Deprecated in patch 1.18.1
struct spell_champion_buff : public SpellScript
{
    SpellCastResult OnCheckCast(Spell* spell, bool /*strict*/) const override
    {
        Player* player = ToPlayer(spell->GetAffectiveCaster());
        if (!player)
            return SPELL_CAST_OK;

        if (player->GetChampionGUID().IsEmpty())
            return SPELL_FAILED_NO_CHAMPION;

        Unit* target = spell->m_targets.getUnitTarget();
        if (!target || player->GetChampionGUID() == target->GetObjectGuid())
            return SPELL_CAST_OK;

        if (Unit* champion = ObjectAccessor::GetUnit(*player, player->GetChampionGUID()))
            spell->m_targets.setUnitTarget(champion);

        return SPELL_CAST_OK;
    }
};

// Deprecated in patch 1.18.1
struct spell_proclaim_champion : public AuraScript
{
    void OnAfterApply(Aura* aura, bool apply) override
    {
        Unit* target = aura->GetTarget();
        Unit* caster = aura->GetCaster();
        if (!caster || !caster->IsPlayer() || !target || target->GetGUID() == caster->GetGUID() || !target->IsAlive())
        {
            if (apply && target)
            {
                RemoveChampionBuffs(target);
                target->RemoveAurasDueToSpell(45568);
            }
            return;
        }

        Player* playerCaster = caster->ToPlayer();
        if (apply)
        {
            playerCaster->SetChampion(target->GetGUID());
            return;
        }

        RemoveChampionBuffs(target);
        playerCaster->SetChampion(ObjectGuid{});
    }

private:
    static void RemoveChampionBuffs(Unit* target)
    {
        target->RemoveAurasDueToSpell(45563);
        target->RemoveAurasDueToSpell(45564);
        target->RemoveAurasDueToSpell(45565);
        target->RemoveAurasDueToSpell(45569);
    }
};

}

void AddSC_priest_spell_scripts()
{
    RegisterSpellScript("spell_priest_power_word_shield", &GetSpellScript<spell_priest_power_word_shield>);
    RegisterSpellScript("spell_priest_holy_nova", &GetSpellScript<spell_priest_holy_nova>);
    RegisterAuraScript("spell_priest_inspiration", &GetAuraScript<spell_priest_inspiration>);
    RegisterAuraScript("spell_priest_vampiric_embrace", &GetAuraScript<spell_priest_vampiric_embrace>);
    RegisterSpellScript("spell_priest_touch_of_weakness_trigger", &GetSpellScript<spell_priest_touch_of_weakness_trigger>);
    RegisterSpellScript("spell_priest_pain_spike", &GetSpellScript<spell_priest_pain_spike>);
    RegisterAuraScript("spell_priest_enlighten", &GetAuraScript<spell_priest_enlighten>);
    RegisterAuraScript("spell_priest_enlighten_link", &GetAuraScript<spell_priest_enlighten_link>);
    RegisterAuraScript("spell_priest_shadowguard", &GetAuraScript<spell_priest_shadowguard>);
    RegisterAuraScript("spell_priest_blessed_recovery", &GetAuraScript<spell_priest_blessed_recovery>);
    RegisterAuraScript("spell_priest_elunes_grace", &GetAuraScript<spell_priest_elunes_grace>);
    RegisterSpellScript("spell_champion_buff", &GetSpellScript<spell_champion_buff>);
    RegisterAuraScript("spell_proclaim_champion", &GetAuraScript<spell_proclaim_champion>);
}
