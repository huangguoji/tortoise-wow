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

void RegisterSpellAndAuraScript(char const* name, SpellScript* (*spellGetter)(SpellEntry const*), AuraScript* (*auraGetter)(SpellEntry const*))
{
    Script* script = new Script;
    script->Name = name;
    script->GetSpellScript = spellGetter;
    script->GetAuraScript = auraGetter;
    script->RegisterSelf();
}

struct spell_special_max_one_target : public SpellScript
{
    void OnSetTargetMap(Spell* /*spell*/, SpellEffectIndex /*effIdx*/, uint32& /*targetMode*/, float& /*radius*/, uint32& unMaxTargets, bool& /*selectClosestTargets*/) const override
    {
        unMaxTargets = 1;
    }
};

struct spell_meteor : public SpellScript
{
    bool OnEffectExecute(Spell* spell, SpellEffectIndex effIdx) const override
    {
        if (effIdx != EFFECT_INDEX_0)
            return true;

        uint32 count = 0;
        for (const auto& hit : spell->m_UniqueTargetInfo)
            if (hit.effectMask & (1 << effIdx))
                ++count;

        if (count)
            spell->damage /= count;

        return true;
    }
};

struct spell_arcane_bomb : public SpellScript
{
    void OnEffectDamageCalculate(Spell* spell, SpellEffectIndex /*effIdx*/, float& damage) const override
    {
        Unit* target = spell->GetUnitTarget();
        if (!target)
            return;

        if (spell->m_casterUnit == target)
        {
            damage = 0;
            return;
        }

        float const distance = spell->m_caster->GetDistance3dToCenter(target);
        damage += std::max(0.0f, std::min(7000.0f, 7000.0f - (distance - 1.0f) * 6900.0f / 99.0f));
    }
};

uint32 GetEffectTargetCount(Spell const* spell, SpellEffectIndex effIdx)
{
    uint32 count = 0;
    for (const auto& hit : spell->m_UniqueTargetInfo)
        if (hit.effectMask & (1 << effIdx))
            ++count;

    return count;
}

struct spell_special_split_weapon_damage : public SpellScript
{
    void OnEffectDamageCalculate(Spell* spell, SpellEffectIndex effIdx, float& damage) const override
    {
        if (damage <= 0)
            return;

        uint32 const count = GetEffectTargetCount(spell, effIdx);
        if (count)
            damage /= count;

        if (damage < 1)
            damage = 1;
    }
};

struct spell_crustaceous_claws : public SpellScript
{
    void OnEffectDamageCalculate(Spell* spell, SpellEffectIndex effIdx, float& damage) const override
    {
        if (damage <= 0)
            return;

        uint32 const count = GetEffectTargetCount(spell, effIdx);
        if (count > 1)
            damage += (count - 1) * 500;
    }
};

struct spell_special_summon_at_target : public SpellScript
{
    bool OnEffectExecute(Spell* spell, SpellEffectIndex effIdx) const override
    {
        float x;
        float y;
        float z;

        if (Unit* target = spell->GetUnitTarget())
            target->GetPosition(x, y, z);
        else
        {
            x = spell->m_targets.m_destX;
            y = spell->m_targets.m_destY;
            z = spell->m_targets.m_destZ;
        }

        spell->m_caster->SummonCreature(spell->m_spellInfo->EffectMiscValue[effIdx], x, y, z, 0, TEMPSUMMON_TIMED_OR_CORPSE_DESPAWN, 30000);
        return false;
    }
};

struct spell_harrowing_nets : public SpellScript
{
    bool OnEffectExecute(Spell* spell, SpellEffectIndex /*effIdx*/) const override
    {
        if (Unit* target = spell->GetUnitTarget())
            target->DoResetThreat();

        return false;
    }
};

