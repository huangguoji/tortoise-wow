
#include "playerbot/playerbot.h"
#include "EmoteStrategy.h"
#include "playerbot/PlayerbotAIConfig.h"

using namespace ai;

void EmoteStrategy::InitNonCombatTriggers(std::list<TriggerNode*> &triggers)
{
    // SoloCommander: ALL emote/talk/suggest/greet/mount-anim triggers stripped.
    // User does not want any RP behaviour from bots. The "hi" whisper still fires
    // through TellPlayer; everything else (random emotes, public says, talk
    // animation on whisper) is silenced. If you really need emotes back for some
    // future use case, add a config knob and gate per-trigger here.
    (void)triggers;
}
