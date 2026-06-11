#ifndef CLAN_PARTY_JOIN_HANDLER_H
#define CLAN_PARTY_JOIN_HANDLER_H

#include "Network/Sessions/MainSession.h"
#include "Network/Packet.h"
#include "Managers/PartiesManager.h"
#include "Structures/Clan/ClanStructures.h"
#include "Handlers/Clan/OtherClanJoinHandler.h"
#include <Utils/Utils.h>

namespace Main
{
    namespace Handlers
    {
        enum ClanJoinExtra
        {
            CLAN_JOIN_SUCCESS,
            CLAN_JOIN_BUSY = 5, // the clan match has already started
            CLAN_JOIN_DELETED = 6, // this party does not exist
            CLAN_JOIN_FULL = 7,
            CLAN_JOIN_DELETED_2 = 35, // this party does not exist
            CLAN_JOIN_BLOCK = 42, // "you cannot enter because you have been previously kicked from the room" => probably unused
            CLAN_JOIN_GENERAL_ERROR = 43 // "disconnected from the server due to unknown error, please restart the program"
        };

        // Reviewed 20.04.2026
        inline void handlePartyJoin(const Common::Network::Packet& request, std::shared_ptr<Main::Network::Session> session, Main::Classes::PartiesManager& partiesManager,
            Main::Classes::RoomsManager& roomsManager)
        {
            Common::Network::Packet response = request;
            response.setTcpHeader(request.getSession(), Common::Enums::NO_ENCRYPTION);

            const Main::ClientData::ClanRoomInfo requestStructure = Details::parseData<Main::ClientData::ClanRoomInfo>(request); 
            const auto& ainfo = session->getAccountInfo();

            if (requestStructure.clanId != ainfo.clanId)
            { // prevent exploits such as joining another clan's party
                response.setExtra(ClanJoinExtra::CLAN_JOIN_GENERAL_ERROR);
                session->asyncWrite(response);
                return;
            }
            else if (auto selfClanRoom = partiesManager.getExactRoomFor(ainfo.clanId, requestStructure.roomNumber))
            {
                auto leaderSession = selfClanRoom->getLeaderSession(); 
                if (!leaderSession)
                {
                    session->sendMessage("Server error: Party leader was not found. Please report this to the team with steps-to-reproduce.");
                    response.setExtra(ClanJoinExtra::CLAN_JOIN_GENERAL_ERROR);
                    session->asyncWrite(response);
                    partiesManager.removeExactRoom(ainfo.clanId, requestStructure.roomNumber);
                    return;
                }
                else if (selfClanRoom->hasMatchStarted())
                {
                    response.setExtra(ClanJoinExtra::CLAN_JOIN_BUSY);
                    session->asyncWrite(response);
                    return;
                }
                else if (selfClanRoom->isFull())
                {
                    response.setExtra(ClanJoinExtra::CLAN_JOIN_FULL);
                    session->asyncWrite(response);
                    return;
                }
                else if (selfClanRoom->isPlayerInRoom(session->getAccountInfo().uniqueId.session))
                {
                    session->sendMessage("Server error: You are already in this party room. Please report this to the team with steps-to-reproduce");
                    response.setExtra(ClanJoinExtra::CLAN_JOIN_GENERAL_ERROR);
                    session->asyncWrite(response);
                    return;
                }

               
                // Get players who are already inside the party room
                const auto& waitingPlayers = selfClanRoom->getPlayers();
                response.setCommand(417, 0, 37, waitingPlayers.size()); 
                response.setData(reinterpret_cast<const std::uint8_t*>(waitingPlayers.data()), waitingPlayers.size() * sizeof(Main::Structures::PartyPlayerInfo));
                session->asyncWrite(response);

                // Get party room settings
                response.setCommand(416, 0, 0, 0);
                const auto& roomSettings = selfClanRoom->getSettings();
                response.setData(reinterpret_cast<const std::uint8_t*>(&roomSettings), sizeof(roomSettings));
                session->asyncWrite(response);

                // Join the actual party room
                response.setCommand(request.getOrder(), 0, 1, 0);
                response.setData(nullptr, 0);
                session->asyncWrite(response);

                // Notify other players that we joined
                response.setCommand(418, 0, 0, 0);
                Main::Structures::JoinPartyInfo joinInfo{ ainfo.nickname, ainfo.uniqueId, static_cast<std::uint32_t>(ainfo.playerLevel),
                    static_cast<std::uint32_t>(ainfo.clanContribution),  static_cast<std::uint32_t>(ainfo.clanWins),
                    static_cast<std::uint32_t>(ainfo.clanLosses), static_cast<std::uint32_t>(ainfo.clanDraws) };
                response.setData(reinterpret_cast<std::uint8_t*>(&joinInfo), sizeof(joinInfo));
                selfClanRoom->broadcastExceptSelf(response, session->getAccountInfo().uniqueId.session);

                if (leaderSession->getPlayer().getMatchContext().roomNumber >= Common::Constants::clanRoomNumberStart)
                { // the clan is in a room vs another clan
                    const Main::ClientData::RoomInfo joinInfo{ leaderSession->getPlayer().getMatchContext().roomNumber - 1, 2 };
                    response.setCommand(140, 0, 0, 0);
                    response.setData(reinterpret_cast<const std::uint8_t*>(&joinInfo), sizeof(joinInfo));
                    Main::Handlers::handleClanRoomJoin(response, session, roomsManager, selfClanRoom->getTeam());
                }

                selfClanRoom->addPlayer(session);
            }
            else
            {
                response.setExtra(ClanJoinExtra::CLAN_JOIN_DELETED);
                response.setData(nullptr, 0);
                session->asyncWrite(response);
            }
        }
    }
}


#endif

