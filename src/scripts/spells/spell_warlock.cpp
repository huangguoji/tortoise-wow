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

bool HasCasterImmolate(Unit* target, WorldObject* caster)
{
    if (!target || !caster)
        return false;

    Unit::AuraList const& periodicDamage = target->GetAurasByType(SPELL_AURA_PERIODIC_DAMAGE);
    for (Aura const* aura : periodicDamage)
    {
        if (aura->GetSpellProto()->IsFitToFamily<SPELLFAMILY_WARLOCK, CF_WARLOCK_IMMOLATE>() &&
                aura->GetCasterGuid() == caster->GetObjectGuid())
            return true;
    }

    return false;
}

struct spell_warlock_fire_shield : public SpellScript
{
    SpellCastResult OnCheckCast(Spell* spell, bool /*strict*/) const override
    {
        return spell->m_targets.getUnitTarget() == spell->m_caster ? SPELL_FAILED_BAD_TARGETS : SPELL_CAST_OK;
    }
};

struct spell_warlock_conflagrate : public SpellScript
{
    SpellCastResult OnCheckCast(Spell* spell, bool /*strict*/) const override
    {
        Unit* target = spell->m_targets.getUnitTarget();
        if (!target)
            return SPELL_FAILED_BAD_IMPLICIT_TARGETS;

        return HasCasterImmolate(target, spell->m_caster) ? SPELL_CAST_OK : SPELL_FAILED_TARGET_AURASTATE;
    }

    void OnEffectDamageCalculate(Spell* spell, SpellEffectIndex /*effIdx*/, float& /*damage*/) const override
    {
        Unit* target = spell->GetUnitTarget();
        if (!target)
            return;

        Unit::AuraList const& periodicDamage = target->GetAurasByType(SPELL_AURA_PERIODIC_DAMAGE);
        for (Aura const* aura : periodicDamage)
        {
            if (aura->GetSpellProto()->IsFitToFamily<SPELLFAMILY_WARLOCK, CF_WARLOCK_IMMOLATE>() &&
                    aura->GetCasterGuid() == spell->m_caster->GetObjectGuid())
            {
                target->RemoveAurasByCasterSpell(aura->GetId(), spell->m_caster->GetObjectGuid());
                break;
            }
        }
    }
};

struct spell_warlock_life_tap : public SpellScript
{
    SpellCastResult OnCheckCast(Spell* spell, bool /*strict*/) const override
    {
        if (!spell->m_casterUnit)
            return SPELL_CAST_OK;

        float cost = spell->m_currentBasePoints[EFFECT_INDEX_0];
        if (Player* modOwner = spell->m_casterUnit->GetSpellModOwner())
            modOwner->ApplySpellMod(spell->m_spellInfo->Id, SPELLMOD_COST, cost, spell);

        int32 dmg = spell->m_casterUnit->SpellDamageBonusDone(spell->m_casterUnit, spell->m_spellInfo, EFFECT_INDEX_0, uint32(cost > 0 ? cost : 0), SPELL_DIRECT_DAMAGE);
        dmg = spell->m_casterUnit->SpellDamageBonusTaken(spell->m_casterUnit, spell->m_spellInfo, EFFECT_INDEX_0, dmg, SPELL_DIRECT_DAMAGE);

        return int32(spell->m_casterUnit->GetHealth()) <= dmg ? SPELL_FAILED_FIZZLE : SPELL_CAST_OK;
    }

    bool OnEffectExecute(Spell* spell, SpellEffectIndex effIdx) const override
    {
        if (effIdx != EFFECT_INDEX_0 || !spell->m_casterUnit)
            return false;

        int32 dmg = spell->m_casterUnit->CalculateSpellDamage(spell->m_casterUnit, spell->m_spellInfo, effIdx, &spell->m_currentBasePoints[EFFECT_INDEX_0]);
        int32 oldDamage = dmg;
        if (Player* modOwner = spell->m_casterUnit->GetSpellModOwner())
            modOwner->ApplySpellMod(spell->m_spellInfo->Id, SPELLMOD_COST, dmg, spell);

        int32 spellModDmg = dmg;
        dmg = spell->m_casterUnit->SpellDamageBonusDone(spell->m_casterUnit, spell->m_spellInfo, effIdx, uint32(dmg > 0 ? dmg : 0), SPELL_DIRECT_DAMAGE);
        dmg = spell->m_casterUnit->SpellDamageBonusTaken(spell->m_casterUnit, spell->m_spellInfo, effIdx, dmg, SPELL_DIRECT_DAMAGE);

        if (int32(spell->m_casterUnit->GetHealth()) <= dmg)
        {
            spell->SendCastResult(SPELL_FAILED_FIZZLE);
            return false;
        }

        spell->m_casterUnit->ModifyHealth(-dmg);
        int32 mana = dmg;

        if (oldDamage > spellModDmg)
            mana += oldDamage - spellModDmg;

        Unit::AuraList const& auraDummy = spell->m_casterUnit->GetAurasByType(SPELL_AURA_DUMMY);
        for (Aura const* aura : auraDummy)
        {
            if (aura->GetSpellProto()->SpellFamilyName == SPELLFAMILY_WARLOCK && aura->GetSpellProto()->SpellIconID == 208)
                mana = (aura->GetModifier()->m_amount + 100) * mana / 100;
        }

        spell->m_casterUnit->CastCustomSpell(spell->m_casterUnit, 31818, &mana, nullptr, nullptr, true, nullptr);
        return false;
    }
};

