#include "scriptPCH.h"
#include "CompanionManager.hpp"
#include "MountManager.hpp"
#include "ToyManager.hpp"
#include "Pet.h"

namespace
{
template <class T>
SpellScript* GetSpellScript(SpellEntry const*)
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

template <class T>
AuraScript* GetAuraScript(SpellEntry const*)
{
    return new T();
}

void RegisterAuraScript(char const* name, AuraScript* (*getter)(SpellEntry const*))
{
    Script* script = new Script;
    script->Name = name;
    script->GetAuraScript = getter;
    script->RegisterSelf();
}

struct spell_mechanical_companion : public SpellScript
{
    bool OnEffectExecute(Spell* spell, SpellEffectIndex effIdx) const override
    {
        if (effIdx != EFFECT_INDEX_0)
            return true;

        if (!spell->m_CastItem || !spell->m_casterUnit)
            return false;

        uint32 triggerSpellId = 0;
        switch (spell->m_spellInfo->Id)
        {
            case 23074: triggerSpellId = 19804; break;
            case 23075: triggerSpellId = 12749; break;
            case 23076: triggerSpellId = 4073; break;
            case 23133: triggerSpellId = 13166; break;
            default: break;
        }

        if (triggerSpellId)
            spell->m_casterUnit->CastSpell(spell->m_casterUnit, triggerSpellId, true, spell->m_CastItem);

        return false;
    }
};

void GetForwardPosition(WorldObject* object, float distance, float& x, float& y, float& z, float& orientation, float& rot2, float& rot3)
{
    object->GetSafePosition(x, y, z);
    x += distance * cos(object->GetOrientation());
    y += distance * sin(object->GetOrientation());
    orientation = remainderf(object->GetOrientation() + M_PI_F, M_PI_F * 2.0f);
    rot2 = sin(orientation / 2);
    rot3 = cos(orientation / 2);
}

bool IsBusyForDeployable(Player* player)
{
    return player->IsInCombat() || player->IsBeingTeleported() || player->GetDeathState() == CORPSE || player->IsMoving();
}

struct spell_item_lunar_lantern : public SpellScript
{
    SpellCastResult OnCheckCast(Spell* spell, bool /*strict*/) const override
    {
        Player* player = spell->m_caster->ToPlayer();
        if (!player)
            return SPELL_CAST_OK;

        if (player->InBattleGround() || player->IsInCombat())
        {
            player->GetSession()->SendNotification("Can't use this item on the battleground or in combat.");
            return SPELL_FAILED_DONT_REPORT;
        }

        if (player->FindNearestGameObject(2004261, 20.0f) || player->FindNearestGameObject(2004263, 20.0f))
        {
            player->GetSession()->SendNotification("You can't use this close to other lanterns.");
            return SPELL_FAILED_DONT_REPORT;
        }

        return SPELL_CAST_OK;
    }

    bool OnEffectExecute(Spell* spell, SpellEffectIndex /*effIdx*/) const override
    {
        Player* player = spell->m_caster->ToPlayer();
        if (!player)
            return false;

        float x, y, z, orientation, rot2, rot3;
        GetForwardPosition(player, 0.1f, x, y, z, orientation, rot2, rot3);
        player->SummonGameObject(urand(0, 1) ? 2004261 : 2004263, x, y, z, orientation, 0.0f, 0.0f, rot2, rot3, 600, true);
        return false;
    }
};

struct spell_item_travelers_boat : public SpellScript
{
    SpellCastResult OnCheckCast(Spell* spell, bool /*strict*/) const override
    {
        Unit* caster = spell->m_casterUnit;
        Player* player = caster ? caster->ToPlayer() : nullptr;
        if (!caster || !player)
            return SPELL_CAST_OK;

        if (!caster->IsInWater() || caster->IsUnderwater())
        {
            player->GetSession()->SendNotification("You need to be in a body of water surface!");
            return SPELL_FAILED_DONT_REPORT;
        }

        return SPELL_CAST_OK;
    }

    bool OnEffectExecute(Spell* spell, SpellEffectIndex /*effIdx*/) const override
    {
        Player* player = spell->m_caster->ToPlayer();
        if (!player)
            return false;

        player->SummonGameObject(1000002, player->GetPositionX(), player->GetPositionY(), player->GetPositionZ() + 1.3f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 3600, true);
        player->TeleportTo(player->GetMapId(), player->GetPositionX(), player->GetPositionY(), player->GetPositionZ() + 3.5f, 3.0f);

        Unit* target = spell->GetUnitTarget() ? spell->GetUnitTarget() : player;
        target->AddAura(target->HasAura(8083) ? 0 : 8083);
        ChatHandler(player).SendSysMessage("You've gained +50 skill bonus to Fishing!");
        return false;
    }
};

struct spell_item_travelers_tent : public SpellScript
{
    SpellCastResult OnCheckCast(Spell* spell, bool /*strict*/) const override
    {
        Unit* caster = spell->m_casterUnit;
        Player* player = caster ? caster->ToPlayer() : nullptr;
        if (!caster || !player)
            return SPELL_CAST_OK;

        bool blockedZone = player->GetZoneId() == 1519 || player->GetZoneId() == 1637 || player->GetZoneId() == 1497 ||
            player->GetZoneId() == 1537 || player->GetZoneId() == 1657 || player->GetZoneId() == 1638;
        if (caster->IsInWater() || !caster->GetTerrain()->IsOutdoors(player->GetPositionX(), player->GetPositionY(), player->GetPositionZ()) ||
            blockedZone || player->GetMap()->IsDungeon())
        {
            player->GetSession()->SendNotification("Can't build here.");
            return SPELL_FAILED_DONT_REPORT;
        }

        if (player->FindNearestGameObject(1100000, 25.0f) || player->FindNearestGameObject(1100001, 25.0f))
        {
            player->GetSession()->SendNotification("You cannot build tents too close to other tents.");
            return SPELL_FAILED_DONT_REPORT;
        }

        return SPELL_CAST_OK;
    }

