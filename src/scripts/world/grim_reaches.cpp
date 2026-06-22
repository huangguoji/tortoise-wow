#include "scriptPCH.h"

enum
{
    NPC_THROMRAR_DARKFLAME       = 62725,
    SPELL_DRAGONFIRE_BOMB        = 52713,
    SPELL_SKARDYN_DARKSHIELD     = 52712
};

bool EffectDummyCreature_npc_thromrar_darkflame(WorldObject* /*pCaster*/, uint32 uiSpellId, SpellEffectIndex uiEffIndex, Creature* pCreatureTarget)
{
    if (pCreatureTarget->GetEntry() != NPC_THROMRAR_DARKFLAME ||
        uiSpellId != SPELL_DRAGONFIRE_BOMB || uiEffIndex != EFFECT_INDEX_0)
        return false;

    pCreatureTarget->RemoveAurasDueToSpell(SPELL_SKARDYN_DARKSHIELD);
    return true;
}

void AddSC_grim_reaches()
{
    Script* newscript = new Script;
    newscript->Name = "npc_thromrar_darkflame";
    newscript->pEffectDummyCreature = &EffectDummyCreature_npc_thromrar_darkflame;
    newscript->RegisterSelf();
}
