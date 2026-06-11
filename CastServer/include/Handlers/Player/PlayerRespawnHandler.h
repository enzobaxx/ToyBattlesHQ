#ifndef CAST_PLAYER_RESPAWN_HANDLER_H
#define CAST_PLAYER_RESPAWN_HANDLER_H

#include "Detail/IpcUtils.h"
#include "Structures/Player/PlayerRespawnPacket.h"
#include "Managers/RoomsManager.h"
#include "Network/SessionsManager.h"
#include "Detail/Utilities.h"
#include <AntiCheat/AntiCheat.h>

namespace Cast
{
    namespace Handlers
    {
        inline void handlePlayerRespawn(const Common::Network::UnecryptedPacket& request, std::shared_ptr<Cast::Network::Session> session,
            Cast::Classes::RoomsManager& roomsManager,
            Cast::Network::SessionsManager& sessionsManager,
            Ac::AntiCheatManager& acManager,
            std::shared_ptr<Common::Network::Session> ipcSession)
        {
            auto roomOpt = roomsManager.getRoom(session->getId());
            if (!roomOpt) return;
            auto& room = *roomOpt;

            Common::Network::UnecryptedPacket response;
            response.setTcpHeader(session->getId());
            response.setCommand(request.getOrder(), 0, 0, 0);

            Cast::Structures::PlayerRespawnPacket playerRespawnPosition = Cast::Details::parseData<Cast::Structures::PlayerRespawnPacket>(request);
            if (room->isArenaMode() && room->m_hasMatchStarted)
            {
                playerRespawnPosition.x = 60590;
                playerRespawnPosition.y = 58810;
                playerRespawnPosition.z = 26000;
            }
            if (playerRespawnPosition.isBad())
            {
                ::Utils::Logger::log("Bad Player Respawn Position", ::Utils::LogType::Warning, "Cast::handlePlayerRespawn");
                session->asyncWrite(response);
            }
            else
            {
                response.setData(reinterpret_cast<std::uint8_t*>(&playerRespawnPosition), sizeof(playerRespawnPosition));
                room->broadcastToMatch(response);
            }

            if (auto targetSession = sessionsManager.getSession(playerRespawnPosition.targetUniqueId.session); targetSession)
            {
                if (targetSession->m_team == Common::Enums::TEAM_OBSERVER) return;

                targetSession->isDead = (room->isArenaMode() && room->m_hasMatchStarted) ? true : false;
                targetSession->m_isInMatch = true;
                session->m_isInMatch = true;
                if (Cast::Details::mustBroadcastDeath(room->getMode()))
                {
                    sendPlayerStateUpdate(targetSession->getAccountId(), false, ipcSession);
                }
            }
            else
            {
                session->sendMessage("Server-side error: no target player to respawn with sessionID: " + std::to_string(playerRespawnPosition.targetUniqueId.session));
            }
        }
    }
}

#endif