struct spell_warlock_demonic_sacrifice : public SpellScript
{
    bool OnEffectExecute(Spell* spell, SpellEffectIndex effIdx) const override
    {
        if (effIdx != EFFECT_INDEX_0 || !spell->m_casterUnit || !spell->GetUnitTarget())
            return true;

        uint32 spellId = 0;
        switch (spell->GetUnitTarget()->GetEntry())
        {
            case 416:  spellId = 18789; break; // Imp
            case 417:  spellId = 18792; break; // Felhunter
            case 1860: spellId = 18790; break; // Voidwalker
            case 1863: spellId = 18791; break; // Succubus
            default:
                sLog.outError("Demonic Sacrifice: Unhandled creature entry (%u) case.", spell->GetUnitTarget()->GetEntry());
                return true;
        }

        spell->m_casterUnit->CastSpell(spell->m_casterUnit, spellId, true);
        return true;
    }
};

bool GetHealthstoneItemType(Unit* target, uint32 spellId, uint32& itemType)
{
    if (!target)
        return false;

    uint32 rank = 0;
    Unit::AuraList const& dummyAuras = target->GetAurasByType(SPELL_AURA_DUMMY);
    for (Aura const* aura : dummyAuras)
    {
        if (aura->GetId() == 18692)
        {
            rank = 1;
            break;
        }
        if (aura->GetId() == 18693)
        {
            rank = 2;
            break;
        }
    }

    static uint32 const itemTypes[6][3] =
    {
        { 5512, 19004, 19005 },
        { 5511, 19006, 19007 },
        { 5509, 19008, 19009 },
        { 5510, 19010, 19011 },
        { 9421, 19012, 19013 },
        { 22103, 22104, 22105 }
    };

    uint32 row = 0;
    switch (spellId)
    {
        case 6201: row = 0; break;
        case 6202: row = 1; break;
        case 5699: row = 2; break;
        case 11729: row = 3; break;
        case 11730: row = 4; break;
        case 27230: row = 5; break;
        default: return false;
    }

    itemType = itemTypes[row][rank];
    return true;
}

struct spell_warlock_create_healthstone : public SpellScript
{
    SpellCastResult OnCheckCast(Spell* spell, bool /*strict*/) const override
    {
        Player* player = spell->m_caster->ToPlayer();
        Unit* target = spell->m_targets.getUnitTarget() ? spell->m_targets.getUnitTarget() : spell->m_casterUnit;
        if (!player || !target || !target->IsPlayer())
            return SPELL_CAST_OK;

        uint32 itemType = 0;
        if (!GetHealthstoneItemType(target, spell->m_spellInfo->Id, itemType))
            return SPELL_CAST_OK;

        ItemPosCountVec dest;
        InventoryResult msg = static_cast<Player*>(target)->CanStoreNewItem(NULL_BAG, NULL_SLOT, dest, itemType, 1);
        if (msg != EQUIP_ERR_OK)
        {
            player->SendEquipError(msg, nullptr, nullptr, itemType);
            return SPELL_FAILED_DONT_REPORT;
        }

        return SPELL_CAST_OK;
    }

    bool OnEffectExecute(Spell* spell, SpellEffectIndex effIdx) const override
    {
        if (effIdx != EFFECT_INDEX_0)
            return false;

        uint32 itemType = 0;
        if (!GetHealthstoneItemType(spell->GetUnitTarget(), spell->m_spellInfo->Id, itemType))
            return false;

        spell->DoCreateItem(effIdx, itemType);
        return false;
    }
};

