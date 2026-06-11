#ifndef TRADE_REQUEST_HANDLER_H
#define TRADE_REQUEST_HANDLER_H

#include "Network/Sessions/MainSession.h"
#include "Network/MainSessionManager.h"
#include "MainEnums.h"
#include "Structures/TradeSystem/TradePlayerInfo.h"
#include "Network/Packet.h"
#include <vector>
#include <cstdint>

namespace Main
{
	namespace Handlers
	{
		inline void sendInitialTradeInfo(std::shared_ptr<Main::Network::Session> session, std::shared_ptr<Main::Network::Session> targetSession, 
			Common::Network::Packet& response)
		{
			const auto& selfAccountInfo = session->getAccountInfo();
			const auto& equippedItems = session->getPlayer().inventory.getEquippedItemsFor(selfAccountInfo.latestSelectedCharacter);
			std::uint32_t selfEquippedHair = 0;
			std::uint32_t selfEquippedEyes = 0;

			const auto getEquippedItemId = [&](std::uint32_t itemType) -> std::uint32_t {
				for (const auto& current : equippedItems)
				{
					if (current.type == itemType)
					{
						if (current.serialInfo.itemNumber == 0) return 0;
						return current.id;
					}
				}
				return 0;
				};

			selfEquippedHair = getEquippedItemId(0);
			if (selfEquippedHair == 0) selfEquippedHair = getEquippedItemId(25);
			selfEquippedEyes = getEquippedItemId(1);

			Main::Structures::TradePlayerInfo tradePlayerInfo{ selfAccountInfo.accountID, static_cast<std::uint32_t>(selfAccountInfo.latestSelectedCharacter), 
				selfEquippedHair, selfEquippedEyes };
			response.setData(reinterpret_cast<std::uint8_t*>(&tradePlayerInfo), sizeof(tradePlayerInfo));
			targetSession->asyncWrite(response);
			targetSession->getPlayer().tradeInfo.setCurrentlyTradingWithAccountId(selfAccountInfo.accountID);
		}

		inline void handleTradeInitialization(const Common::Network::Packet& request, std::shared_ptr<Main::Network::Session> session, 
			Main::Network::SessionsManager& sessionsManager)
		{
			Common::Network::Packet response;
			response.setTcpHeader(request.getSession(), Common::Enums::USER_LARGE_ENCRYPTION);
			response.setOrder(request.getOrder());
			response.setExtra(request.getExtra());
			
			const std::uint32_t targetAccountId = Main::Details::parseData<std::uint32_t>(request, sizeof(std::uint32_t));

			if (auto targetSession = sessionsManager.getSessionByAccountId(targetAccountId))
			{
				const auto targetPlayerState = targetSession->getPlayer().getPlayerState();
				if (!session->getPlayer().socialInfo.isFriend(targetAccountId))
				{
					response.setExtra(Enums::TradeSystemExtra::PLAYERS_NOT_FRIENDS);
					session->asyncWrite(response);
				}
				else if (request.getExtra() == Enums::TradeSystemExtra::TRADE_DECLINED)
				{
					response.setExtra(Enums::TradeSystemExtra::TRADE_DECLINED);
					sendInitialTradeInfo(session, targetSession, response);
				}
				else if (targetPlayerState == Common::Enums::PlayerState::STATE_LOBBY && targetSession->getPlayer().isInLobby())
				{
					response.setExtra(Enums::TradeSystemExtra::TRADE_SUCCESS);
					session->setPlayerState(Common::Enums::STATE_TRADE);
					targetSession->setPlayerState(Common::Enums::STATE_TRADE);
					session->temporarilySealAllItems();
					targetSession->temporarilySealAllItems();
					sendInitialTradeInfo(targetSession, session, response);
					sendInitialTradeInfo(session, targetSession, response);
				}
				else
				{
					response.setExtra(Enums::TradeSystemExtra::CANNOT_TRADE_NOW_OR_PLAYER_OFFLINE);
					session->asyncWrite(response);
				}
			}
			
		}
	}
}

#endif