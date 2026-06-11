#ifndef TRADE_REMOVE_ITEM_HANDLER_H
#define TRADE_REMOVE_ITEM_HANDLER_H

#include "Network/Sessions/MainSession.h"
#include "Network/MainSessionManager.h"
#include "MainEnums.h"
#include "Structures/TradeSystem/TradeSystemItem.h"
#include "Enums/GameEnums.h"
#include "Network/Packet.h"
#include <vector>
#include <cstdint>
#include <optional>

namespace Main
{
	namespace Handlers
	{
		inline void handleTradeItemRemoval(const Common::Network::Packet& request, std::shared_ptr<Main::Network::Session>session,
			Main::Network::SessionsManager& sessionsManager)
		{
			const Main::Structures::ItemSerialInfo itemSerialInfo = Main::Details::parseData<Main::Structures::ItemSerialInfo>(request, 8);

			Common::Network::Packet response;
			response.setTcpHeader(request.getSession(), Common::Enums::USER_LARGE_ENCRYPTION);
			response.setOrder(request.getOrder());

			std::uint32_t itemID = 0;
			const auto& itemId = session->getPlayer().inventory.findItemIdBySerialInfo(itemSerialInfo);
			if (itemId.has_value()) itemID = *itemId;
			else return;

			const auto& accountInfo = session->getAccountInfo();
			if (auto targetSession = sessionsManager.getSessionByAccountId(session->getPlayer().tradeInfo.getCurrentlyTradingWithAccountId()))
			{
				Main::Structures::TradeAddedItemDetailed tradeItem{ session->getAccountInfo().accountID, itemSerialInfo, itemID };			
				response.setExtra(Enums::TradeSystemExtra::TRADE_SUCCESS);
				session->getPlayer().tradeInfo.removeTradedItem(itemSerialInfo);
				response.setData(reinterpret_cast<std::uint8_t*>(&tradeItem), sizeof(tradeItem));
				session->asyncWrite(response);
				targetSession->asyncWrite(response);
			}
			else
			{
				response.setExtra(Enums::TradeSystemExtra::CANNOT_TRADE_NOW_OR_PLAYER_OFFLINE);
				session->asyncWrite(response);
				session->resetTradeInfo();
				session->setPlayerState(Common::Enums::PlayerState::STATE_INVENTORY);
			}
		}
	}
}

#endif