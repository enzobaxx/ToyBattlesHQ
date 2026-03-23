#ifndef CAST_MAIN_IPC_CALLBACKS_H
#define CAST_MAIN_IPC_CALLBACKS_H

#include "Network/Packet.h"
#include "../../Network/MainSessionManager.h"
#include "../../Network/AuthSession.h"

namespace Main
{
    namespace Handlers
    {
        inline void getSessionIdFor(const Common::Network::UnecryptedPacket& request, std::shared_ptr<Common::Network::Session> session,
            Main::Network::SessionsManager& sessionsManager)
        {
            Utils::Logger::log("CastSv requested SessionID, retrieving from main...", ::Utils::LogType::Info, "Handlers::getSessionIdFor");

            auto response = request;
            response.setData(nullptr, 0);
            if (request.getDataSize() != sizeof(uint32_t))
            {
                response.setExtra(1);
                session->asyncWrite(response);
                return;
            }
            const std::uint32_t accountId = Main::Details::parseData<std::uint32_t>(request);

            if (auto targetSession = sessionsManager.getSessionByAccountId(accountId); targetSession)
            {
                std::uint32_t sessionId = targetSession->getId();
                response.setData(reinterpret_cast<std::uint8_t*>(&sessionId), sizeof(sessionId));
                response.setExtra(0);
            }
            else
            { 
                response.setExtra(1);
                response.setData(nullptr, 0);
            }
            session->asyncWrite(response); // "pong" the cast server
        }

        inline void getPlayerStateUpdate(const Common::Network::UnecryptedPacket& request, std::shared_ptr<Common::Network::Session> session,
            Main::Network::SessionsManager& sessionsManager)
        {
            auto response = request;
            response.setData(nullptr, 0);

            if (request.getDataSize() != sizeof(uint32_t))
            {
                response.setExtra(1);
                session->asyncWrite(response);
                return;
            }

            const std::uint32_t accountId = Main::Details::parseData<std::uint32_t>(request);
            const auto playerState = request.getOption(); 

            if (auto targetSession = sessionsManager.getSessionByAccountId(accountId); targetSession)
            {
                targetSession->setPlayerState(static_cast<Common::Enums::PlayerState>(playerState));
                response.setExtra(0);
            }
            else
            {
                response.setExtra(1);
            }

            session->asyncWrite(response); // "pong" the cast server with success/failure status
        }

        inline void ipcRequestCast(const Common::Network::UnecryptedPacket& request, std::shared_ptr<Common::Network::Session> session,
            Main::Network::SessionsManager& sessionsManager)
        {
            auto response = request;
            response.setData(nullptr, 0);

            const std::uint32_t seid = Main::Details::parseData<std::uint64_t>(request);

            if (auto targetSession = sessionsManager.getSessionBySessionId(seid); targetSession)
            {
                targetSession->closeSocket();
            }

            session->asyncWrite(response);
        }

    }
}

#endif