struct spell_winterax_ritual_suicide : public SpellScript
{
    bool OnEffectExecute(Spell* spell, SpellEffectIndex /*effIdx*/) const override
    {
        Player* player = spell->m_caster->ToPlayer();
        if (!player || player->GetMapId() != 30 || !player->HasAura(46437))
            return false;

        player->m_Events.AddLambdaEventAtOffset([player]() { player->SendSpellGo(player, 24240); }, 500);

        if (Creature* korrak = player->FindNearestCreature(12159, 30.0f, false))
        {
            if (!korrak->IsAlive())
            {
                korrak->Respawn();
                korrak->m_Events.AddLambdaEventAtOffset([korrak]() { korrak->SendSpellGo(korrak, 24240); }, 500);
                return false;
            }
        }

        std::set<Unit*> attackers = player->GetAttackers();
        if (attackers.size() > 2)
        {
            for (Unit* attacker : attackers)
            {
                if (attacker->GetFactionTemplateId() == 37)
                    attacker->CastSpell(attacker, 6742, true);
                else
                    player->CastSpell(attacker, 24328, true);
            }
            player->CastSpell(player, 28438, true);
            return false;
        }

        bool buffed = false;
        std::list<Player*> players;
        player->GetAlivePlayerListInRange(player, players, 10.0f);
        for (Player* friendlyPlayer : players)
        {
            if (friendlyPlayer == player)
                continue;

            if (friendlyPlayer->IsFriendlyTo(player))
            {
                player->CastSpell(friendlyPlayer, 23505, true);
                if (friendlyPlayer->HasAura(46437))
                {
                    friendlyPlayer->SendSpellGo(friendlyPlayer, 24240);
                    friendlyPlayer->CastSpell(friendlyPlayer, 46435, true);
                    friendlyPlayer->CastSpell(friendlyPlayer, 46434, true);
                    friendlyPlayer->RemoveAurasDueToSpellByCancel(46437);
                }
                buffed = true;
            }
        }

        if (buffed)
            return false;

        if (Group* group = player->GetGroup())
        {
            for (GroupReference* itr = group->GetFirstMember(); itr != nullptr; itr = itr->next())
            {
                if (Player* member = itr->getSource())
                {
                    if (member == player)
                        continue;

                    member->SendSummonRequest(player->GetObjectGuid(), player->GetMapId(), player->GetZoneId(), player->GetPositionX(), player->GetPositionY(), player->GetPositionZ());
                }
            }
        }

        return false;
    }
};

struct spell_burning_blood_visual : public AuraScript
{
    void OnAfterApply(Aura* aura, bool apply) override
    {
        Unit* target = aura->GetTarget();
        if (!apply && target->IsAlive())
            target->m_Events.AddLambdaEventAtOffset([target]() { target->CastSpell(target, 3240, true); }, 500);
    }
};

// Deprecated in patch 1.18.0
struct spell_dust_of_disappearance : public AuraScript
{
    void OnAfterApply(Aura* aura, bool apply) override
    {
        if (!apply && (aura->GetRemoveMode() == AURA_REMOVE_BY_EXPIRE || aura->GetRemoveMode() == AURA_REMOVE_BY_CANCEL))
            aura->GetTarget()->CastSpell(aura->GetTarget(), 52504, true, nullptr, aura);
    }
};

struct spell_inactive_tracker : public AuraScript
{
    void OnAfterApply(Aura* aura, bool apply) override
    {
        if (apply)
            aura->SetPeriodicTimer(1 * IN_MILLISECONDS);
    }

    void OnPeriodicDummy(Aura* aura) override
    {
        Player* player = aura->GetTarget()->ToPlayer();
        if (!player)
            return;

        if (player->IsInCombat())
        {
            player->RemoveAurasDueToSpell(44024);
            aura->GetModifier()->m_amount = 300;
            return;
        }

        aura->GetModifier()->m_amount -= (aura->GetModifier()->periodictime / IN_MILLISECONDS);
        if (aura->GetModifier()->m_amount > 0)
            return;

        aura->GetModifier()->m_amount = 300;
        player->CastSpell(player, 44024, true);

        if (SpellAuraHolder* holder = aura->GetTarget()->GetSpellAuraHolder(44024))
        {
            if (holder->GetStackAmount() == 3)
            {
                player->m_Events.AddLambdaEventAtOffset([player]()
                {
                    player->ToggleAFK();
                }, 1);
            }
        }
    }
};

