// Sprint 10 cmangos/playerbots port — Phase 3d host hooks.
//
// This translation unit lives in the bot module so it sees BOTH the host headers
// (Player.h, World.h, ...) AND the bot module's PlayerbotAI / PlayerbotMgr definitions.
// The host declares these methods on Player but needs the full bot type to actually
// implement new/delete/dispatch. Defining them here keeps the host's source clean
// while still satisfying the linker.

#include "playerbot/playerbot.h"
#include "Objects/Player.h"
#include "World.h"
#include "playerbot/RandomPlayerbotMgr.h"
#include "playerbot/PlayerbotAIConfig.h"
#include "SoloCommander.h"

void Player::CreatePlayerbotAI()
{
    if (!m_playerbotAI)
        m_playerbotAI = new PlayerbotAI(this);
}

void Player::RemovePlayerbotAI()
{
    if (m_playerbotAI)
    {
        delete m_playerbotAI;
        m_playerbotAI = nullptr;
    }
}

void Player::CreatePlayerbotMgr()
{
    if (!m_playerbotMgr)
        m_playerbotMgr = new PlayerbotMgr(this);
}

void Player::RemovePlayerbotMgr()
{
    if (m_playerbotMgr)
    {
        // Sprint 10 cmangos/playerbots port — Wave 8: gracefully log out all bots this master
        // was controlling BEFORE deleting the mgr. Otherwise bots are orphaned: their
        // PlayerbotAI lives on but their master pointer dangles, and they linger in-world
        // with no input. LogoutAllBots iterates m_playerbotMgr->playerBots, calls
        // LogoutPlayerBot on each (which deletes the bot's PlayerbotAI, removes the bot
        // from the world, and frees the bot's free-floating WorldSession).
        m_playerbotMgr->LogoutAllBots();
        delete m_playerbotMgr;
        m_playerbotMgr = nullptr;
    }
}

bool Player::isRealPlayer() const
{
    // A bot has an AI attached; if no AI, it's a human player. If AI is "real-player" mode, also human.
    return !m_playerbotAI || m_playerbotAI->IsRealPlayer();
}

// Sprint 10 cmangos/playerbots port — Phase 3d Wave 2: World-tick driver for bots.
// Called from World::Update once per server frame. Forwards to RandomPlayerbotMgr::UpdateAI
// which both manages the bot login queue and ticks every active bot's PlayerbotAI.
void World::UpdatePlayerbotsTick(uint32 diff)
{
    if (!sPlayerbotAIConfig.enabled)
        return;
    sRandomPlayerbotMgr.UpdateAI(diff);
}

// Sprint 10 cmangos/playerbots port — Phase 3d Wave 2: per-Player bot tick driver.
// Called from Player::Update once per server frame for each in-world Player.
// - If this Player is a bot (has m_playerbotAI), tick the AI so it processes triggers/strategies.
// - If this Player is a real player driving bots (has m_playerbotMgr), tick the manager so its
//   alt-bot squad responds to the master's actions.
void Player::UpdatePlayerbotHooks(uint32 diff)
{
    if (!sPlayerbotAIConfig.enabled)
        return;
    if (m_playerbotAI)
    {
        SC_PHASE("HostHooks.UpdatePlayerbotHooks.beforeUpdateAI", GetName());
        m_playerbotAI->UpdateAI(diff);
        // After UpdateAI returns. If we crash here vs. inside UpdateAI we
        // can tell from the phase tag. If the bot self-destructed during
        // UpdateAI (logout / teleport finalize) `this` may be freed, so
        // even reading m_playerbotMgr can fault. Set a distinct phase
        // before the load so the crash log fingers the right call site.
        SC_PHASE("HostHooks.UpdatePlayerbotHooks.afterUpdateAI.checkMgr", GetName());
    }
    if (m_playerbotMgr)
    {
        SC_PHASE("HostHooks.UpdatePlayerbotHooks.beforeMgrUpdateAI", GetName());
        m_playerbotMgr->UpdateAI(diff);
    }
    SC_PHASE("HostHooks.UpdatePlayerbotHooks.exit", GetName());
}

// Sprint 10 cmangos/playerbots port — Phase 3d Wave 5: one-shot startup init.
// Called from World::SetInitialWorldSettings after MMap loads and before world is "up".
// Loads aiplayerbot.conf and primes the random-bot manager. Singleton instances for
// sRandomPlayerbotMgr / sPlayerBotLoginMgr / etc. lazy-instantiate on first reference,
// so we just need to load the config file here. If the file isn't present or
// AiPlayerbot.Enabled = 0, this is a no-op (logs a "Disabled" message and returns).
void World::InitPlayerbotsAtStartup()
{
    sPlayerbotAIConfig.Initialize();
    // sRandomPlayerbotMgr instantiates lazily on first World tick reference.
}

// Sprint 10 cmangos/playerbots port — Wave 9: outgoing-packet interceptor.
// Called from WorldSession::SendPacket. For real players this returns false and SendPacket
// proceeds to the network. For bot players (m_playerbotAI != null) the AI consumes the
// packet via HandleBotOutgoingPacket so the bot can react to server events (group invites
// → auto-accept, BG status, vendor errors, etc.) — returns true to suppress network send.
bool Player_DispatchBotOutgoingPacket(Player* player, WorldPacket const& packet)
{
    if (!player) return false;
    PlayerbotAI* ai = player->GetPlayerbotAI();
    if (!ai) return false;
    ai->HandleBotOutgoingPacket(packet);
    return true;
}

// Sprint 10 cmangos/playerbots port — Wave 10: chat-message dispatcher to bots.
// Called from WorldSession::HandleMessagechatOpcode. The master's chat (party/raid/guild/say/
// yell/whisper) gets fed to:
//   1) The master's own PlayerbotMgr — dispatches to each owned alt-bot's PlayerbotAI
//   2) sRandomPlayerbotMgr — same for any random-bots that recognize this master
// Each bot then parses the chat as a possible command (e.g. "co", "+heal", "stay") and reacts.
// Without this hook, bots in your party never see your chat → /party commands do nothing.
void Player_DispatchBotChatCommand(Player* master, uint32 type, std::string const& msg, uint32 lang)
{
    if (!master || !sPlayerbotAIConfig.enabled)
        return;

    if (PlayerbotMgr* mgr = master->GetPlayerbotMgr())
        mgr->HandleCommand(type, msg, lang);

    // Also feed to random-bot manager so any random bots near the master can react.
    sRandomPlayerbotMgr.HandleCommand(type, msg, *master, "", master->GetTeam(), lang);
}