    bool OnEffectExecute(Spell* spell, SpellEffectIndex /*effIdx*/) const override
    {
        Player* player = spell->m_caster->ToPlayer();
        if (!player)
            return false;

        float x, y, z, orientation, rot2, rot3;
        GetForwardPosition(player, 4.0f, x, y, z, orientation, rot2, rot3);
        player->SummonGameObject(player->GetTeam() == ALLIANCE ? 1000001 : 1000236, x, y, z, orientation, 0.0f, 0.0f, rot2, rot3, 1200, true);
        return false;
    }
};

struct spell_item_simple_wooden_planter : public SpellScript
{
    SpellCastResult OnCheckCast(Spell* spell, bool /*strict*/) const override
    {
        Player* player = spell->m_caster->ToPlayer();
        if (!player)
            return SPELL_CAST_OK;

        if (IsBusyForDeployable(player))
        {
            player->GetSession()->SendNotification("Can't build right now.");
            return SPELL_FAILED_DONT_REPORT;
        }

        if (!player->FindNearestGameObject(1000373, 25.0f))
        {
            player->GetSession()->SendNotification("Requires a garden or a farm.");
            return SPELL_FAILED_DONT_REPORT;
        }

        if (player->FindNearestGameObject(1000334, 3.0f))
        {
            player->GetSession()->SendNotification("Can't place planters too close to each other.");
            return SPELL_FAILED_DONT_REPORT;
        }

        return SPELL_CAST_OK;
    }

    bool OnEffectExecute(Spell* spell, SpellEffectIndex /*effIdx*/) const override
    {
        Player* player = spell->m_caster->ToPlayer();
        if (!player)
            return false;

        float x, y, z, orientation, rot2, rot3;
        GetForwardPosition(player, 2.0f, x, y, z, orientation, rot2, rot3);
        player->SummonGameObject(1000334, x, y, z - 0.1f, orientation, 0.0f, 0.0f, rot2, rot3, 60 * MINUTE * IN_MILLISECONDS, true);
        player->SummonGameObject(1000335, x, y, z + 0.1f, orientation + 1.6f, 0.0f, 0.0f, 0.0f, 0.0f, 60 * MINUTE * IN_MILLISECONDS, true);
        return false;
    }
};

struct spell_item_portable_wormhole : public SpellScript
{
    SpellCastResult OnCheckCast(Spell* spell, bool /*strict*/) const override
    {
        Player* player = spell->m_caster->ToPlayer();
        if (!player)
            return SPELL_CAST_OK;

        if (IsBusyForDeployable(player))
        {
            player->GetSession()->SendNotification("Cannot use it right now.");
            return SPELL_FAILED_DONT_REPORT;
        }

        if (player->GetMoney() < 500)
        {
            player->GetSession()->SendNotification("Not enough money. The device crackles and whirls.", player->GetLevel());
            return SPELL_FAILED_DONT_REPORT;
        }

        return SPELL_CAST_OK;
    }

    bool OnEffectExecute(Spell* spell, SpellEffectIndex /*effIdx*/) const override
    {
        Player* player = spell->m_caster->ToPlayer();
        if (!player)
            return false;

        float x, y, z, orientation, rot2, rot3;
        GetForwardPosition(player, 2.0f, x, y, z, orientation, rot2, rot3);
        player->PMonsterEmote("%s just opened a wormhole.", nullptr, false, player->GetName());
        player->SummonGameObject(1000081, x, y, z + 0.5f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 8, true);
        player->ModifyMoney(-500);
        return false;
    }
};

struct spell_item_picnic_basket : public SpellScript
{
    SpellCastResult OnCheckCast(Spell* spell, bool /*strict*/) const override
    {
        Player* player = spell->m_caster->ToPlayer();
        if (!player || !IsBusyForDeployable(player))
            return SPELL_CAST_OK;

        player->GetSession()->SendNotification("You cannot use this right now.");
        return SPELL_FAILED_DONT_REPORT;
    }

    bool OnEffectExecute(Spell* spell, SpellEffectIndex /*effIdx*/) const override
    {
        Player* player = spell->m_caster->ToPlayer();
        if (!player)
            return false;

        float x, y, z;
        player->GetSafePosition(x, y, z);
        x += 2.0f * cos(player->GetOrientation());
        y += 2.0f * sin(player->GetOrientation());
        player->SummonGameObject(2004896, x, y, z, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 300, true);
        player->SummonGameObject(2004895, x + 0.5f, y + 0.5f, z, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 300, true);
        return false;
    }
};

struct spell_item_toy_train_set : public SpellScript
{
    SpellCastResult OnCheckCast(Spell* spell, bool /*strict*/) const override
    {
        Player* player = spell->m_caster->ToPlayer();
        if (!player || !IsBusyForDeployable(player))
            return SPELL_CAST_OK;

        player->GetSession()->SendNotification("You cannot use this right now.");
        return SPELL_FAILED_DONT_REPORT;
    }

    bool OnEffectExecute(Spell* spell, SpellEffectIndex /*effIdx*/) const override
    {
        Player* player = spell->m_caster->ToPlayer();
        if (!player)
            return false;

        float x, y, z, orientation, rot2, rot3;
        GetForwardPosition(player, 0.1f, x, y, z, orientation, rot2, rot3);
        if (GameObject* otherObject = player->FindNearestGameObject(2000388, 50.0f))
            otherObject->SetRespawnTime(1);

        player->SummonGameObject(2000388, x, y, z, orientation, 0.0f, 0.0f, rot2, rot3, 600, true);
        return false;
    }
};

struct spell_item_summon_utility_object : public SpellScript
{
    SpellCastResult OnCheckCast(Spell* spell, bool /*strict*/) const override
    {
        Player* player = spell->m_caster->ToPlayer();
        if (!player || spell->m_spellInfo->Id == 36600 || !IsBusyForDeployable(player))
            return SPELL_CAST_OK;

        player->GetSession()->SendNotification("You cannot use this right now.");
        return SPELL_FAILED_DONT_REPORT;
    }

