#ifndef CLAN_MATCH_LEAVE_HANDLER_H
#define CLAN_MATCH_LEAVE_HANDLER_H

#include "../../Network/MainSession.h"
#include "Network/Packet.h"
#include "../../Classes/PartiesManager.h"
#include "../../Structures/Clan/ClanStructures.h"
#include "../../Classes/Room.h"
#include "PartyJoinHandler.h"

#include <memory>
#include "PartyCreationHandler.h"
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
            Main::Classes::RoomsManager& roomsManager, bool isLeaderLeaving = false)
        {
            Common::Network::Packet response = request;
            response.setTcpHeader(request.getSession(), Common::Enums::NO_ENCRYPTION);
            response.setExtra(Main::Enums::PartyLeaveExtra::PARTY_LEAVE_SUCCESS);

            const auto& ainfo = session->getAccountInfo();
            std::uint16_t selfPartyRoomNumber = session->getPlayer().getPartyRoomNumber();

            if (auto selfPartyRoom = partiesManager.getExactRoomFor(ainfo.clanId, selfPartyRoomNumber))
            {
                const std::uint16_t roomNumber = selfPartyRoom->getClanMatchRoomNumber();

                auto targetPlayerIndex = selfPartyRoom->getPlayerIndex(ainfo.uniqueId.session);  // on purpose here - removePlayer invalidates targetPlayerIndex otherwise!
                if (std::optional<bool> res = selfPartyRoom->removePlayer(ainfo.uniqueId.session); res)
                {
                    session->asyncWrite(response);

                    if (*res)
                    { // the party must be closed since the only player that was in it left
                        partiesManager.removeExactRoom(ainfo.clanId, selfPartyRoomNumber);
                        auto* room = roomsManager.getRoomByNumber(roomNumber);
                        if (room && room->removePlayer(session, 27))
                        {
                            roomsManager.removeRoom(room->getRoomNumber(), 27);
                        }
                        else if (room && room->hasObserverPlayers())
                        {
                            auto obsPlayers = room->getObserverPlayers();
                            roomsManager.removeRoom(room->getRoomNumber(), 27);

                            for (auto& [info, ws] : obsPlayers)
                            {
                                if (auto s = ws.lock())
                                {
                                    if (s->getPlayer().getPartyRoomNumber())
                                    {
                                        Common::Network::Packet leavePartyReq;
                                        leavePartyReq.setCommand(111, 0, 0, 0);
                                        Main::Handlers::handlePartyRoomLeave(leavePartyReq, s, partiesManager, roomsManager);
                                    }
                                }
                            }
                            // If we dont close the room when a MOD is spectating, then the last client bugs out - that's why we're removing the room here in that case
                            auto matchResult = partiesManager.getClanMatch(roomNumber);
                            if (matchResult)
                            {
                                auto& [partyRoomA, partyRoomB] = *matchResult;
                                auto processPartyRoom = [&](std::shared_ptr<Main::Classes::PartyRoom> partyRoom)
                                    {
                                        if (!partyRoom) return;

                                        std::uint16_t clanId = partyRoom->getClanId();
                                        std::uint16_t partyRoomNumber = partyRoom->getRoomNumber();

                                        partyRoom->updatePartyStatus(false);
                                        partyRoom->broadcast(response);

                                        for (auto partySession : partyRoom->getPlayerSessions())
                                        {
                                            Common::Network::Packet req;
                                            req.setCommand(111, 0, 0, 0);
                                            handlePartyRoomLeave(req, partySession, partiesManager, roomsManager, true);
                                        }
                                    };
                                processPartyRoom(partyRoomA);
                                processPartyRoom(partyRoomB);
                            }
                        }
                        return;
                    }

                    if (selfPartyRoom->isRegistered() && selfPartyRoom->getClanMatchRoomNumber() == 0)
                    { // only unregister if outside a clan vs clan room - parties in a clan vs clan room remain always registered but unjoinable
                        response.setCommand(120, 0, 45, 0);
                        selfPartyRoom->broadcast(response);
                        selfPartyRoom->switchRegistered();
                    }

                    if (auto* room = roomsManager.getRoomByNumber(roomNumber))
                    { // clan vs clan room, notify all players about leaving
                        if (room->removePlayer(session, 27))
                        {
                            roomsManager.removeRoom(room->getRoomNumber(), 27);
                        }
                    }

                    // TODO: Check whether this condition makes sense
                    else if (!isLeaderLeaving && targetPlayerIndex)
                    {
                        // party room, notify other party members
                        response.setCommand(419, 0, 0, *targetPlayerIndex);
                        response.setData(reinterpret_cast<const std::uint8_t*>(&ainfo.uniqueId), sizeof(ainfo.uniqueId));
                        selfPartyRoom->broadcastExceptSelf(response, ainfo.uniqueId.session);
                    }
                }
                else
                { // remove everyone to avoid further bugs
                    handleClanRoomError(partiesManager, roomsManager, roomNumber, selfPartyRoomNumber, ainfo.clanId,
                        "[handlePartyRoomLeave] ERROR: removePlayer failed - Closing the party to avoid further issues.");
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

            const std::uint16_t selfClanRoomNumber = session->getPlayer().getPartyRoomNumber();
            const std::uint16_t selfRoomNumber = session->getPlayer().getRoomNumber();

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
                            handlePartyRoomLeave(req, partySession, partiesManager, roomsManager, true);

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
                            handlePartyRoomLeave(req, partySession, partiesManager, roomsManager, true);

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