struct spell_vampirism : public AuraScript
{
    std::optional<SpellAuraProcResult> OnProc(Unit* owner, Unit* /*victim*/, uint32 damage, int32 /*originalAmount*/, Aura* aura, SpellEntry const* /*procSpell*/, uint32 /*procFlag*/, uint32 /*procEx*/, uint32 /*cooldown*/) override
    {
        if (damage <= 1 || !owner->IsAlive())
            return SPELL_AURA_PROC_FAILED;

        int32 const heal = std::max(1u, aura->GetModifier()->m_amount * damage / 100);
        owner->ModifyHealth(heal);
        return SPELL_AURA_PROC_OK;
    }
};

struct spell_freezing_cold_passive : public AuraScript
{
    std::optional<SpellProcEventTriggerCheck> OnCheckProc(Unit const* /*owner*/, Unit* /*victim*/, SpellAuraHolder* /*holder*/, SpellEntry const* procSpell, uint32 procFlag, uint32 procExtra, WeaponAttackType /*attType*/, bool /*isVictim*/) override
    {
        if (!procSpell || (procExtra & PROC_EX_CAST_END) || !(procFlag & PROC_FLAG_DEAL_HARMFUL_SPELL))
            return SPELL_PROC_TRIGGER_FAILED;

        return roll_chance_u(10) ? SPELL_PROC_TRIGGER_OK : SPELL_PROC_TRIGGER_ROLL_FAILED;
    }
};

struct spell_simone_seductress_chain_lightning : public SpellScript
{
    bool OnEffectExecute(Spell* spell, SpellEffectIndex effIdx) const override
    {
        if (effIdx == EFFECT_INDEX_0)
            if (Unit* target = spell->GetUnitTarget(); target && target->HasAura(20190))
                spell->damage *= 0.25f;

        return true;
    }
};

struct spell_darkmoon_steam_tonk_cannon : public SpellScript
{
    bool OnEffectExecute(Spell* spell, SpellEffectIndex effIdx) const override
    {
        if (effIdx == EFFECT_INDEX_0)
            if (Unit* target = spell->GetUnitTarget())
                spell->m_caster->CastSpell(target, 27766, true);

        return true;
    }
};

struct spell_cannibalize_aura : public AuraScript
{
    void OnAfterApply(Aura* aura, bool apply) override
    {
        if (!apply)
            aura->GetTarget()->HandleEmoteCommand(EMOTE_STATE_NONE);
    }

    void OnPeriodicTickEnd(Aura* aura) override
    {
        Unit* target = aura->GetTarget();
        if (!target->IsAlive() || int32(target->GetPowerType()) != aura->GetModifier()->m_miscvalue)
            return;

        target->HandleEmoteCommand(EMOTE_STATE_CANNIBALIZE);
    }
};

struct spell_darkmoon_vehicle_mana_drain : public AuraScript
{
    void OnPeriodicTrigger(Aura* aura, Unit* /*caster*/, Unit* target, WorldObject* /*targetObject*/, SpellEntry const*& spellInfo) override
    {
        constexpr uint32 manaCost = 10;

        if (target->GetPower(POWER_MANA) >= manaCost)
        {
            target->ModifyPower(POWER_MANA, -int32(manaCost));
            target->SendEnergizeSpellLog(target, aura->GetId(), -int32(manaCost), POWER_MANA);
        }
        else
        {
            target->RemoveAurasDueToSpell(aura->GetId());
            spellInfo = nullptr;
        }
    }
};

struct spell_shadowmeld : public AuraScript
{
    void OnAfterApply(Aura* aura, bool apply) override
    {
        Unit* target = aura->GetTarget();
        if (target->GetTypeId() != TYPEID_PLAYER)
            return;

        if (apply)
            target->CastSpell(target, 21009, true, nullptr, aura);
        else
            target->RemoveAurasDueToSpell(21009);
    }
};