    bool OnEffectExecute(Spell* spell, SpellEffectIndex /*effIdx*/) const override
    {
        Player* player = spell->m_caster->ToPlayer();
        if (!player)
            return false;

        uint32 object = 0;
        switch (spell->m_spellInfo->Id)
        {
            case 36600: object = 3000684; break;
            case 46002: object = 1000333; break;
            case 46001: object = 144112; break;
            default: break;
        }

        if (!object)
            return false;

        float x, y, z;
        player->GetSafePosition(x, y, z);
        x += 2.0f * cos(player->GetOrientation());
        y += 2.0f * sin(player->GetOrientation());

        if (GameObject* otherObject = player->FindNearestGameObject(object, 50.0f))
            otherObject->SetRespawnTime(1);

        player->SummonGameObject(object, x, y, z, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 300, true);

        if (object == 3000684)
        {
            if (GameObject* otherObject = player->FindNearestGameObject(3000685, 50.0f))
                otherObject->SetRespawnTime(1);

            player->SummonGameObject(3000685, x + 1.5f, y + 1.5f, z, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 300, true);
        }

        return false;
    }
};

struct spell_item_guild_house_teleport : public SpellScript
{
    SpellCastResult OnCheckCast(Spell* spell, bool /*strict*/) const override
    {
        Player* player = spell->m_caster->ToPlayer();
        if (!player)
            return SPELL_CAST_OK;

        if (IsBusyForDeployable(player))
        {
            player->GetSession()->SendNotification("You cannot use this right now.");
            return SPELL_FAILED_DONT_REPORT;
        }

        return SPELL_CAST_OK;
    }

    bool OnEffectExecute(Spell* spell, SpellEffectIndex /*effIdx*/) const override
    {
        Player* player = spell->m_caster->ToPlayer();
        if (!player)
            return false;

        uint32 guildId = player->GetGuildId();
        if (!guildId)
        {
            player->GetSession()->SendNotification("You don't have a guild.");
            return false;
        }

        GuildHouseEntry const* guildHouse = sObjectMgr.GetGuildHouse(guildId);
        if (!guildHouse)
        {
            player->GetSession()->SendNotification("Your guild doesn't have a guild house yet.");
            return false;
        }

        player->TeleportTo(guildHouse->map_id, guildHouse->position_x, guildHouse->position_y, guildHouse->position_z, guildHouse->orientation);
        return false;
    }
};

struct spell_item_city_protector_teleport : public SpellScript
{
    SpellCastResult OnCheckCast(Spell* spell, bool /*strict*/) const override
    {
        Player* player = spell->m_caster->ToPlayer();
        if (!player)
            return SPELL_CAST_OK;

        if (IsBusyForDeployable(player))
        {
            player->GetSession()->SendNotification("You cannot use this right now.");
            return SPELL_FAILED_DONT_REPORT;
        }

        if (!player->IsCityProtector())
        {
            player->GetSession()->SendNotification("You are no longer a City Protector.");
            return SPELL_FAILED_DONT_REPORT;
        }

        return SPELL_CAST_OK;
    }

    bool OnEffectExecute(Spell* spell, SpellEffectIndex /*effIdx*/) const override
    {
        Player* player = spell->m_caster->ToPlayer();
        if (!player)
            return false;

        std::array<std::pair<uint32, WorldLocation>, 10> racesAndLocations =
        { {
            { RACE_HUMAN, WorldLocation{0, -8828.23F, 627.92F, 94.05F, 0.0F} },
            { RACE_GNOME, WorldLocation{0, -4917.00F, -955.0F, 502.0F, 0.0F} },
            { RACE_DWARF, WorldLocation{0, -4917.0F, -955.0F, 502.0F, 0.0F} },
            { RACE_NIGHTELF, WorldLocation{1, 9962.71F, 2280.14F, 1341.39F, 0.0F} },
            { RACE_ORC, WorldLocation{1, 1437.0F, -4421.0F, 25.24F, 1.65F} },
            { RACE_TAUREN, WorldLocation{1, -1272.70F, 116.88F, 131.01F, 0.0F} },
            { RACE_TROLL, WorldLocation{1, 1437.0F, -4421.0F, 25.24F, 1.65F} },
            { RACE_UNDEAD, WorldLocation{0, 1822.09F, 238.63F, 60.69F, 0.0F} },
            { RACE_GOBLIN, WorldLocation{1, -4594.56F, -3208.2F, 34.9253F, 4.65F} },
            { RACE_HIGH_ELF, WorldLocation{0, 4226.82F, -2722.43F, 121.87F, 5.3F} },
        } };

        for (auto const& data : racesAndLocations)
        {
            if (player->GetRace() == data.first)
            {
                player->TeleportTo(data.second);
                break;
            }
        }

        return false;
    }
};

struct spell_item_ritual_table : public SpellScript
{
    SpellCastResult OnCheckCast(Spell* spell, bool /*strict*/) const override
    {
        Player* player = spell->m_caster->ToPlayer();
        if (!player || !IsBusyForDeployable(player))
            return SPELL_CAST_OK;

        player->GetSession()->SendNotification("You cannot use this right now.");
        return SPELL_FAILED_DONT_REPORT;
    }

