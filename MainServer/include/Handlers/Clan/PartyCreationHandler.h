#ifndef CLAN_PARTY_CREATION_HANDLER_H
#define CLAN_PARTY_CREATION_HANDLER_H

#include "Network/Sessions/MainSession.h"
#include "Network/Packet.h"
#include "Managers/PartiesManager.h"
#include "Structures/Clan/ClanStructures.h"
#include "Rooms/PartyRoom.h"

namespace Main
{
	namespace Handlers
	{
		enum ClanMatchCreationExtra
		{
			CLAN_MATCH_CREATE_SUCCESS = 1,
			CLAN_MATCH_CREATE_EMPTY = 6, // "Cannot create ClanBattle"
			CLAN_MATCH_CREATE_FAIL = 7,  // nothing happens, use 6 
		};

        // Reviewed and tested 20.04.2026
        inline void handlePartyCreation(const Common::Network::Packet& request, std::shared_ptr<Main::Network::Session> session, Main::Classes::PartiesManager& partiesManager)
        {
            Main::ClientData::ClanRoomSettings clientReq = Details::parseData<Main::ClientData::ClanRoomSettings>(request);

            Common::Network::Packet response = request;
            response.setTcpHeader(request.getSession(), Common::Enums::NO_ENCRYPTION);

            if (const std::optional<std::uint16_t> nextRoomNumberOpt = partiesManager.getNextAvailableRoomNumberFor(session->getAccountInfo().clanId))
            {
                auto createdRoom = std::make_shared<Main::Classes::PartyRoom>(session, *nextRoomNumberOpt, clientReq);
                const auto roomIdPair = createdRoom->getRoomId();
                partiesManager.addRoom(createdRoom);
                response.setExtra(ClanMatchCreationExtra::CLAN_MATCH_CREATE_SUCCESS);
                response.setData(reinterpret_cast<const std::uint8_t*>(&roomIdPair), sizeof(roomIdPair));
            }
            else
            {
                session->sendMessage("Error: Only 4 simultaneous clan matches per clan are allowed");
                response.setExtra(ClanMatchCreationExtra::CLAN_MATCH_CREATE_EMPTY);
                response.setData(nullptr, 0);
            }

            session->asyncWrite(response);
        }
	}
}

#endif