struct spell_stoneform : public AuraScript
{
    void OnAfterApply(Aura* aura, bool apply) override
    {
        Unit* target = aura->GetTarget();
        target->ApplySpellDispelImmunity(aura->GetSpellProto(), DISPEL_DISEASE, apply);
        target->ApplySpellDispelImmunity(aura->GetSpellProto(), DISPEL_POISON, apply);
    }
};

struct spell_silithyst : public AuraScript
{
    void OnAfterApply(Aura* aura, bool apply) override
    {
        Unit* target = aura->GetTarget();
        if (target->GetTypeId() != TYPEID_PLAYER)
            return;

        Player* player = target->ToPlayer();
        if (player->GetZoneId() != 1377)
            return;

        uint32 const buffSpell = player->GetTeam() == ALLIANCE ? 29894 : 29895;
        if (apply)
        {
            player->CastSpell(player, buffSpell, true);
            return;
        }

        player->RemoveAurasDueToSpell(buffSpell);
        AuraRemoveMode const removeMode = aura->GetRemoveMode();
        if (removeMode == AURA_REMOVE_BY_CANCEL || removeMode == AURA_REMOVE_BY_DEATH || removeMode == AURA_REMOVE_BY_DISPEL)
            if (ZoneScript* zoneScript = player->GetZoneScript())
                zoneScript->HandleDropFlag(player, aura->GetId());
    }
};

struct spell_controlling_steam_tonk : public AuraScript
{
    void OnAfterApply(Aura* aura, bool apply) override
    {
        if (apply)
            return;

        Unit* target = aura->GetTarget();
        Unit* caster = aura->GetCaster();
        if (!caster)
            return;

        target->CastSpell(target, 27771, true);
        caster->CastSpell(caster, 9179, true);
        caster->RemoveAurasDueToSpell(24935);

        if (GameObject* console = caster->FindNearestGameObject(180524, INTERACTION_DISTANCE))
        {
            console->SetGoState(GO_STATE_READY);
            console->SetLootState(GO_READY);
            console->RemoveFlag(GAMEOBJECT_FLAGS, GO_FLAG_IN_USE);
        }
    }
};

struct spell_battleground_banner_open : public SpellScript
{
    void OnSuccessfulStart(Spell* spell) const override
    {
        if (!spell->m_casterUnit)
            return;

        GameObject* go = spell->m_targets.getGOTarget();
        if (!go)
            return;

        LockEntry const* lockInfo = sLockStore.LookupEntry(go->GetGOInfo()->GetLockId());
        if (lockInfo && lockInfo->Index[1] == LOCKTYPE_SLOW_OPEN)
        {
            Spell* visual = new Spell(spell->m_casterUnit, sSpellMgr.GetSpellEntry(24390), true);
            visual->prepare();
        }
    }
};

struct spell_special_directed_pull : public SpellScript
{
    bool OnEffectExecute(Spell* spell, SpellEffectIndex effIdx) const override
    {
        Unit* target = spell->GetUnitTarget();
        if (!target)
            return false;

        float speedXY = float(spell->m_spellInfo->EffectMiscValue[effIdx]) * 0.1f;
        float speedZ = target->GetDistance(spell->m_caster) / speedXY * 0.5f * 20.0f;
        target->KnockBackFrom(spell->m_caster, -speedXY, speedZ);
        return false;
    }
};

struct spell_turtle_av_tied_up_spell : public SpellScript
{
    SpellCastResult OnCheckCast(Spell* spell, bool /*strict*/) const override
    {
        Unit* target = spell->m_targets.getUnitTarget();
        if (!target || !target->IsPlayer())
            return SPELL_FAILED_BAD_TARGETS;

        if ((spell->m_spellInfo->Id == 46433 && target->GetRace() != RACE_GNOME) ||
                (spell->m_spellInfo->Id == 46432 && target->GetRace() != RACE_TAUREN))
            return SPELL_FAILED_BAD_TARGETS;

        if (target->GetHealthPercent() <= 50.0f)
            return SPELL_CAST_OK;

        if (Player* player = spell->m_caster->ToPlayer())
            player->GetSession()->SendNotification("Target must be below 50%% health.");

        return SPELL_FAILED_DONT_REPORT;
    }
};