    void OnSummon(Spell* spell, GameObject* /*summon*/) const override
    {
        Player* player = spell->m_caster->ToPlayer();
        if (!player)
            return;

        switch (spell->m_spellInfo->Id)
        {
            case 45407:
                player->MonsterTextEmote("%s begins to conjure a refreshment table.", player, false);
                break;
            case 45920:
                player->MonsterTextEmote("%s begins a Soulwell ritual.", player, false);
                break;
            default:
                break;
        }
    }
};

struct spell_item_skin_change_token : public SpellScript
{
    SpellCastResult OnCheckCast(Spell* spell, bool /*strict*/) const override
    {
        Player* player = spell->m_caster->ToPlayer();
        if (!player || !IsBusyForDeployable(player))
            return SPELL_CAST_OK;

        player->GetSession()->SendNotification("You cannot use this right now.");
        return SPELL_FAILED_DONT_REPORT;
    }

    bool OnEffectExecute(Spell* spell, SpellEffectIndex /*effIdx*/) const override
    {
        Player* player = spell->m_caster->ToPlayer();
        if (!player || !spell->m_CastItem)
            return false;

        CustomCharacterSkinEntry const* customSkin = sObjectMgr.GetCustomCharacterSkin(spell->m_CastItem->GetEntry());
        if (!customSkin)
            return false;

        bool isMale = player->GetGender() == GENDER_MALE;
        int8 bytes = isMale ? customSkin->male_id : customSkin->female_id;
        if (bytes <= 0)
        {
            ChatHandler(player).SendSysMessage("This skin is not supported by your character's gender.");
            return false;
        }

        player->SetPlayerVariable(PlayerVariables::OriginalSkinByte, std::to_string(player->GetByteValue(PLAYER_BYTES, 0)));
        player->SetByteValue(PLAYER_BYTES, 0, static_cast<uint8>(bytes));
        player->SetDisplayId(15435);

        if (bytes == 6 && player->GetRace() == RACE_UNDEAD)
            player->SetByteValue(PLAYER_BYTES_2, 0, 0);

        player->m_Events.AddLambdaEventAtOffset([player] { player->DeMorph(); }, 250);
        return false;
    }
};

struct spell_item_teleport_to_gm_island : public SpellScript
{
    bool OnEffectExecute(Spell* spell, SpellEffectIndex /*effIdx*/) const override
    {
        Player* player = spell->m_caster->ToPlayer();
        if (player)
            player->TeleportTo(1, 16247.7F, 16305.58F, 20.89F, 3.47F);

        return false;
    }
};

struct spell_item_toggle_gm_invisibility : public SpellScript
{
    bool OnEffectExecute(Spell* spell, SpellEffectIndex /*effIdx*/) const override
    {
        Player* player = spell->m_caster->ToPlayer();
        if (!player)
            return false;

        if (player->GetVisibility() != VISIBILITY_OFF)
        {
            player->SetGMVisible(false);
            player->GetSession()->SendNotification("You're now invisible.");
        }
        else
        {
            player->GetSession()->SendNotification("You're now visible.");
            player->SetGMVisible(true);
        }

        return false;
    }
};

struct spell_item_teresas_copper_coin : public SpellScript
{
    bool OnEffectExecute(Spell* spell, SpellEffectIndex /*effIdx*/) const override
    {
        Player* player = spell->m_caster->ToPlayer();
        if (!player || !spell->m_CastItem)
            return false;

        if (player->FindNearestGameObject(1000220, 3.0f))
        {
            player->HandleEmoteCommand(EMOTE_ONESHOT_KNEEL);
            player->PlayDirectSound(1204, player);
            if (CreatureInfo const* creatureInfo = sObjectMgr.GetCreatureTemplate(51301))
            {
                player->KilledMonster(creatureInfo, ObjectGuid());
                spell->ForceConsumeCastItem();
            }
        }

        player->GetSession()->SendNotification("Requires Stormwind Fountain.");
        return false;
    }
};

struct spell_item_spitelash_shrine_offering : public SpellScript
{
    bool OnEffectExecute(Spell* spell, SpellEffectIndex /*effIdx*/) const override
    {
        Player* player = spell->m_caster->ToPlayer();
        if (!player || !spell->m_CastItem)
            return false;

        if (GameObject* shrine = player->FindNearestGameObject(2010801, 10.0f))
        {
            player->SummonGameObject(2010804, shrine->GetPositionX(), shrine->GetPositionY(), shrine->GetPositionZ(), 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 4, true);
            if (CreatureInfo const* dummy = sObjectMgr.GetCreatureTemplate(60312))
            {
                player->KilledMonster(dummy, ObjectGuid());
                spell->ForceConsumeCastItem();
            }
        }
        else
            player->GetSession()->SendNotification("Requires Spitelash Shrine.");

        return false;
    }
};

struct spell_turtle_toy_collection : public SpellScript
{
    bool OnEffectExecute(Spell* spell, SpellEffectIndex /*effIdx*/) const override
    {
        Player* player = spell->m_caster->ToPlayer();
        if (!player || !spell->m_CastItem)
            return false;

        auto spellIdOpt = sToyMgr.GetToySpellId(spell->m_CastItem->GetEntry());
        if (spellIdOpt)
        {
            player->LearnSpell(spellIdOpt.value(), false);
            spell->ForceConsumeCastItem();
        }

        return false;
    }
};

struct spell_item_display_debug : public SpellScript
{
    bool OnEffectExecute(Spell* spell, SpellEffectIndex /*effIdx*/) const override
    {
        Player* player = spell->m_caster->ToPlayer();
        if (!player)
            return false;

        switch (spell->m_spellInfo->Id)
        {
            case 56044:
                player->SetDisplayId(player->GetDisplayId() - 1);
                ChatHandler(player).PSendSysMessage("|cffff8040Current DisplayID: %u|r", player->GetDisplayId());
                break;
            case 56043:
                player->SetDisplayId(player->GetDisplayId() + 1);
                ChatHandler(player).PSendSysMessage("|cffff8040Current DisplayID: %u|r", player->GetDisplayId());
                break;
            case 56045:
                player->DeMorph();
                break;
            default:
                break;
        }

        return false;
    }
};

struct spell_item_gm_flight_mode : public SpellScript
{
    bool OnEffectExecute(Spell* spell, SpellEffectIndex /*effIdx*/) const override
    {
        Player* player = spell->m_caster->ToPlayer();
        if (!player)
            return false;

        player->CastSpell(player, 14867, true);

        if (player->GetDisplayId() == 6299 || player->GetDisplayId() == 4566)
        {
            player->SetFlying(false);
            player->SetObjectScale(player->GetNativeScale());
            player->UpdateSpeed(MOVE_SWIM, false, 1.0f);
            player->UpdateSpeed(MOVE_RUN, false, 1.0f);
            player->UpdateSpeed(MOVE_WALK, false, 1.0f);
            player->DeMorph();
        }
        else
        {
            player->SetFlying(true);
            player->SetDisplayId(player->GetTeam() == ALLIANCE ? 6299 : 4566);
            player->SetObjectScale(0.7f);
            player->UpdateSpeed(MOVE_SWIM, false, 6.0f);
            player->NearLandTo(player->GetPositionX(), player->GetPositionY(), player->GetPositionZ() + 4.0f, player->GetOrientation());
        }

        return false;
    }
};

struct spell_item_jukebox : public SpellScript
{
    bool OnEffectExecute(Spell* spell, SpellEffectIndex /*effIdx*/) const override
    {
        Player* player = spell->m_caster->ToPlayer();
        if (!player || !spell->m_CastItem)
            return false;

        uint32 object = 0;
        switch (spell->m_CastItem->GetEntry())
        {
            case 51021: object = 1000055; break;
            case 10585: object = 1000077; break;
            default: break;
        }

        if (!object)
            return false;

        float x, y, z;
        player->GetSafePosition(x, y, z);
        x += 2.0f * cos(player->GetOrientation());
        y += 2.0f * sin(player->GetOrientation());
        player->SummonGameObject(object, x, y, z, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 600, true);
        return false;
    }
};

struct spell_item_illusion : public SpellScript
{
    bool OnEffectExecute(Spell* spell, SpellEffectIndex /*effIdx*/) const override
    {
        Player* player = spell->m_caster->ToPlayer();
        if (!player)
            return false;

        if (player->GetDisplayId() != player->GetNativeDisplayId())
        {
            player->hasIllusion = false;
            player->DeMorph();
            return false;
        }

        uint32 displayId = 0;
        bool const isMale = player->GetGender() == GENDER_MALE;

        switch (spell->m_spellInfo->Id)
        {
            case 31984: displayId = 10008; break;
            case 31974: displayId = 2176; break;
            case 31979: displayId = 18251; break;
            case 31971: displayId = 12030; break;
            case 31972: displayId = 8053; break;
            case 31980: displayId = 10543; break;
            case 31981: displayId = 7803; break;
            case 31982: displayId = 4923; break;
            case 31983: displayId = 11263; break;
            case 31987: displayId = 14778 + urand(0, 1); break;
            case 31976: displayId = 15393 + urand(0, 5); break;
            case 31966:
            {
                uint32 const models[] = { 14368, 14592, 14594, 14695 };
                displayId = models[urand(0, 3)];
                break;
            }
            case 31977:
            {
                uint32 const models[] = { 158, 612, 733, 18117 };
                displayId = models[urand(0, 3)];
                break;
            }
            case 31970:
            {
                uint32 const models[] = { 4761, 16162 };
                displayId = models[urand(0, 1)];
                break;
            }
            case 31973:
            {
                uint32 const models[] = { 18731, 18735, 18738, 18741, 18733, 18734, 18148 };
                displayId = models[urand(0, 6)];
                break;
            }
            case 31962:
            {
                uint32 const models[] = { 3022, 10872, 1352 };
                displayId = models[urand(0, 2)];
                break;
            }
            case 31986:
                displayId = 181;
                break;
            case 31961:
            {
                uint32 const maleModels[] = { 7170, 7102, 8847, 7185, 7809, 15095, 15096, 15097, 7209 };
                uint32 const femaleModels[] = { 9553, 15094, 10744, 15094, 11675, 15094, 7175, 11689, 10651 };
                uint32 const model = urand(0, 8);
                displayId = isMale ? maleModels[model] : femaleModels[model];
                break;
            }
            case 31964:
            {
                uint32 const models[] = { 522, 523, 524 };
                displayId = models[urand(0, 2)];
                break;
            }
            case 31978:
            {
                uint32 const models[] = { 487, 383, 384, 491 };
                displayId = models[urand(0, 3)];
                break;
            }
            case 31975:
            {
                uint32 const models[] = { 18065, 18066, 18067, 18068, 18069, 18070, 18182 };
                displayId = models[urand(0, 6)];
                break;
            }
            case 31967:
            {
                uint32 const models[] = { 8782, 10728, 10750, 10994 };
                displayId = models[urand(0, 3)];
                break;
            }
            case 31985:
            {
                uint32 const models[] = { 2012, 2010, 11331, 1013 };
                displayId = models[urand(0, 3)];
                break;
            }
            case 31968:
            {
                uint32 const maleModels[] = { 4232, 4214, 4215, 4212, 4213 };
                uint32 const femaleModels[] = { 4233, 4234, 4313, 4233, 4234 };
                uint32 const model = urand(0, 4);
                displayId = isMale ? maleModels[model] : femaleModels[model];
                break;
            }
            case 31969:
            {
                uint32 const models[] = { 10923, 10924, 10925, 10926 };
                displayId = models[urand(0, 3)];
                break;
            }
            case 31960:
            case 31965:
            {
                uint32 const maleModels[] = { 10375, 4245, 6779, 14394, 11671, 6549 };
                uint32 const femaleModels[] = { 4729, 4729, 3293, 4730, 1643, 10381 };
                uint32 const model = urand(0, 5);
                displayId = isMale ? maleModels[model] : femaleModels[model];
                break;
            }
            case 50910:
            {
                uint32 const models[] = { 18157, 18158, 7970, 5049, 9017 };
                displayId = models[urand(0, 4)];
                break;
            }
            case 50911:
            {
                uint32 const models[] = { 18121, 182, 18077 };
                displayId = models[urand(0, 2)];
                break;
            }
            case 50912:
            {
                uint32 const models[] = { 10116, 10094, 18122 };
                displayId = models[urand(0, 2)];
                break;
            }
            case 50913:
            {
                uint32 const models[] = { 18040, 18041, 18042, 18592 };
                displayId = models[urand(0, 3)];
                break;
            }
            case 50914:
            {
                uint32 const models[] = { 18120, 18123, 177 };
                displayId = models[urand(0, 2)];
                break;
            }
            case 50915:
            {
                uint32 const models[] = { 6762, 6761, 6760, 20490, 20491 };
                displayId = models[urand(0, 4)];
                break;
            }
            case 50916:
            {
                uint32 const models[] = { 20497, 20498, 20499, 20500, 20501, 20502 };
                displayId = models[urand(0, 5)];
                break;
            }
            case 50917:
            {
                uint32 const models[] = { 20492, 20493, 20494, 20495 };
                displayId = models[urand(0, 3)];
                break;
            }
            case 50918:
            {
                uint32 const models[] = { 1196, 1197, 1198, 1200, 1201, 1202 };
                displayId = models[urand(0, 5)];
                break;
            }
            case 50919:
            {
                uint32 const models[] = { 828, 987, 1065, 18114 };
                displayId = models[urand(0, 3)];
                break;
            }
            case 50920:
                displayId = 11396;
                break;
            case 31963:
            {
                uint32 const maleModels[] = { 2725, 2432, 12350 };
                uint32 const femaleModels[] = { 2721, 2722, 20505 };
                uint32 const model = urand(0, 2);
                displayId = isMale ? maleModels[model] : femaleModels[model];
                break;
            }
            case 50921:
                displayId = 18778;
                break;
            default:
                break;
        }

        if (displayId)
        {
            player->hasIllusion = true;
            player->SetDisplayId(displayId);
        }

        return false;
    }
};

struct spell_item_jukebox_music : public SpellScript
{
    bool OnEffectExecute(Spell* spell, SpellEffectIndex /*effIdx*/) const override
    {
        Player* player = spell->m_caster->ToPlayer();
        if (!player)
            return false;

        std::array<std::pair<uint32, uint32>, 16> const spellsAndSounds =
        { {
            { 57531, 30311 },
            { 57532, 30241 },
            { 57533, 30275 },
            { 57534, 30292 },
            { 57535, 30294 },
            { 57536, 30295 },
            { 57537, 30296 },
            { 57538, 30298 },
            { 57539, 30299 },
            { 57540, 30226 },
            { 57541, 30243 },
            { 57542, 15000 },
            { 57543, 30245 },
            { 57544, 30297 },
            { 57545, 30220 },
            { 57546, 30218 },
        } };

        for (auto const& data : spellsAndSounds)
        {
            if (spell->m_spellInfo->Id == data.first)
            {
                player->PlayDirectMusic(data.second, player);
                break;
            }
        }

        return false;
    }
};

struct spell_item_green_glow : public SpellScript
{
    bool OnEffectExecute(Spell* spell, SpellEffectIndex /*effIdx*/) const override
    {
        Unit* target = spell->GetUnitTarget();
        if (!target)
            return false;

        spell->m_caster->CastSpell(target, urand(0, 2) ? 11638 : 11637, true, spell->m_CastItem, nullptr, spell->GetOriginalCasterGuid());
        return false;
    }
};

uint32 GetHallowedCostumeSpell(Unit* target, uint32 spellId)
{
    switch (spellId)
    {
        case 24717:
            return target->GetGender() == GENDER_MALE ? 24708 : 24709;
        case 24718:
            return target->GetGender() == GENDER_MALE ? 24711 : 24710;
        case 24719:
            return target->GetGender() == GENDER_MALE ? 24712 : 24713;
        case 24737:
            return target->GetGender() == GENDER_MALE ? 24735 : 24736;
        default:
            break;
    }

    switch (urand(0, 6))
    {
        case 0: return target->GetGender() == GENDER_MALE ? 24708 : 24709;
        case 1: return target->GetGender() == GENDER_MALE ? 24711 : 24710;
        case 2: return target->GetGender() == GENDER_MALE ? 24712 : 24713;
        case 3: return 24723;
        case 4: return 24732;
        case 5: return target->GetGender() == GENDER_MALE ? 24735 : 24736;
        case 6: return 24740;
        default: return 0;
    }
}

struct spell_item_hallowed_wand : public SpellScript
{
    bool OnEffectExecute(Spell* spell, SpellEffectIndex /*effIdx*/) const override
    {
        Unit* target = spell->GetUnitTarget();
        if (!target || target->GetTypeId() != TYPEID_PLAYER || target->IsInCombat())
            return false;

        uint32 costumeSpell = GetHallowedCostumeSpell(target, spell->m_spellInfo->Id);
        if (costumeSpell)
            spell->m_caster->CastSpell(target, costumeSpell, true);

        return false;
    }
};

struct spell_item_trick : public SpellScript
{
    bool OnEffectExecute(Spell* spell, SpellEffectIndex /*effIdx*/) const override
    {
        if (spell->m_caster->GetTypeId() != TYPEID_PLAYER || !spell->m_casterUnit)
            return false;

        Unit* target = spell->GetUnitTarget();
        if (!target)
            return false;

        uint32 spells[8] =
        {
            target->GetGender() == GENDER_MALE ? 24708u : 24709u,
            target->GetGender() == GENDER_MALE ? 24711u : 24710u,
            target->GetGender() == GENDER_MALE ? 24712u : 24713u,
            target->GetGender() == GENDER_MALE ? 24735u : 24736u,
            24723u,
            24732u,
            24740u,
            24753u
        };

        spell->m_casterUnit->CastSpell(spell->m_casterUnit, spells[urand(0, 7)], true);
        return false;
    }
};

struct spell_item_magic_wings : public SpellScript
{
    bool OnEffectExecute(Spell* spell, SpellEffectIndex /*effIdx*/) const override
    {
        if (Unit* target = spell->GetUnitTarget())
            target->RemoveAurasDueToSpell(24754);

        return false;
    }
};

struct spell_item_trick_or_treat : public SpellScript
{
    bool OnEffectExecute(Spell* spell, SpellEffectIndex /*effIdx*/) const override
    {
        Unit* target = spell->GetUnitTarget();
        if (!target || target->GetTypeId() != TYPEID_PLAYER)
            return false;

        target->CastSpell(target, 24755, true);
        target->CastSpell(target, roll_chance_i(50) ? 24714 : 24715, true);
        return false;
    }
};

struct spell_reindeer_transformation : public SpellScript
{
    SpellCastResult OnCheckCast(Spell* spell, bool /*strict*/) const override
    {
        Unit* caster = spell->m_casterUnit;
        if (!caster || caster->HasAuraType(SPELL_AURA_MOUNTED))
            return SPELL_CAST_OK;

        return SPELL_FAILED_ONLY_MOUNTED;
    }

