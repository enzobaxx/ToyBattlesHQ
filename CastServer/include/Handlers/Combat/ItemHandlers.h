#ifndef CAST_ITEM_HANDLERS_H
#define CAST_ITEM_HANDLERS_H

#include "Managers/RoomsManager.h"
#include "Network/Sessions/CastSession.h"

namespace Cast
{
    namespace Handlers
    {
        template<Common::Enums::PlayerType PlayerType>
        inline void handleItemPickup(const Common::Network::UnecryptedPacket& request, std::shared_ptr<Cast::Network::Session> session,
            Cast::Classes::RoomsManager& roomsManager)
        {
            if (session->m_team == Common::Enums::TEAM_OBSERVER || !session->m_isInMatch) return;

            if constexpr (PlayerType == Common::Enums::HOST)
            {
                roomsManager.broadcastToMatch(session->getId(), const_cast<Common::Network::UnecryptedPacket&>(request));
            }
            else if constexpr (PlayerType == Common::Enums::NON_HOST)
            {
                roomsManager.playerForwardToHost(request.getSession(), session->getId(), const_cast<Common::Network::UnecryptedPacket&>(request));
            }
        }

        template<Common::Enums::PlayerType PlayerType>
        inline void handleZombieAbility(const Common::Network::UnecryptedPacket& request, std::shared_ptr<Cast::Network::Session> session,
            Cast::Classes::RoomsManager& roomsManager)
        {
            if (session->m_team == Common::Enums::TEAM_OBSERVER || !session->m_isInMatch) return;

            if constexpr (PlayerType == Common::Enums::HOST)
            {
                roomsManager.broadcastToMatch(session->getId(), const_cast<Common::Network::UnecryptedPacket&>(request));
            }
            else if constexpr (PlayerType == Common::Enums::NON_HOST)
            {
                roomsManager.playerForwardToHost(request.getSession(), session->getId(), const_cast<Common::Network::UnecryptedPacket&>(request));
            }
        }
    }
}

#endif
