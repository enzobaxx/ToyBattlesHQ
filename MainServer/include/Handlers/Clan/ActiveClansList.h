

#ifndef ACTIVE_CLANS_LIST_HANDLER_H
#define ACTIVE_CLANS_LIST_HANDLER_H

#include "Network/Sessions/MainSession.h"
#include "Network/Packet.h"
#include "Managers/PartiesManager.h"

namespace Main
{
	namespace Handlers
	{
		template<std::size_t OrderId>
		inline void handleActiveClansList(const Common::Network::Packet& request, std::shared_ptr<Main::Network::Session> session,
			Main::Classes::PartiesManager& clansManager)
		{
			const auto& ainfo = session->getAccountInfo();

			if constexpr (OrderId == 112)
			{ // this wasn't fully working on the official versions either...
				auto response = request;
				response.setExtra(6);
				response.setData(nullptr, 0);

				if (auto clanRoom = clansManager.getExactRoomFor(ainfo.clanId, session->getPlayer().getMatchContext().partyRoomNumber))
				{
					clanRoom->broadcast(response);
				}
			}
			else if constexpr (OrderId == 113)
			{
				if (auto clanRoom = clansManager.getExactRoomFor(ainfo.clanId, session->getPlayer().getMatchContext().partyRoomNumber))
				{
					auto response = request;
					const auto allRegisteredClans = clansManager.getAllRegisteredClans();
					response.setExtra(37); // 37 is enough assuming maxRooms = 30
					response.setOption(allRegisteredClans.size()); // num of clans waiting for a pairing
					response.setData(reinterpret_cast<const std::uint8_t*>(allRegisteredClans.data()), allRegisteredClans.size() *
						sizeof(Main::Structures::RegisteredClanInfo));
					clanRoom->broadcast(response);
				}
			}
		}
	}
}

#endif