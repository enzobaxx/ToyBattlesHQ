#ifndef ROOM_LEAVE_HANDLER_H
#define ROOM_LEAVE_HANDLER_H

#include "Network/Sessions/MainSession.h"
#include "Managers/RoomsManager.h"
#include "Network/Packet.h"
#include "Network/MainSessionManager.h"
#include "Rooms/Room.h"
#include "Handlers/Clan/ClanRoomLeaveHandler.h"

namespace Main
{
	namespace Handlers
	{
		enum RoomLeaveExtra
		{
			LEAVE_ERROR = 0, // doesn't leave the room
			LEAVE_NORMAL = 1,
			LEAVE_UNKNOWN = 0x2F, // seems normal exit?
			LEAVE_UNKNOWN1 = 0x15, // seems normal exit?
			LEAVE_UNKNOWN2 = 4, // seems normal exit?
			LEAVE_BREAKROOM = 0x1B,
			LEAVE_KICKED_BY_HOST = 0x2A,
			LEAVE_KICKED_BY_MOD = 0x23,
			LEAVE_VOTEKICK_COARSE_LANGUAGE = 0x27,
		};

		enum ClientExtra
		{
			KICK_PLAYER = 28,
		};

		inline void handleRoomLeave(const Common::Network::Packet& request, std::shared_ptr<Main::Network::Session> session,
			Main::Network::SessionsManager& sessionsManager,
			Main::Classes::RoomsManager& roomsManager, Main::Classes::PartiesManager& partiesManager,  const Main::Structures::UniqueId& uniqueId)
		{
			const auto& ainfo = session->getAccountInfo();
			const std::uint16_t selfRoomNumber = session->getPlayer().getRoomNumber();

			if (Main::Classes::Room* room = roomsManager.getRoomByNumber(selfRoomNumber))
			{
				if (auto targetSession = sessionsManager.getSessionBySessionId(uniqueId.session); targetSession && request.getExtra() == ClientExtra::KICK_PLAYER)
				{ // player kicked
					if (targetSession->getAccountInfo().playerGrade >= Main::Enums::GRADE_ES)
					{
						session->sendMessage("Error: Cannot kick an user with grade >= MOD", Main::Enums::HELP);
						return;
					}
					if (room->hasMatchStarted()) return; // prevent host kicking while in match
					room->addKickedPlayer(targetSession->getAccountInfo().accountID, targetSession->getAccountInfo().nickname);
					targetSession->sendMessage("You have been kicked by the host");
					room->removePlayer(targetSession, RoomLeaveExtra::LEAVE_KICKED_BY_HOST);
				}
				else
				{ // normal leave
					const std::uint16_t clanRoomNum = session->getPlayer().getPartyRoomNumber();
					if (auto clanRoom = partiesManager.getExactRoomFor(ainfo.clanId, clanRoomNum))
					{ // check if this is a clan room
						Common::Network::Packet req;
						req.setCommand(111, 0, 0, 0);
						handlePartyRoomLeave(req, session, partiesManager, roomsManager);
						return;
					}
				
					if (room->getTargetVotekickUid() == ainfo.uniqueId)
					{ // The target votekicked player is leaving while a votekick is going on
						const std::string& targetNickname = room->getAccountInfoFor(ainfo.uniqueId).nickname;
						room->votekickPlayer(ainfo.uniqueId);
						room->resetVotekick();
						room->broadcastMessage("[" + targetNickname + "] was kicked after attempting to leaving during a votekick.");
					}
					else if (room->removePlayer(session, RoomLeaveExtra::LEAVE_NORMAL))
					{ // remove the player from the actual room
						roomsManager.removeRoom(room->getRoomNumber());
					}
				}
			}
			else
			{
				Common::Network::Packet response;
				response.setTcpHeader(request.getSession(), Common::Enums::NO_ENCRYPTION);
				response.setCommand(request.getOrder(), 0, RoomLeaveExtra::LEAVE_ERROR, 0);
				session->asyncWrite(response);
				return;
			}
		}
	}
}

#endif