struct spell_decoy_place_loot : public SpellScript
{
    SpellCastResult OnCheckCast(Spell* spell, bool /*strict*/) const override
    {
        Player* player = ToPlayer(spell->GetAffectiveCaster());
        if (!player || player->HasAura(25688))
            return SPELL_CAST_OK;

        return SPELL_FAILED_TARGET_AURASTATE;
    }
};

struct spell_forbidden_capital_restriction : public SpellScript
{
    SpellCastResult OnCheckCast(Spell* spell, bool /*strict*/) const override
    {
        Unit* caster = spell->m_casterUnit;
        if (!caster)
            return SPELL_CAST_OK;

        uint32 areaId = caster->GetAreaId();
        return areaId == 1519 || areaId == 1637 ? SPELL_FAILED_NOT_HERE : SPELL_CAST_OK;
    }
};

struct spell_turtle_av_tied_up_aura : public AuraScript
{
    void OnAuraInit(Aura* aura) override
    {
        aura->SetPeriodicTimer(1000);
    }

    void OnAfterApply(Aura* aura, bool apply) override
    {
        Unit* target = aura->GetTarget();
        if (apply)
        {
            target->SetFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_IMMUNE_TO_NPC);
            target->MonsterYell("Help! I'm being kidnapped!");
        }
        else
            target->RemoveFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_IMMUNE_TO_NPC);
    }

    void OnPeriodicDummy(Aura* aura) override
    {
        Unit* target = aura->GetTarget();
        Player* caster = ToPlayer(aura->GetCaster());
        if (!caster)
            return;

        if (Creature* cook = target->FindNearestCreature(39998, 10.0f))
        {
            target->StopMoving(true);

            float x, y, z;
            target->GetNearPoint(cook, x, y, z, 0, 5.0f, target->GetAngle(cook));
            cook->MonsterMove(x, y, z);

            if (cook->IsFriendlyTo(target))
            {
                cook->m_Events.AddLambdaEventAtOffset([cook, target]
                {
                    cook->SetFacingToObject(target);
                    cook->MonsterSay("Why have you brought me one of our own? Imbecile!");
                }, 1000);

                char const* text = aura->GetId() == 46433 ?
                    "Find me a gnome or we'll be eating idiot stew instead." :
                    "Big fat tauren! Don't come back until you've caught one.";

                cook->m_Events.AddLambdaEventAtOffset([cook, text]
                {
                    cook->MonsterSay(text);
                    cook->GetMotionMaster()->MoveTargetedHome();
                }, 3000);
            }
            else
            {
                char const* text = aura->GetId() == 46433 ?
                    "What a succulent piece of meat! In the pot you go!" :
                    "The beast is humongous! Quickly, put it down before it breaks free!";

                caster->AreaExploredOrEventHappens(aura->GetId() == 46433 ? 40005 : 40006);

                cook->m_Events.AddLambdaEventAtOffset([cook, target, text]
                {
                    cook->SetFacingToObject(target);
                    cook->MonsterSay(text);
                    target->RemoveFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_IMMUNE_TO_NPC);
                    cook->CastSpell(target, 13608, true);
                }, 1000);

                target->m_Events.AddLambdaEventAtOffset([target]
                {
                    if (!target->IsDead())
                        return;

                    if (Creature* troll = target->FindNearestCreature(10983, 30.0f))
                        troll->HandleEmote(EMOTE_ONESHOT_CHEER);

                    if (Creature* mystic = target->FindNearestCreature(13956, 30.0f))
                        mystic->HandleEmote(EMOTE_ONESHOT_CHEER);

                    if (Creature* cook = target->FindNearestCreature(39998, 30.0f))
                        cook->HandleEmote(EMOTE_ONESHOT_CHEER);
                }, 3000);
            }

            caster->InterruptSpell(CURRENT_CHANNELED_SPELL);
            return;
        }

        if (!caster->HasUnitMovementFlag(MOVEFLAG_JUMPING) && caster->GetDistance(target) > 8.0f)
            target->MonsterMove(caster->GetPositionX(), caster->GetPositionY(), caster->GetPositionZ());
    }
};

