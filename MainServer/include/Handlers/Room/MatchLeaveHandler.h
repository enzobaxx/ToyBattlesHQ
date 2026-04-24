#ifndef MATCH_LEAVE_HANDLER_H
#define MATCH_LEAVE_HANDLER_H

#include "../../Network/MainSession.h"
#include "../../Classes/RoomsManager.h"
#include "Network/Packet.h"
#include "../../Network/MainSessionManager.h"
#include "../Clan/ClanRoomLeaveHandler.h"

namespace Main
{
	namespace Handlers
	{	
        inline void handleMatchLeaveClan(const Common::Network::Packet& request, std::shared_ptr<Main::Network::Session> session,
            Main::Network::SessionsManager& sessionsManager,
            Main::Classes::RoomsManager& roomsManager, Main::Classes::PartiesManager& partiesManager)
        {
            if (Main::Classes::Room* room = roomsManager.getRoomByNumber(session->getPlayer().getRoomNumber()))
            {
                const std::uint16_t partyNumber = session->getPlayer().getPartyRoomNumber();

                const std::uint32_t selfRoomNumber = room->getRoomNumber();
                const auto ainfo = session->getAccountInfo();

                // remove penalty mp
                const auto currentMp = session->getPlayer().getAccountInfo().microPoints;
                session->setAccountMicroPoints(currentMp <= 120 ? 0 : currentMp - 120);
                session->sendCurrency();

                if (room->isHost(ainfo.uniqueId))
                {
                    if (room->removeHostFromMatch()) // this removes the player from both the room & the match
                    { // no valid host could be found, close the room + the clan room + the party
                        handleClanRoomError(partiesManager, roomsManager, selfRoomNumber, partyNumber, ainfo.clanId,
                            "[handleMatchLeaveClan] ERROR: removeHostFromMatch failed - Closing the parties to avoid further issues.");
                    }
                }
                else
                {
                    const auto uniqueId = ainfo.uniqueId;
                    Common::Network::Packet response;
                    response.setTcpHeader(request.getSession(), Common::Enums::NO_ENCRYPTION);
                    response.setCommand(request.getOrder(), 0, 0, 0);
                    response.setData(reinterpret_cast<const std::uint8_t*>(&uniqueId), sizeof(uniqueId));
                    room->broadcastToRoom(response);
                    room->setStateFor(uniqueId, Common::Enums::STATE_WAITING);
                }

                Common::Network::Packet req;
                req.setCommand(111, 0, 0, 0);
                handlePartyRoomLeave(req, session, partiesManager, roomsManager);
            }
        }

        inline void handleMatchLeaveNormal(const Common::Network::Packet& request, std::shared_ptr<Main::Network::Session> session,
            Main::Network::SessionsManager& sessionsManager, Main::Classes::RoomsManager& roomsManager)
        {
            if (Main::Classes::Room* room = roomsManager.getRoomByNumber(session->getPlayer().getRoomNumber()))
            {
                const std::uint32_t selfRoomNumber = room->getRoomNumber();
                const auto ainfo = session->getAccountInfo();

                if (room->getTargetVotekickUid() == ainfo.uniqueId)
                { // The target votekicked player is leaving while a votekick is going on
                    const std::string& targetNickname = room->getAccountInfoFor(ainfo.uniqueId).nickname;
                    room->votekickPlayer(ainfo.uniqueId);
                    room->resetVotekick();
                    room->broadcastMessage("[" + targetNickname + "] was kicked after attempting to leave the room during a votekick.");
                }
                else
                {
                    if (request.getExtra() == 28)
                    { // no penalty item used
                        const Main::Structures::ItemSerialInfo itemSerialInfo =
                            Main::Details::parseData<Main::Structures::ItemSerialInfo>(request, request.getDataSize() - sizeof(Main::Structures::ItemSerialInfo));
                        session->useNoPenalty(itemSerialInfo, request);
                    }
                    else if (room->getTeamForSession(session->getId()).value_or(0) != Common::Enums::TEAM_OBSERVER)
                    { // remove penalty mp
                        const auto currentMp = session->getPlayer().getAccountInfo().microPoints;
                        session->setAccountMicroPoints(currentMp <= 120 ? 0 : currentMp - 120);
                        session->sendCurrency();
                    }
                    if (room->isHost(ainfo.uniqueId))
                    {
                        if (room->removeHostFromMatch())
                        {
                            roomsManager.removeRoom(selfRoomNumber);
                        }
                    }
                    else
                    {
                        const auto uniqueId = ainfo.uniqueId;
                        Common::Network::Packet response;
                        response.setTcpHeader(request.getSession(), Common::Enums::NO_ENCRYPTION);
                        response.setCommand(request.getOrder(), 0, 0, 0);
                        response.setData(reinterpret_cast<const std::uint8_t*>(&uniqueId), sizeof(uniqueId));
                        room->broadcastToRoom(response);
                        room->setStateFor(uniqueId, Common::Enums::STATE_WAITING);
                    }
                }
            }
        }


        inline void handleMatchLeave(const Common::Network::Packet& request, std::shared_ptr<Main::Network::Session> session, 
            Main::Network::SessionsManager& sessionsManager,
            Main::Classes::RoomsManager& roomsManager, Main::Classes::PartiesManager& partiesManager)
        {
            if (session->getPlayer().getRoomNumber() >= Common::Constants::clanRoomNumberStart)
            {
                handleMatchLeaveClan(request, session, sessionsManager, roomsManager, partiesManager);
            }
            else
            { 
                handleMatchLeaveNormal(request, session, sessionsManager, roomsManager);
            }

            session->flushPendingFriendRequests();
		}
	}
}

#endif
