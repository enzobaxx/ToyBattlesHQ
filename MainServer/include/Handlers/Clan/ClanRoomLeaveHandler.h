#ifndef CLAN_MATCH_LEAVE_HANDLER_H
#define CLAN_MATCH_LEAVE_HANDLER_H

#include "Network/Sessions/MainSession.h"
#include "Network/Packet.h"
#include "Managers/PartiesManager.h"
#include "Structures/Clan/ClanStructures.h"
#include "Rooms/Room.h"
#include "Handlers/Clan/PartyJoinHandler.h"

#include <memory>
#include "Handlers/Clan/PartyCreationHandler.h"
#include <Structures/ClientData/Structures.h>

namespace Main
{
    namespace Handlers
    {
        enum ClanRoomLeaveExtra
        {
            CLANROOM_LEAVE_SUCCESS = 1,
            CLANROOM_LEAVE_UNK1 = 51,
            CLANROOM_LEAVE_NOT_LEADER = 16,
            CLANROOM_LEAVE_CLOSE = 27, // used to get back to the party waiting room
            CLANROOM_LEAVE_GENERAL_ERROR = 28
        };

        // Reviewed 22.04.2026
        inline void handleClanRoomError(Main::Classes::PartiesManager& partiesManager, Main::Classes::RoomsManager& roomsManager,
            std::uint16_t roomNumber, std::uint16_t clanRoomNumber, std::uint32_t clanId, const std::string& errorMessage)
        {
            auto matchResult = partiesManager.getClanMatch(roomNumber);
            if (!matchResult)
            {
                // Fallback 
                if (auto selfClanRoom = partiesManager.getExactRoomFor(clanId, clanRoomNumber))
                {
                    selfClanRoom->broadcastChatMessage(errorMessage);
                    partiesManager.removeExactRoom(clanId, clanRoomNumber);
                }

                if (auto* room = roomsManager.getRoomByNumber(roomNumber))
                {
                    room->broadcastMessage(errorMessage);
                    roomsManager.removeRoom(roomNumber);
                }
                return;
            }

            partiesManager.removeClanMatch(roomNumber, true);

            if (auto* room = roomsManager.getRoomByNumber(roomNumber))
            {
                room->broadcastMessage(errorMessage);
                roomsManager.removeRoom(roomNumber, 27);
            }
        }

        // Reviewed 20.04.2026
        inline void handlePartyRoomLeave(const Common::Network::Packet& request, std::shared_ptr<Main::Network::Session> session,
            Main::Classes::PartiesManager& partiesManager,
            Main::Classes::RoomsManager& roomsManager)
        {
            Common::Network::Packet response = request;
            response.setTcpHeader(request.getSession(), Common::Enums::NO_ENCRYPTION);
            response.setExtra(Main::Enums::PartyLeaveExtra::PARTY_LEAVE_SUCCESS);

            const auto& ainfo = session->getAccountInfo();
            std::uint16_t selfPartyRoomNumber = session->getPlayer().matchContext.partyRoomNumber;

            if (auto selfPartyRoom = partiesManager.getExactRoomFor(ainfo.clanId, selfPartyRoomNumber))
            {
                const std::uint16_t roomNumber = selfPartyRoom->getClanMatchRoomNumber();
                auto result = selfPartyRoom->removePlayer(ainfo.uniqueId.session);
                if (result)
                {
                    session->asyncWrite(response);

                    bool roomEmpty = result->roomEmpty;
                    std::size_t originalIndex = result->originalIndex;

                    if (roomEmpty)
                    {
                        partiesManager.removeExactRoom(ainfo.clanId, selfPartyRoomNumber);
                        if (auto* room = roomsManager.getRoomByNumber(roomNumber); room && room->removePlayer(session, 27))
                        {
                            roomsManager.removeRoom(room->getRoomNumber(), 27);
                        }
                        return;
                    }

                    if (selfPartyRoom->isRegistered() && selfPartyRoom->getClanMatchRoomNumber() == 0)
                    {
                        response.setCommand(120, 0, 45, 0);
                        selfPartyRoom->broadcast(response);
                        selfPartyRoom->switchRegistered();
                    }

                    if (auto* room = roomsManager.getRoomByNumber(roomNumber))
                    {
                        if (room->removePlayer(session, 27))
                        {
                            roomsManager.removeRoom(room->getRoomNumber(), 27);
                        }
                    }

                    response.setCommand(419, 0, 0, static_cast<std::uint16_t>(originalIndex));
                    response.setData(reinterpret_cast<const std::uint8_t*>(&ainfo.uniqueId), sizeof(ainfo.uniqueId));
                    selfPartyRoom->broadcastExceptSelf(response, ainfo.uniqueId.session);
                }
                else
                {
                    handleClanRoomError(partiesManager, roomsManager, roomNumber, selfPartyRoomNumber, ainfo.clanId,
                        "[handlePartyRoomLeave] ERROR: removePlayer failed - " + result.error() + " - Closing the party to avoid further issues.");
                }
            }
            else
            {
                response.setExtra(Main::Enums::PartyLeaveExtra::PARTY_LEAVE_GENERAL_ERROR);
                response.setData(nullptr, 0);
                session->asyncWrite(response);
            }
        }

