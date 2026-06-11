#ifndef CAST_CONNECTION_HANDLER_H
#define CAST_CONNECTION_HANDLER_H

#include "Detail/IpcUtils.h"
#include "Detail/Utilities.h"
#include "Managers/RoomsManager.h"
#include "Network/SessionsManager.h"
#include "Network/Sessions/CastSession.h"
#include "Structures/Match/Rest.h"
#include <asio/post.hpp>
#include <thread>

namespace Cast
{
    namespace Handlers
    {
        inline void connectionHandler(const Common::Network::UnecryptedPacket& request,
            std::shared_ptr<Cast::Network::Session> session,
            Cast::Network::SessionsManager& sessionsManager,
            asio::io_context& ioContext,
            std::shared_ptr<Common::Network::Session> ipcSession)
        {
            const std::uint32_t aid = Cast::Details::parseData<std::uint32_t>(request, 4);
            std::jthread([aid, session, &sessionsManager, &ioContext, ipcSession]() {
                std::optional<std::uint32_t> retrievedSessionId = getSessionId(aid);

                asio::post(ioContext.get_executor(), [retrievedSessionId, session, &sessionsManager, aid, ipcSession]() {
                    if (!retrievedSessionId)
                    {
                        sendCloseSocketReq(session, ipcSession);
                        return;
                    }

                    session->setAccountId(aid);
                    session->setSessionId(*retrievedSessionId);
                    sessionsManager.addSession(session, *retrievedSessionId);

                    Common::Network::UnecryptedPacket response;
                    response.setTcpHeader(session->getId());
                    response.setCommand(501, 0, 32, 0);
                    session->asyncWrite(response);
                });
            });
        }

        inline void pongHandler(const Common::Network::UnecryptedPacket& request, std::shared_ptr<Cast::Network::Session> session,
            Cast::Classes::RoomsManager& roomsManager,
            Cast::Network::SessionsManager& sessionsManager, std::uint32_t m_serverId)
        {
            Common::Network::UnecryptedPacket response;
            response.setTcpHeader(session->getId());
            response.setCommand(72, 1, 0, request.getOption());
            session->asyncWrite(response);

            sessionsManager.addSession(session, session->getId());

            auto roomOpt = roomsManager.getRoom(session->getId());
            if (!roomOpt) return;
            auto& room = *roomOpt;

            if (room->m_isAssassinMode)
            {
                static Cast::Structures::SpecialItem speedItem{ 327680, 4011000 };
                static Cast::Structures::SpecialItem powerItem{ 1376256, 4010000 };
                static Cast::Structures::SpecialItemUse speedItemUse{ 4011000 };
                static Cast::Structures::SpecialItemUse powerItemUse{ 4010000 };

                if (session->getId() == room->m_assassinBlueUid.session || session->getId() == room->m_assassinRedUid.session)
                {
                    speedItem.uid.session = powerItem.uid.session = speedItemUse.uid.session = powerItemUse.uid.session = session->getId();
                    speedItem.uid.server = powerItem.uid.server = speedItemUse.uid.server = powerItemUse.uid.server = m_serverId;

                    response.setCommand(262, 0, 0, 0);
                    response.setData(reinterpret_cast<std::uint8_t*>(&speedItem), sizeof(speedItem));
                    room->broadcastToMatch(response);
                    response.setCommand(263, 0, 1, 0);
                    response.setData(reinterpret_cast<std::uint8_t*>(&speedItemUse), sizeof(speedItemUse));
                    room->broadcastToMatch(response);

                    response.setCommand(262, 0, 0, 0);
                    response.setData(reinterpret_cast<std::uint8_t*>(&powerItem), sizeof(powerItem));
                    room->broadcastToMatch(response);
                    response.setCommand(263, 0, 1, 0);
                    response.setData(reinterpret_cast<std::uint8_t*>(&powerItemUse), sizeof(powerItemUse));
                    room->broadcastToMatch(response);
                }
            }
        }
    }
}

#endif
