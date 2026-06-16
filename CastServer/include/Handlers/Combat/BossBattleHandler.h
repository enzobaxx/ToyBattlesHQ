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
                uint64_t hostId = roomsManager.getHostIdOf(session->getId());
                bool isHost = (session->getId() == hostId);

                if (isHost)
                {
                    roomsManager.broadcastToMatch(session->getId(), const_cast<Common::Network::UnecryptedPacket&>(request));
                }
                else
                {
                    roomsManager.playerForwardToHost(hostId, session->getId(), const_cast<Common::Network::UnecryptedPacket&>(request));
                }
            }
            else
            {
                roomsManager.hostForwardToPlayer(request.getSession(), session->getId(), const_cast<Common::Network::UnecryptedPacket&>(request));
            }
        }
    }
}
#endif