    bool OnEffectExecute(Spell* spell, SpellEffectIndex /*effIdx*/) const override
    {
        Unit* caster = spell->m_casterUnit;
        if (!caster || !caster->HasAuraType(SPELL_AURA_MOUNTED))
            return false;

        float speed = caster->GetSpeedRate(MOVE_RUN);
        caster->RemoveSpellsCausingAura(SPELL_AURA_MOUNTED);
        caster->CastSpell(caster, speed >= 2.0f ? 25859 : 25858, true);
        return false;
    }
};

struct spell_turtle_companion_collection : public SpellScript
{
    SpellCastResult OnCheckCast(Spell* spell, bool /*strict*/) const override
    {
        Player* player = spell->m_caster->ToPlayer();
        if (!player || !spell->m_CastItem)
            return SPELL_CAST_OK;

        auto spellIdOpt = sCompanionMgr.GetCompanionSpellId(spell->m_CastItem->GetEntry());
        if (!spellIdOpt || !player->HasSpell(spellIdOpt.value()))
            return SPELL_CAST_OK;

        player->GetSession()->SendNotification("You already know this companion.");
        return SPELL_FAILED_DONT_REPORT;
    }

    bool OnEffectExecute(Spell* spell, SpellEffectIndex /*effIdx*/) const override
    {
        Player* player = spell->m_caster->ToPlayer();
        if (!player || !spell->m_CastItem)
            return false;

        auto spellIdOpt = sCompanionMgr.GetCompanionSpellId(spell->m_CastItem->GetEntry());
        if (!spellIdOpt)
            return false;

        player->LearnSpell(spellIdOpt.value(), false);

        if (player->HasEarnedTitle(TITLE_CRAZY_CAT_LADY))
            player->AwardTitle(TITLE_CRAZY_CAT_LADY);

        if (player->HasEarnedTitle(TITLE_GRAND_FROGUS))
            player->AwardTitle(TITLE_GRAND_FROGUS);

        spell->ForceConsumeCastItem();
        return false;
    }
};

struct spell_turtle_mount_collection : public SpellScript
{
    SpellCastResult OnCheckCast(Spell* spell, bool /*strict*/) const override
    {
        Player* player = spell->m_caster->ToPlayer();
        if (!player || !spell->m_CastItem)
            return SPELL_CAST_OK;

        auto spellIdOpt = sMountMgr.GetMountSpellId(spell->m_CastItem->GetEntry());
        if (!spellIdOpt || !player->HasSpell(spellIdOpt.value()))
            return SPELL_CAST_OK;

        player->GetSession()->SendNotification("You already know this mount.");
        return SPELL_FAILED_DONT_REPORT;
    }

