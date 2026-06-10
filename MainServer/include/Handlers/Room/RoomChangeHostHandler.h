#ifndef ROOM_CHANGE_HOST_HANDLER
#define ROOM_CHANGE_HOST_HANDLER

#include "Network/Sessions/MainSession.h"
#include "Network/Packet.h"
#include "Managers/RoomsManager.h"

namespace Main
{
	namespace Handlers
	{
		enum RoomChangeHostExtra
		{
			CHANGE_HOST_SUCCESS = 1,
			CHANGE_HOST_FAIL = 2,
			CHANGE_HOST_DOESNT_EXIST = 0xD,
			CHANGE_HOST_NO_PERMISSION = 0x10,
		};

		inline void handleHostChange(const Common::Network::Packet& request, std::shared_ptr<Main::Network::Session> session, Main::Classes::RoomsManager& roomsManager)
		{	
			if (Main::Classes::Room* room = roomsManager.getRoomByNumber(session->getPlayer().getRoomNumber()))
			{
				Common::Network::Packet response;
				response.setTcpHeader(request.getSession(), Common::Enums::USER_LARGE_ENCRYPTION);
				response.setCommand(request.getOrder(), 0, RoomChangeHostExtra::CHANGE_HOST_SUCCESS, request.getOption());
				
				if (!room->isHost(session->getAccountInfo().uniqueId))
				{
					response.setExtra(RoomChangeHostExtra::CHANGE_HOST_NO_PERMISSION);
					session->asyncWrite(response);
				}
				else if (!room->changeHost(request.getOption()))
				{
					response.setExtra(RoomChangeHostExtra::CHANGE_HOST_DOESNT_EXIST);
					session->asyncWrite(response);
				}
				else
				{
					room->broadcastToRoom(const_cast<Common::Network::Packet&>(request));
				}
			}
		}
	}
}

#endif