struct spell_superconducting_magnet : public AuraScript
{
    void OnAuraInit(Aura* aura) override
    {
        aura->SetPeriodicTimer(1000);
    }

    void OnPeriodicDummy(Aura* aura) override
    {
        Unit* target = aura->GetTarget();
        if (target->IsDead() || target->IsMounted() || target->IsNonMeleeSpellCasted() ||
                !target->movespline->Finalized() || target->HasUnitState(UNIT_STAT_CAN_NOT_MOVE) ||
                target->GetMotionMaster()->GetCurrentMovementGeneratorType() != IDLE_MOTION_TYPE)
            return;

        ObjectGuid selectionGuid = target->GetTargetGuid();
        if (selectionGuid.IsEmpty())
            return;

        Unit* selection = target->GetMap()->GetUnit(selectionGuid);
        if (!selection)
            return;

        float distance = target->GetDistance(selection);
        if (distance < 10.0f || distance > 100.0f)
            return;

        target->CastSpell(selection, 45871, false);
    }
};

struct spell_baxxil_dummy : public AuraScript
{
    void OnAuraInit(Aura* aura) override
    {
        aura->SetPeriodicTimer(1000);
    }

    void OnPeriodicDummy(Aura* aura) override
    {
        if (Player* player = aura->GetTarget()->ToPlayer())
            if (!player->GetMiniPet())
                player->CastSpell(player, 50009, true);
    }
};

struct spell_spell_momentum_passive : public AuraScript
{
    void OnAuraInit(Aura* aura) override
    {
        aura->SetPeriodicTimer(1000);
    }

    void OnPeriodicDummy(Aura* aura) override
    {
        uint32 const spellId = aura->GetSpellProto()->EffectTriggerSpell[aura->GetEffIndex()];
        if (!spellId)
            return;

        Unit* target = aura->GetTarget();
        if (target->IsMoving())
            target->CastSpell(target, spellId, true);
        else
            target->RemoveAurasDueToSpell(spellId);
    }
};

struct spell_wisdom_of_the_makaru : public AuraScript
{
    void OnAfterApply(Aura* aura, bool apply) override
    {
        if (!apply || aura->GetHolder()->GetStackAmount() < 10)
            return;

        Unit* target = aura->GetTarget();
        uint32 const triggerSpell = aura->GetSpellProto()->EffectTriggerSpell[aura->GetEffIndex()];
        if (triggerSpell)
            target->CastSpell(target, triggerSpell, true);

        target->m_Events.AddLambdaEventAtOffset([target, spellId = aura->GetId()]
        {
            target->RemoveAurasDueToSpellByCancel(spellId);
        }, 1);
    }
};

struct spell_frostfire : public AuraScript
{
    void OnAfterApply(Aura* aura, bool apply) override
    {
        if (!apply || aura->GetHolder()->GetStackAmount() < 10)
            return;

        Unit* target = aura->GetTarget();
        if (Unit* caster = aura->GetCaster())
        {
            caster->CastSpell(target, 51278, true);
            caster->CastSpell(target, 51279, true);
        }

        target->m_Events.AddLambdaEventAtOffset([target, spellId = aura->GetId()]
        {
            target->RemoveAurasDueToSpellByCancel(spellId);
        }, 1);
    }
};

struct spell_snowball_knockback : public SpellScript
{
    bool OnEffectExecute(Spell* spell, SpellEffectIndex effIdx) const override
    {
        if (spell->m_spellInfo->Effect[effIdx] != SPELL_EFFECT_KNOCK_BACK)
            return true;

        return !spell->GetCaster()->IsPlayer();
    }
};
}

