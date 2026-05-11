// Host-side glue for bot lifecycle and dispatch. Implements:
//   - Player::{Create,Remove}Playerbot{AI,Mgr}, Player::isRealPlayer
//   - Player::UpdatePlayerbotHooks (per-Player tick)
//   - World::{Update,Init}Playerbots* (world-tick driver, startup init)
//   - Player_DispatchBotOutgoing{Packet,ChatCommand} (free functions called
//     from WorldSession; the bot-AI null-check happens here so the host
//     call sites stay unconditional)
//
// Lives in the bot module so it sees both the host headers and the bot
// module's full PlayerbotAI / PlayerbotMgr types — the host declares the
// methods, only the bot module satisfies the linker with real bodies. The
// matching BUILD_PLAYERBOTS=OFF stubs live in src/game/PlayerbotStubs.cpp.

#include "playerbot/playerbot.h"
#include "Objects/Player.h"
#include "World.h"
#include "playerbot/RandomPlayerbotMgr.h"
#include "playerbot/PlayerbotAIConfig.h"
#include "BotDiagnostics.h"

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
        // Log out the master's alt bots first; otherwise their PlayerbotAI
        // outlives the mgr and they linger in-world with a dangling master.
        m_playerbotMgr->LogoutAllBots();
        delete m_playerbotMgr;
        m_playerbotMgr = nullptr;
    }
}

bool Player::isRealPlayer() const
{
    return !m_playerbotAI || m_playerbotAI->IsRealPlayer();
}

// World-tick driver. RandomPlayerbotMgr ticks both the login queue and every
// active bot's PlayerbotAI.
void World::UpdatePlayerbotsTick(uint32 diff)
{
    if (!sPlayerbotAIConfig.enabled)
        return;
    sRandomPlayerbotMgr.UpdateAI(diff);
}

// Per-Player bot tick:
//  - if `this` is a bot (m_playerbotAI != null), tick the AI
//  - if `this` is a real master driving bots (m_playerbotMgr != null), tick
//    the mgr so its alt-bot squad responds to the master's actions
//
// SC_PHASE tags around each call site let the crash handler identify which
// call faulted when bots self-destruct mid-tick (logout, far-teleport).
void Player::UpdatePlayerbotHooks(uint32 diff)
{
    if (!sPlayerbotAIConfig.enabled)
        return;
    if (m_playerbotAI)
    {
        SC_PHASE("Player::UpdatePlayerbotHooks/ai.UpdateAI", GetName());
        m_playerbotAI->UpdateAI(diff);
    }
    if (m_playerbotMgr)
    {
        SC_PHASE("Player::UpdatePlayerbotHooks/mgr.UpdateAI", GetName());
        m_playerbotMgr->UpdateAI(diff);
    }
}

// One-shot startup init. Singleton bot managers (RandomPlayerbotMgr,
// PlayerBotLoginMgr, etc.) lazy-instantiate on first reference; we just
// need the config file loaded here. No-op when AiPlayerbot.Enabled=0.
void World::InitPlayerbotsAtStartup()
{
    sPlayerbotAIConfig.Initialize();
}

// Outgoing-packet interceptor (called from WorldSession::SendPacket). For a
// real player returns false → packet goes to the network normally. For a
// bot Player returns true after handing the packet to the AI to react to
// (group invites → auto-accept, BG status, vendor errors, ...); the AI
// suppresses the network send.
bool Player_DispatchBotOutgoingPacket(Player* player, WorldPacket const& packet)
{
    if (!player) return false;
    PlayerbotAI* ai = player->GetPlayerbotAI();
    if (!ai) return false;
    ai->HandleBotOutgoingPacket(packet);
    return true;
}

// Chat dispatcher: feeds the master's chat to every bot that listens. Without
// this, in-party "+heal" / "stay" / "co" commands don't reach any bot. Bots
// owned by the master and matching random bots both get the message.
void Player_DispatchBotChatCommand(Player* master, uint32 type, std::string const& msg, uint32 lang)
{
    if (!master || !sPlayerbotAIConfig.enabled)
        return;

    if (PlayerbotMgr* mgr = master->GetPlayerbotMgr())
        mgr->HandleCommand(type, msg, lang);

    sRandomPlayerbotMgr.HandleCommand(type, msg, *master, "", master->GetTeam(), lang);
}
