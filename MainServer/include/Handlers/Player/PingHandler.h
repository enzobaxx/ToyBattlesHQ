#ifndef PING_HANDLER_H
#define PING_HANDLER_H

#include "Network/Sessions/MainSession.h"
#include "Structures/AccountInfo/MainAccountInfo.h"
#include "Network/Packet.h"
#include "Managers/RoomsManager.h"
#include "Structures/ClientData/Structures.h"
#include <Utils/Utils.h>
#include "Rooms/Room.h"

namespace Main
{
	namespace Handlers
	{
        inline void handlePing(const Common::Network::Packet& request,
            std::shared_ptr<Main::Network::Session> session,
            Main::Classes::RoomsManager& roomsManager,
            const Main::ClientData::Ping& pingData,
            Main::Persistence::MainScheduler& scheduler)
        {
            if (Main::Classes::Room* room = roomsManager.getRoomByNumber(session->getPlayer().matchContext.roomNumber);
                room && request.getMission() == 1)
            {
                session->getPlayer().setPing(pingData.ping);
                const std::pair<Main::ClientData::Ping, Main::Structures::UniqueId> resp{ pingData, session->getAccountInfo().uniqueId };

                Common::Network::Packet response;
                response.setTcpHeader(request.getSession(), Common::Enums::NO_ENCRYPTION);
                response.setCommand(request.getOrder() + 1, request.getMission() + 1, 0, request.getOption());
                response.setData(reinterpret_cast<const std::uint8_t*>(&resp), sizeof(resp));

                room->broadcastToRoomExceptSelf(response, resp.second);
            }
        }
	}
}

#endif
