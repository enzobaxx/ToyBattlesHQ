#ifndef CLAN_PARTYLIST_HANDLER_H
#define CLAN_PARTYLIST_HANDLER_H

#include "Network/Sessions/MainSession.h"
#include "Managers/PartiesManager.h"
#include "Network/Packet.h"

namespace Main
{
	namespace Handlers
	{
		// This is used for showing the current matches for a single clan going on (i.e. when one clicks on "Clan Match")
        inline void handlePartyList(const Common::Network::Packet& request, std::shared_ptr<Main::Network::Session> session, Main::Classes::PartiesManager& clansManager)
        {
            auto response = request;
            response.setTcpHeader(request.getSession(), Common::Enums::NO_ENCRYPTION);
            response.setExtra(37);

            if (auto* selfClansList = clansManager.getRoomsFor(session->getAccountInfo().clanId))
            {
                std::vector<Main::Structures::PartyInfo> selfClanInfoList;
                response.setOption(selfClansList->size());
                for (const auto& currentClanMatchInfo : *selfClansList)
                {
                    selfClanInfoList.push_back(currentClanMatchInfo->getClanMatchInfo());
                }
                response.setData(reinterpret_cast<std::uint8_t*>(selfClanInfoList.data()), sizeof(Main::Structures::PartyInfo) * selfClanInfoList.size());
                session->asyncWrite(response);
            }
        }
	}
}

#endif