        // This is sent by the leader's client when they leave a clan room (note: not a party room)
        // Reviewed 20.04.2026 -- Todo: TO BE TESTED!
        inline void handleClanRoomLeave(const Common::Network::Packet& request, std::shared_ptr<Main::Network::Session> session,
            Main::Classes::PartiesManager& partiesManager, Main::Classes::RoomsManager& roomsManager)
        {
            session->sendMessage("DEBUG: HandlePartyRoomLeave");

            Common::Network::Packet response = request;
            response.setTcpHeader(request.getSession(), Common::Enums::NO_ENCRYPTION);
            response.setExtra(ClanRoomLeaveExtra::CLANROOM_LEAVE_SUCCESS);
            response.setData(nullptr, 0);

            const std::uint16_t selfClanRoomNumber = session->getPlayer().matchContext.partyRoomNumber;
            const std::uint16_t selfRoomNumber = session->getPlayer().matchContext.roomNumber;

            if (auto selfPartyRoom = partiesManager.getExactRoomFor(session->getAccountInfo().clanId, selfClanRoomNumber))
            {
                if (!selfPartyRoom->isLeader(session->getAccountInfo().uniqueId.session))
                {
                    session->sendMessage("Server error: the server received a host-only packet from a non-host client. Please report this issue");
                    response.setExtra(ClanRoomLeaveExtra::CLANROOM_LEAVE_NOT_LEADER);
                    session->asyncWrite(response);
                    return;
                }
            }
            else
            {
                session->sendMessage("Server error: failed to retrieve clan room info in HandleClanRoomLeave. Please report this issue");
                response.setExtra(ClanRoomLeaveExtra::CLANROOM_LEAVE_GENERAL_ERROR);
                session->asyncWrite(response);
                return;
            }

            auto matchResult = partiesManager.getClanMatch(selfRoomNumber);
            if (!matchResult)
            {
                session->sendMessage("Server error: failed to retrieve clan match information. Please report this issue");
                response.setExtra(ClanRoomLeaveExtra::CLANROOM_LEAVE_GENERAL_ERROR);
                session->asyncWrite(response);
                return;
            }

            auto& [partyRoomA, partyRoomB] = *matchResult;
            roomsManager.removeRoom(selfRoomNumber, ClanRoomLeaveExtra::CLANROOM_LEAVE_CLOSE);

            auto processPartyRoom = [&](std::shared_ptr<Main::Classes::PartyRoom> partyRoom)
                {
                    if (!partyRoom) return;

                    std::uint16_t clanId = partyRoom->getClanId();
                    std::uint16_t partyRoomNumber = partyRoom->getRoomNumber();

                    partyRoom->updatePartyStatus(false);
                    partyRoom->broadcast(response);

                    // Let the non-leaders leave and rejoin first
                    for (auto partySession : partyRoom->getPlayerSessions())
                    {
                        if (!partyRoom->isLeader(partySession->getSessionId()))
                        {
                            Common::Network::Packet req;
                            req.setCommand(111, 0, 0, 0);
                            handlePartyRoomLeave(req, partySession, partiesManager, roomsManager);

                            Main::ClientData::ClanRoomInfo requestStructure;
                            requestStructure.clanId = clanId;
                            requestStructure.roomNumber = partyRoomNumber;
                            req.setCommand(110, 0, 0, 0);
                            req.setData(reinterpret_cast<std::uint8_t*>(&requestStructure), sizeof(requestStructure));
                            handlePartyJoin(req, partySession, partiesManager, roomsManager);
                        }
                    }

                    // Then let the leaders leave, rejoin, and restore their leadership
                    for (auto partySession : partyRoom->getPlayerSessions())
                    {
                        if (partyRoom->isLeader(partySession->getSessionId()))
                        {
                            Common::Network::Packet req;
                            req.setCommand(111, 0, 0, 0);
                            handlePartyRoomLeave(req, partySession, partiesManager, roomsManager);

                            Main::ClientData::ClanRoomInfo requestStructure;
                            requestStructure.clanId = clanId;
                            requestStructure.roomNumber = partyRoomNumber;
                            req.setCommand(110, 0, 0, 0);
                            req.setData(reinterpret_cast<std::uint8_t*>(&requestStructure), sizeof(requestStructure));
                            handlePartyJoin(req, partySession, partiesManager, roomsManager);

                            auto newLeaderIdx = partyRoom->getPlayerIndex(partySession->getId());
                            if (newLeaderIdx && partyRoom->changeLeaderTo(*newLeaderIdx))
                            {
                                req.setCommand(114, 0, 1, *newLeaderIdx);
                                req.setData(nullptr, 0);
                                partyRoom->broadcast(req);
                            }
                        }
                    }
                };

            processPartyRoom(partyRoomA);
            processPartyRoom(partyRoomB);

            auto result = partiesManager.removeClanMatch(selfRoomNumber);
            if (!result)
            {
                session->sendMessage("ERROR: Failed to remove clan match - please report this issue!");
            }

            session->asyncWrite(response);
        }
    }
}

#endif

