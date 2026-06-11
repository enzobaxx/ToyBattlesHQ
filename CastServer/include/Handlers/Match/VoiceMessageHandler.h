#ifndef CAST_VOICE_MESSAGE_HANDLER_H
#define CAST_VOICE_MESSAGE_HANDLER_H

#include "Managers/RoomsManager.h"
#include "Network/Sessions/CastSession.h"
#include "Detail/Utilities.h"
#include "Structures/AccountInfo/MainAccountUniqueId.h"

namespace Cast
{
    namespace Handlers
    {
        inline void handleVoiceMessage(const Common::Network::UnecryptedPacket& request, std::shared_ptr<Cast::Network::Session> session,
            Cast::Classes::RoomsManager& roomsManager, std::uint32_t serverId)
        {
            auto response = request;
            response.setOrder(273);

            struct VoiceMessageData
            {
                Main::Structures::UniqueId uid;
                std::uint32_t voiceId;
            } voiceMessageData;

            voiceMessageData.uid = Main::Structures::UniqueId{ static_cast<std::uint32_t>(session->getId()), serverId, 0 };
            voiceMessageData.voiceId = Cast::Details::parseData<std::uint32_t>(request);
            response.setData(reinterpret_cast<std::uint8_t*>(&voiceMessageData), sizeof(voiceMessageData));

            roomsManager.broadcastToMatchTeamExceptSelf(session->getId(), response, session->m_team);
        }
    }
}

#endif