    bool OnEffectExecute(Spell* spell, SpellEffectIndex /*effIdx*/) const override
    {
        Player* player = spell->m_caster->ToPlayer();
        if (!player || !spell->m_CastItem)
            return false;

        auto spellIdOpt = sMountMgr.GetMountSpellId(spell->m_CastItem->GetEntry());
        if (!spellIdOpt)
            return false;

        player->LearnSpell(spellIdOpt.value(), false);
        spell->ForceConsumeCastItem();
        return false;
    }
};

struct spell_turtle_glyph_of_the_beastkeeper : public SpellScript
{
    enum
    {
        RequiredStableSlots = 3,
    };

    SpellCastResult OnCheckCast(Spell* spell, bool /*strict*/) const override
    {
        Player* player = spell->m_caster->ToPlayer();
        if (!player || !spell->m_CastItem)
            return SPELL_CAST_OK;

        if (player->m_stableSlots < RequiredStableSlots)
        {
            player->GetSession()->SendNotification("You must unlock the third pet stable slot first.");
            return SPELL_FAILED_DONT_REPORT;
        }

        if (player->m_stableSlots >= MAX_PET_STABLES)
        {
            player->GetSession()->SendNotification("You have already unlocked all pet stable slots.");
            return SPELL_FAILED_DONT_REPORT;
        }

        return SPELL_CAST_OK;
    }

