#ifndef TRADE_ACK_HANDLER_HEADER
#define TRADE_ACK_HANDLER_HEADER

#include "Network/Sessions/MainSession.h"
#include "Network/MainSessionManager.h"
#include "Enums/GameEnums.h"
#include "MainEnums.h"
#include "Structures/TradeSystem/TradeAck.h"
#include "Network/Packet.h"
#include <vector>
#include <cstdint>

namespace Main
{
	namespace Handlers
	{
		inline void handleTradeAck(const Common::Network::Packet& request, std::shared_ptr<Main::Network::Session> session, 
			Main::Network::SessionsManager& sessionsManager,
			const Main::Structures::EventMissionInfo& tradeInfo)
		{

			Common::Network::Packet response;
			response.setTcpHeader(request.getSession(), Common::Enums::USER_LARGE_ENCRYPTION);
			response.setOrder(request.getOrder());

			const std::uint64_t now = static_cast<std::uint64_t>(std::chrono::duration_cast<std::chrono::seconds>(std::chrono::system_clock::now().time_since_epoch()).count());
			if (now >= tradeInfo.startDate && now <= tradeInfo.endDate)
			{
				const std::uint32_t targetAccountId = Main::Details::parseData<std::uint32_t>(request, sizeof(std::uint32_t));
				const auto& selfAccountInfo = session->getAccountInfo();
				if (targetAccountId == selfAccountInfo.accountID)
				{ // prevent trading with self
					session->sendMessage("You cannot trade with yourself!");
					return;
				}
				
				if (session->getPlayer().playerState != Common::Enums::STATE_LOBBY && session->getPlayer().matchContext.roomNumber != 0)
				{
					session->sendMessage("You must be inside the lobby to trade!", Main::Enums::TIP);
					return;
				}

				Main::Structures::TradeAck tradeAck{ selfAccountInfo.uniqueId, selfAccountInfo.accountID };
				response.setData(reinterpret_cast<std::uint8_t*>(&tradeAck), sizeof(tradeAck));

				if (auto targetSession = sessionsManager.getSessionByAccountId(targetAccountId))
				{
					auto targetPlayerState = targetSession->getPlayer().playerState;
					const auto& targetAccountInfo = targetSession->getAccountInfo();

					if (targetAccountInfo.playerLevel < 16 || selfAccountInfo.playerLevel < 16)
					{
						response.setOrder(192);
						response.setExtra(Enums::TradeSystemExtra::LEVEL_TOO_LOW);
					}
					else if (targetPlayerState == Common::Enums::PlayerState::STATE_LOBBY && targetSession->getPlayer().matchContext.roomNumber == 0)
					{
						// Todo: check whether this branch is entered in case the player is in a room in a non-ready state
						targetSession->asyncWrite(response);
						return;
					}
					else
					{
						response.setOrder(192);
						response.setExtra(Enums::TradeSystemExtra::CANNOT_TRADE_NOW_OR_PLAYER_OFFLINE);
					}
					session->asyncWrite(response);
				}
			}
			else
			{
				response.setOrder(192);
				response.setExtra(15);
				session->asyncWrite(response);
			}
		}
	}
}

#endif