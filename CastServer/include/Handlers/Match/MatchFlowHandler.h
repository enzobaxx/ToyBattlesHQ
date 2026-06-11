#ifndef CAST_MATCH_FLOW_HANDLER_H
#define CAST_MATCH_FLOW_HANDLER_H

#include "Detail/IpcUtils.h"
#include "Handlers/Room/ArenaModeHandler.h"
#include "Managers/RoomsManager.h"
#include "Network/SessionsManager.h"
#include "Structures/Player/PlayerPositionFromClient.h"
#include "Detail/Utilities.h"

namespace Cast
{
    namespace Handlers
    {
        inline void handleMatchInitialLoading(const Common::Network::UnecryptedPacket& request, std::shared_ptr<Cast::Network::Session> session,
            Cast::Classes::RoomsManager& roomsManager,
            std::uint32_t serverId,
            std::shared_ptr<Common::Network::Session> ipcSession)
        {
            auto roomOpt = roomsManager.getRoom(session->getId());
            if (!roomOpt) return;
            auto& room = *roomOpt;

            Common::Network::UnecryptedPacket response;
            response.setTcpHeader(session->getId());
            response.setCommand(request.getOrder(), 0, 0, request.getOption());
            Main::Structures::UniqueId uniqueId{ static_cast<std::uint16_t>(session->getId()), serverId, 0 };
            response.setData(reinterpret_cast<std::uint8_t*>(&uniqueId), sizeof(uniqueId));
            room->broadcastToMatch(response);

            if (request.getOption() == 9)
            {
                if (Cast::Details::mustBroadcastDeath(room->getMode()))
                {
                    sendPlayerStateUpdate(session->getAccountId(), true, ipcSession);
                }
                session->isDead = true;
                session->m_isInMatch = true;

                if (room->m_isAssassinMode && request.getSession() == session->getId())
                {
                    session->sendMessage("Assassin for RED team: " + room->m_assassinRedName);
                    session->sendMessage("Assassin for BLUE team: " + room->m_assassinBlueName);
                }
            }
        }

        inline void handleMatchStart(const Common::Network::UnecryptedPacket& request, std::shared_ptr<Cast::Network::Session> session,
            Cast::Classes::RoomsManager& roomsManager)
        {
            const auto receiverSessionId = request.getSession();
            const auto hostSessionId = session->getId();

            if (hostSessionId != receiverSessionId)
            {
                roomsManager.hostForwardToPlayer(hostSessionId, receiverSessionId, const_cast<Common::Network::UnecryptedPacket&>(request));
            }
        }

        inline void handleMatchLeave(const Common::Network::UnecryptedPacket& request, std::shared_ptr<Cast::Network::Session> session,
            Cast::Classes::RoomsManager& roomsManager)
        {
            session->setIsInMatch(false);

            auto roomOpt = roomsManager.getRoom(session->getId());
            if (!roomOpt) return;
            auto& room = *roomOpt;

            if (room->isArenaMode()) handleArenaMode(roomsManager, room);
            room->tryFindNewAssassin(session->getId());
        }

        inline void roomInfoJoinHandler(const Common::Network::UnecryptedPacket& request, std::shared_ptr<Cast::Network::Session> session,
            Cast::Classes::RoomsManager& roomsManager,
            Cast::Network::SessionsManager& sessionsManager)
        {
            const std::uint32_t mode = roomsManager.getModeOf(session->getId());
            std::vector<Cast::Structures::SinglePlayerJoinInfoResponse> singleInfoResp(request.getOption());
            for (std::size_t i = 0; i < request.getOption(); ++i)
            {
                Cast::Structures::SinglePlayerJoinInfo sp = Cast::Details::parseData<Cast::Structures::SinglePlayerJoinInfo>(request,
                    sizeof(Cast::Structures::SinglePlayerJoinInfo) * i);

                if (auto targetSession = sessionsManager.getSession(sp.uid.session); targetSession)
                {
                    singleInfoResp[i].uid = sp.uid;
                    singleInfoResp[i].playerState = targetSession->isDead ? Common::Enums::STATE_DYING : Common::Enums::STATE_NORMAL;
                    singleInfoResp[i].mode = mode == Common::Enums::MODES_MAX ? 0 : mode;
                }
            }
            auto response = request;
            response.setMission(2);
            response.setData(reinterpret_cast<std::uint8_t*>(singleInfoResp.data()), singleInfoResp.size() * sizeof(Cast::Structures::SinglePlayerJoinInfoResponse));

            roomsManager.hostForwardToPlayer(session->getId(), request.getSession(), const_cast<Common::Network::UnecryptedPacket&>(request), false);
            roomsManager.hostForwardToPlayer(session->getId(), request.getSession(), response, false);
        }

        inline void roomInfoHandler(const Common::Network::UnecryptedPacket& request, std::shared_ptr<Cast::Network::Session> session,
            Cast::Classes::RoomsManager& roomsManager)
        {
        }

        inline void handleEliminationNextRound(const Common::Network::UnecryptedPacket& request, std::shared_ptr<Cast::Network::Session> session,
            Cast::Classes::RoomsManager& roomsManager)
        {
        }
    }
}

#endif