    bool OnEffectExecute(Spell* spell, SpellEffectIndex effIdx) const override
    {
        if (effIdx != EFFECT_INDEX_0)
            return true;

        Player* player = spell->m_caster->ToPlayer();
        if (!player || !spell->m_CastItem)
            return false;

        player->m_stableSlots = MAX_PET_STABLES;
        player->GetSession()->SendNotification("You have unlocked the fourth pet stable slot.");
        spell->ForceConsumeCastItem();
        return false;
    }
};

struct spell_turtle_taming_quest_credit : public AuraScript
{
    void OnBeforeApply(Aura* aura, bool apply) override
    {
        if (apply || aura->GetRemoveMode() != AURA_REMOVE_BY_EXPIRE)
            return;

        Unit* target = aura->GetTarget();
        if (!target || !target->IsAlive())
            return;

        Unit* caster = aura->GetCaster();
        if (!caster || !caster->IsAlive())
            return;

        uint32 finalSpellId = 0;
        switch (aura->GetId())
        {
            case 44000: finalSpellId = 44001; break;
            case 44002: finalSpellId = 44003; break;
            case 44004: finalSpellId = 44005; break;
            case 44006: finalSpellId = 44007; break;
            case 44008: finalSpellId = 44009; break;
            case 44010: finalSpellId = 44011; break;
            case 44012: finalSpellId = 44013; break;
            case 44014: finalSpellId = 44015; break;
            case 44016: finalSpellId = 44017; break;
            case 44018: finalSpellId = 44019; break;
            case 44020: finalSpellId = 44021; break;
            case 44022: finalSpellId = 44023; break;
            case 46700: finalSpellId = 46701; break;
            case 46702: finalSpellId = 46703; break;
            case 46704: finalSpellId = 46705; break;
            case 46090: finalSpellId = 46091; break;
            case 46092: finalSpellId = 46093; break;
            case 46094: finalSpellId = 46095; break;
        }

        if (finalSpellId)
            caster->CastSpell(target, finalSpellId, true, nullptr, aura);
    }
};

} // namespace

