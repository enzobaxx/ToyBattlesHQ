#ifndef ROOM_SETTINGS_SIMPLE_ROOM_SETTING_HANDLER_HEADER
#define ROOM_SETTINGS_SIMPLE_ROOM_SETTING_HANDLER_HEADER

#include "Network/Sessions/MainSession.h"
#include "Structures/AccountInfo/MainAccountInfo.h"
#include "Network/Packet.h"
#include "Managers/RoomsManager.h"

namespace Main
{
	namespace Handlers
	{
		// Takes care of settings that are outside the "Room Settings" button
		template<Main::Enums::RoomSimpleSetting T>
		inline void handleSimpleRoomSetting(const Common::Network::Packet& request, std::shared_ptr<Main::Network::Session> session,
			Main::Classes::RoomsManager& roomsManager)
		{
			if (Main::Classes::Room* room = roomsManager.getRoomByNumber(session->getPlayer().getMatchContext().roomNumber))
			{
				if (room->getRoomSettings().mode == Common::Enums::AiBattle)
				{
					session->sendMessage("Cannot change settings in AI battle, please create a new room");
					return;
				}

				if (!room->isHost(session->getAccountInfo().uniqueId))
				{
					return;
				}
				Common::Network::Packet response;
				response.setTcpHeader(request.getSession(), Common::Enums::USER_LARGE_ENCRYPTION);
				response.setCommand(request.getOrder(), request.getMission(), 0, request.getOption());

				if constexpr (T == Main::Enums::RoomSimpleSetting::SETTING_MAP)
				{
					response.setExtra(request.getExtra());
					response.setData(reinterpret_cast<std::uint8_t*>(const_cast<std::uint8_t*>(request.getData())), request.getDataSize());
					room->updateMap(request.getExtra());
				}
				else if constexpr (T == Main::Enums::RoomSimpleSetting::SETTING_PLAYERS_PER_TEAM)
				{
					room->setPlayersPerTeam(request.getOption() / 2); // for square mode where 1 player is chosen, this results in 0 and is taken care elsewhere!
				}
				else if constexpr (T == Main::Enums::RoomSimpleSetting::SETTING_TIME)
				{
					room->setTime(request.getOption());
				}
				else if constexpr (T == Main::Enums::RoomSimpleSetting::SETTING_SPECIFIC)
				{
					room->setSpecificSetting(request.getOption());
				}
				else
				{
					if constexpr (T == Main::Enums::SETTING_TEAMBALANCE)
					{
						return; // For now, team balance is not supported: it causes issues such as the team bug.
					}
					room->switchSimpleSetting<T>();
				}
				room->broadcastToRoom(response);
			}
		}
	}
}

#endif