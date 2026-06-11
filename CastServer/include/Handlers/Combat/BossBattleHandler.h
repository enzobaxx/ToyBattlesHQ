#ifndef CAST_BOSS_BATTLE_HANDLER_H
#define CAST_BOSS_BATTLE_HANDLER_H

#include "Managers/RoomsManager.h"
#include "Network/Sessions/CastSession.h"

namespace Cast
{
    namespace Handlers
    {
        inline void handleBossBattleForwardPacket(const Common::Network::UnecryptedPacket& request, std::shared_ptr<Cast::Network::Session> session,
            Cast::Classes::RoomsManager& roomsManager)
        {
            if (roomsManager.getModeOf(session->getId()) == Common::Enums::BossBattle)
            {
                roomsManager.broadcastToMatch(session->getId(), const_cast<Common::Network::UnecryptedPacket&>(request));
            }
            else
            {
                roomsManager.hostForwardToPlayer(request.getSession(), session->getId(), const_cast<Common::Network::UnecryptedPacket&>(request));
            }
        }
    }
}

#endif