void AddSC_turtle_spell_scripts()
{
    RegisterSpellScript("spell_mechanical_companion", &GetSpellScript<spell_mechanical_companion>);
    RegisterSpellScript("spell_item_lunar_lantern", &GetSpellScript<spell_item_lunar_lantern>);
    RegisterSpellScript("spell_item_travelers_boat", &GetSpellScript<spell_item_travelers_boat>);
    RegisterSpellScript("spell_item_travelers_tent", &GetSpellScript<spell_item_travelers_tent>);
    RegisterSpellScript("spell_item_simple_wooden_planter", &GetSpellScript<spell_item_simple_wooden_planter>);
    RegisterSpellScript("spell_item_portable_wormhole", &GetSpellScript<spell_item_portable_wormhole>);
    RegisterSpellScript("spell_item_picnic_basket", &GetSpellScript<spell_item_picnic_basket>);
    RegisterSpellScript("spell_item_toy_train_set", &GetSpellScript<spell_item_toy_train_set>);
    RegisterSpellScript("spell_item_summon_utility_object", &GetSpellScript<spell_item_summon_utility_object>);
    RegisterSpellScript("spell_item_guild_house_teleport", &GetSpellScript<spell_item_guild_house_teleport>);
    RegisterSpellScript("spell_item_city_protector_teleport", &GetSpellScript<spell_item_city_protector_teleport>);
    RegisterSpellScript("spell_item_ritual_table", &GetSpellScript<spell_item_ritual_table>);
    RegisterSpellScript("spell_item_skin_change_token", &GetSpellScript<spell_item_skin_change_token>);
    RegisterSpellScript("spell_item_teleport_to_gm_island", &GetSpellScript<spell_item_teleport_to_gm_island>);
    RegisterSpellScript("spell_item_toggle_gm_invisibility", &GetSpellScript<spell_item_toggle_gm_invisibility>);
    RegisterSpellScript("spell_item_teresas_copper_coin", &GetSpellScript<spell_item_teresas_copper_coin>);
    RegisterSpellScript("spell_item_spitelash_shrine_offering", &GetSpellScript<spell_item_spitelash_shrine_offering>);
    RegisterSpellScript("spell_turtle_toy_collection", &GetSpellScript<spell_turtle_toy_collection>);
    RegisterSpellScript("spell_item_display_debug", &GetSpellScript<spell_item_display_debug>);
    RegisterSpellScript("spell_item_gm_flight_mode", &GetSpellScript<spell_item_gm_flight_mode>);
    RegisterSpellScript("spell_item_jukebox", &GetSpellScript<spell_item_jukebox>);
    RegisterSpellScript("spell_item_illusion", &GetSpellScript<spell_item_illusion>);
    RegisterSpellScript("spell_item_jukebox_music", &GetSpellScript<spell_item_jukebox_music>);
    RegisterSpellScript("spell_item_green_glow", &GetSpellScript<spell_item_green_glow>);
    RegisterSpellScript("spell_item_hallowed_wand", &GetSpellScript<spell_item_hallowed_wand>);
    RegisterSpellScript("spell_item_trick", &GetSpellScript<spell_item_trick>);
    RegisterSpellScript("spell_item_magic_wings", &GetSpellScript<spell_item_magic_wings>);
    RegisterSpellScript("spell_item_trick_or_treat", &GetSpellScript<spell_item_trick_or_treat>);
    RegisterSpellScript("spell_reindeer_transformation", &GetSpellScript<spell_reindeer_transformation>);
    RegisterSpellScript("spell_turtle_companion_collection", &GetSpellScript<spell_turtle_companion_collection>);
    RegisterSpellScript("spell_turtle_mount_collection", &GetSpellScript<spell_turtle_mount_collection>);
    RegisterSpellScript("spell_turtle_glyph_of_the_beastkeeper", &GetSpellScript<spell_turtle_glyph_of_the_beastkeeper>);
    RegisterAuraScript("spell_turtle_taming_quest_credit", &GetAuraScript<spell_turtle_taming_quest_credit>);
}
