#ifndef CAST_CRASH_HANDLER_H
#define CAST_CRASH_HANDLER_H

#include "Managers/RoomsManager.h"
#include "Network/Sessions/CastSession.h"

namespace Cast
{
    namespace Handlers
    {
        inline void handleCrash(const Common::Network::UnecryptedPacket& request, std::shared_ptr<Cast::Network::Session> session,
            Cast::Classes::RoomsManager& roomsManager,
            std::uint32_t serverId)
        {
            session->setIsInMatch(false);
            Main::Structures::UniqueId uniqueId{ static_cast<std::uint16_t>(session->getId()), serverId, 0 };
            Common::Network::UnecryptedPacket response = request;
            response.setData(reinterpret_cast<std::uint8_t*>(&uniqueId), sizeof(uniqueId));
            roomsManager.playerForwardToHost(request.getSession(), session->getId(), response);
            roomsManager.removePlayerFromRoom(session->getId());
        }
    }
}

#endif
