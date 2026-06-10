#ifndef CHARACTER_SELECTION_HANDLER_H
#define CHARACTER_SELECTION_HANDLER_H

#include "Network/Sessions/MainSession.h"
#include "Managers/RoomsManager.h"
#include "Network/Packet.h"
#include "MainEnums.h"
#include "Detail/Utilities.h"

namespace Main
{
	namespace Handlers
	{
		inline void handleCharacterSelection(const Common::Network::Packet& request, std::shared_ptr<Main::Network::Session> session,
			Main::Classes::RoomsManager& roomsManager)
		{
			const std::uint32_t selectedCharacter = static_cast<std::uint32_t>(request.getOption());
			const bool isCharacterAvailable = true; //selectedCharacter == Common::Enums::Naomi || selectedCharacter == Common::Enums::Pandora
				//|| selectedCharacter == Common::Enums::CHIP || selectedCharacter == Common::Enums::Knox || selectedCharacter == Common::Enums::Kai;

			Common::Network::Packet response;
			response.setTcpHeader(request.getSession(), Common::Enums::USER_LARGE_ENCRYPTION);
			response.setCommand(request.getOrder(), 0, isCharacterAvailable, request.getOption());
			session->asyncWrite(response);

			session->setAccountLatestCharacterSelected(request.getOption());
			Details::broadcastPlayerItems(roomsManager, session, request);
		}
	}
}

#endif