struct spell_warlock_pyroclasm : public AuraScript
{
    std::optional<SpellProcEventTriggerCheck> OnCheckProc(Unit const* owner, Unit* victim, SpellAuraHolder* holder, SpellEntry const* procSpell, uint32 /*procFlag*/, uint32 /*procExtra*/, WeaponAttackType /*attType*/, bool /*isVictim*/) override
    {
        if (!procSpell || !victim || !victim->IsAlive() || victim == owner)
            return SPELL_PROC_TRIGGER_FAILED;

        uint32 tick = 1;
        if ((procSpell->SpellIconID == 184 && procSpell->SpellVisual == 2253) || procSpell->IsFitToFamilyMask<CF_WARLOCK_CONFLAGRATE>())
            tick = 1;
        else if (procSpell->IsFitToFamilyMask<CF_WARLOCK_HELLFIRE>())
            tick = 15;
        else if (procSpell->IsFitToFamilyMask<CF_WARLOCK_RAIN_OF_FIRE>())
            tick = 4;
        else
            return SPELL_PROC_TRIGGER_FAILED;

        float chance = holder->GetSpellProto()->Id == 18096 ? 13.0f / tick : 26.0f / tick;
        return roll_chance_f(chance) ? SPELL_PROC_TRIGGER_OK : SPELL_PROC_TRIGGER_ROLL_FAILED;
    }
};

struct spell_warlock_cheat_death : public AuraScript
{
    std::optional<SpellAuraProcResult> OnProc(Unit* owner, Unit* /*victim*/, uint32 damage, int32 /*originalAmount*/, Aura* /*aura*/, SpellEntry const* /*procSpell*/, uint32 /*procFlag*/, uint32 /*procEx*/, uint32 /*cooldown*/) override
    {
        int32 health20 = int32(owner->GetMaxHealth()) / 5;
        if (int32(owner->GetHealth()) - int32(damage) >= health20 || int32(owner->GetHealth()) < health20)
            return SPELL_AURA_PROC_FAILED;

        return std::nullopt;
    }
};

struct spell_warlock_consequences : public AuraScript
{
    std::optional<SpellProcEventTriggerCheck> OnCheckProc(Unit const* owner, Unit* victim, SpellAuraHolder* /*holder*/, SpellEntry const* /*procSpell*/, uint32 /*procFlag*/, uint32 /*procExtra*/, WeaponAttackType /*attType*/, bool /*isVictim*/) override
    {
        if (victim == owner)
            return SPELL_PROC_TRIGGER_FAILED;

        return std::nullopt;
    }
};

struct spell_warlock_soul_siphon : public AuraScript
{
    std::optional<SpellProcEventTriggerCheck> OnCheckProc(Unit const* owner, Unit* victim, SpellAuraHolder* /*holder*/, SpellEntry const* /*procSpell*/, uint32 /*procFlag*/, uint32 /*procExtra*/, WeaponAttackType /*attType*/, bool /*isVictim*/) override
    {
        if (!owner || !victim || !victim->HasAuraTypeByCaster(SPELL_AURA_CHANNEL_DEATH_ITEM, owner->GetObjectGuid()))
            return SPELL_PROC_TRIGGER_FAILED;

        return std::nullopt;
    }
};

struct spell_warlock_ritual_of_summoning : public SpellScript
{
    SpellCastResult OnCheckCast(Spell* spell, bool /*strict*/) const override
    {
        Player* caster = spell->m_caster->ToPlayer();
        if (!caster || !caster->GetSelectionGuid())
            return SPELL_FAILED_BAD_TARGETS;

        Player* target = sObjectMgr.GetPlayer(caster->GetSelectionGuid());
        if (!target || caster == target || !target->IsInSameRaidWith(caster))
            return SPELL_FAILED_BAD_TARGETS;

        if (target->IsInCombat())
            return SPELL_FAILED_TARGET_IN_COMBAT;

        MapEntry const* mapEntry = sMapStorage.LookupEntry<MapEntry>(caster->GetMapId());
        if (mapEntry && mapEntry->IsDungeon())
        {
            if (caster->GetMap() != target->GetMap())
                return SPELL_FAILED_TARGET_NOT_IN_INSTANCE;
        }
        else if (caster->InBattleGround())
            return SPELL_FAILED_NOT_HERE;

        return SPELL_CAST_OK;
    }
};