void AddSC_special_spell_scripts()
{
    RegisterSpellScript("spell_emperor_mutate_bug", &GetSpellScript<spell_special_max_one_target>);
    RegisterSpellScript("spell_emperor_explode_bug", &GetSpellScript<spell_special_max_one_target>);
    RegisterSpellScript("spell_shazzrah_gate", &GetSpellScript<spell_special_max_one_target>);
    RegisterSpellScript("spell_emerald_dragons_dream_fog", &GetSpellScript<spell_special_max_one_target>);
    RegisterSpellScript("spell_meteor", &GetSpellScript<spell_meteor>);
    RegisterSpellScript("spell_arcane_bomb", &GetSpellScript<spell_arcane_bomb>);
    RegisterSpellScript("spell_special_split_weapon_damage", &GetSpellScript<spell_special_split_weapon_damage>);
    RegisterSpellScript("spell_crustaceous_claws", &GetSpellScript<spell_crustaceous_claws>);
    RegisterSpellScript("spell_special_summon_at_target", &GetSpellScript<spell_special_summon_at_target>);
    RegisterSpellScript("spell_harrowing_nets", &GetSpellScript<spell_harrowing_nets>);
    RegisterSpellScript("spell_winterax_ritual_suicide", &GetSpellScript<spell_winterax_ritual_suicide>);
    RegisterSpellScript("spell_simone_seductress_chain_lightning", &GetSpellScript<spell_simone_seductress_chain_lightning>);
    RegisterSpellScript("spell_darkmoon_steam_tonk_cannon", &GetSpellScript<spell_darkmoon_steam_tonk_cannon>);
    RegisterAuraScript("spell_cannibalize_aura", &GetAuraScript<spell_cannibalize_aura>);
    RegisterAuraScript("spell_activate_mg_turret", &GetAuraScript<spell_darkmoon_vehicle_mana_drain>);
    RegisterAuraScript("spell_flamethrower", &GetAuraScript<spell_darkmoon_vehicle_mana_drain>);
    RegisterAuraScript("spell_shadowmeld", &GetAuraScript<spell_shadowmeld>);
    RegisterAuraScript("spell_stoneform", &GetAuraScript<spell_stoneform>);
    RegisterAuraScript("spell_silithyst", &GetAuraScript<spell_silithyst>);
    RegisterAuraScript("spell_controlling_steam_tonk", &GetAuraScript<spell_controlling_steam_tonk>);
    RegisterSpellScript("spell_battleground_banner_open", &GetSpellScript<spell_battleground_banner_open>);
    RegisterSpellScript("spell_decoy_place_loot", &GetSpellScript<spell_decoy_place_loot>);
    RegisterSpellScript("spell_forbidden_capital_restriction", &GetSpellScript<spell_forbidden_capital_restriction>);
    RegisterSpellAndAuraScript("spell_turtle_av_tied_up", &GetSpellScript<spell_turtle_av_tied_up_spell>, &GetAuraScript<spell_turtle_av_tied_up_aura>);
    RegisterAuraScript("spell_superconducting_magnet", &GetAuraScript<spell_superconducting_magnet>);
    RegisterAuraScript("spell_baxxil_dummy", &GetAuraScript<spell_baxxil_dummy>);
    RegisterAuraScript("spell_spell_momentum_passive", &GetAuraScript<spell_spell_momentum_passive>);
    RegisterAuraScript("spell_wisdom_of_the_makaru", &GetAuraScript<spell_wisdom_of_the_makaru>);
    RegisterAuraScript("spell_frostfire", &GetAuraScript<spell_frostfire>);
    RegisterSpellScript("spell_snowball_knockback", &GetSpellScript<spell_snowball_knockback>);
    RegisterAuraScript("spell_burning_blood_visual", &GetAuraScript<spell_burning_blood_visual>);
    RegisterAuraScript("spell_dust_of_disappearance", &GetAuraScript<spell_dust_of_disappearance>);
    RegisterAuraScript("spell_inactive_tracker", &GetAuraScript<spell_inactive_tracker>);
    RegisterAuraScript("spell_vampirism", &GetAuraScript<spell_vampirism>);
    RegisterAuraScript("spell_freezing_cold_passive", &GetAuraScript<spell_freezing_cold_passive>);
}
