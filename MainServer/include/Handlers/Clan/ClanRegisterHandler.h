#ifndef CLANMATCH_REGISTER_HANDLER_H
#define CLANMATCH_REGISTER_HANDLER_H

#include "Network/Sessions/MainSession.h"
#include "Managers/PartiesManager.h"
#include "Network/Packet.h"

namespace Main
{
	namespace Handlers
	{
		enum ClanRegisterExtra
		{
			// Order 115
			REGISTER_CLAN_MAX_BUSY = 5, // "Party cannot be started because a game has started" ?
			REGISTER_CLAN_WRONGPASSWORD = 35,
			REGISTER_CLAN_SUCCESS = 44,
			REGISTER_CLAN_UNKNOWN = 45, // nothing happens with this

			// Order 120
			ROOM_CHAT_CLAN_CUSTOM_LISTUP = 44, // "Logged into custom match"
			ROOM_CHAT_CLAN_CUSTOM_LISTDOWN = 45, // "Cancelled custom match"
		};

		// Reviewed 22.04.2026
		template<std::size_t OrderId>
		inline void handleClanRegister(const Common::Network::Packet& request, std::shared_ptr<Main::Network::Session> session, Main::Classes::PartiesManager& clansManager)
		{
			auto response = request;
			response.setTcpHeader(request.getSession(), Common::Enums::NO_ENCRYPTION);
			const auto& ainfo = session->getAccountInfo();

			if (clansManager.getAllRegisteredClans().size() >= Common::Constants::maxClanRooms)
			{
				session->sendMessage("[INFO] Cannot register since there are already 30 registered clans, which is the current maximum");
				return;
			}
			else if (auto clanRoom = clansManager.getExactRoomFor(ainfo.clanId, session->getPlayer().getMatchContext().partyRoomNumber))
			{
				if (!clanRoom->isLeader(ainfo.uniqueId.session))
				{
					session->sendMessage("Error: Cannot register a clan if you're not the leader. If you think that this is a bug, make sure to report it");
					return;
				}
				if (!clanRoom->canRegister())
				{
					response.setExtra(ClanRegisterExtra::REGISTER_CLAN_UNKNOWN);
					session->asyncWrite(response);
					return;
				}

				if constexpr (OrderId == 115)
				{
					if (request.getExtra() == 45)
					{
						response.setExtra(ClanRegisterExtra::REGISTER_CLAN_SUCCESS);
						response.setData(reinterpret_cast<const std::uint8_t*>(session->getAccountInfo().nickname), Common::Constants::maxNicknameSize);
						session->asyncWrite(response);
					}
				}
				else if constexpr (OrderId == 120)
				{
					clanRoom->switchRegistered();
					response.setExtra(clanRoom->isRegistered() ? ClanRegisterExtra::ROOM_CHAT_CLAN_CUSTOM_LISTUP : ClanRegisterExtra::ROOM_CHAT_CLAN_CUSTOM_LISTDOWN);
					session->asyncWrite(response);
				}
			}
		}
	}
}

#endif