struct spell_warlock_curse_of_idiocy : public AuraScript
{
    void OnPeriodicTrigger(Aura* /*aura*/, Unit* caster, Unit* target, WorldObject* /*targetObject*/, SpellEntry const*& spellInfo) override
    {
        if (caster && caster->GetObjectGuid() == target->GetObjectGuid())
        {
            spellInfo = nullptr;
            return;
        }

        int32 intellectLoss = 0;
        int32 spiritLoss = 0;

        Unit::AuraList const& statAuras = target->GetAurasByType(SPELL_AURA_MOD_STAT);
        for (const auto& aura : statAuras)
        {
            if (aura->GetId() != 1010)
                continue;

            switch (aura->GetModifier()->m_miscvalue)
            {
                case STAT_INTELLECT:
                    intellectLoss += aura->GetModifier()->m_amount;
                    break;
                case STAT_SPIRIT:
                    spiritLoss += aura->GetModifier()->m_amount;
                    break;
                default:
                    break;
            }
        }

        if (intellectLoss <= -90 && spiritLoss <= -90)
            spellInfo = nullptr;
    }
};

struct spell_warlock_devour_magic : public SpellScript
{
    void OnSuccessfulSpecificDispel(Spell* spell, SpellEffectIndex effIdx, uint32 /*removedAura*/, ObjectGuid const& /*removedAuraCasterGuid*/, uint32 /*dispelCount*/) const override
    {
        if (effIdx != EFFECT_INDEX_0 || !spell->m_casterUnit)
            return;

        uint32 healSpell = 0;
        switch (spell->m_spellInfo->Id)
        {
            case 19505: healSpell = 19658; break;
            case 19731: healSpell = 19732; break;
            case 19734: healSpell = 19733; break;
            case 19736: healSpell = 19735; break;
            default:
                DEBUG_LOG("Spell for Devour Magic %d not handled in spell_warlock_devour_magic", spell->m_spellInfo->Id);
                break;
        }

        if (healSpell)
            spell->m_casterUnit->CastSpell(spell->m_casterUnit, healSpell, true);
    }
};

struct spell_warlock_inferno : public SpellScript
{
    void OnSummon(Spell* spell, Creature* summon) const override
    {
        spell->m_caster->CastSpell(summon, 20882, true);
        summon->CastSpell(summon, 22707, true);
        summon->CastSpell(summon, 22703, true);
    }
};

struct spell_warlock_summon_felguard : public SpellScript
{
    void OnSummon(Spell* spell, Creature* summon) const override
    {
        spell->m_caster->CastSpell(summon, 20882, true);
        summon->CastSpell(summon, 22707, true);
        summon->SetStat(STAT_SPIRIT, summon->GetLevel() * 3);
        summon->UpdateManaRegen();
    }
};

struct spell_warlock_ritual_of_doom : public SpellScript
{
    void OnFinish(Spell* spell, bool ok) const override
    {
        if (ok || spell->IsTriggered())
            return;

        Player* player = spell->m_caster->ToPlayer();
        if (!player || player->HasSpellCooldown(spell->m_spellInfo->Id))
            return;

        player->SendClearCooldown(spell->m_spellInfo->Id, player);
    }
};
}

void AddSC_warlock_spell_scripts()
{
    RegisterSpellScript("spell_warlock_fire_shield", &GetSpellScript<spell_warlock_fire_shield>);
    RegisterSpellScript("spell_warlock_conflagrate", &GetSpellScript<spell_warlock_conflagrate>);
    RegisterSpellScript("spell_warlock_life_tap", &GetSpellScript<spell_warlock_life_tap>);
    RegisterSpellScript("spell_warlock_demonic_sacrifice", &GetSpellScript<spell_warlock_demonic_sacrifice>);
    RegisterSpellScript("spell_warlock_create_healthstone", &GetSpellScript<spell_warlock_create_healthstone>);
    RegisterAuraScript("spell_warlock_pyroclasm", &GetAuraScript<spell_warlock_pyroclasm>);
    RegisterAuraScript("spell_warlock_cheat_death", &GetAuraScript<spell_warlock_cheat_death>);
    RegisterAuraScript("spell_warlock_consequences", &GetAuraScript<spell_warlock_consequences>);
    RegisterAuraScript("spell_warlock_soul_siphon", &GetAuraScript<spell_warlock_soul_siphon>);
    RegisterSpellScript("spell_warlock_ritual_of_summoning", &GetSpellScript<spell_warlock_ritual_of_summoning>);
    RegisterAuraScript("spell_warlock_curse_of_idiocy", &GetAuraScript<spell_warlock_curse_of_idiocy>);
    RegisterSpellScript("spell_warlock_devour_magic", &GetSpellScript<spell_warlock_devour_magic>);
    RegisterSpellScript("spell_warlock_inferno", &GetSpellScript<spell_warlock_inferno>);
    RegisterSpellScript("spell_warlock_summon_felguard", &GetSpellScript<spell_warlock_summon_felguard>);
    RegisterSpellScript("spell_warlock_ritual_of_doom", &GetSpellScript<spell_warlock_ritual_of_doom>);
}
