#ifndef TRADE_INFO_CLASS_H
#define TRADE_INFO_CLASS_H

#include "Structures/Item/ItemId.h"
#include "Structures/Item/MainItemSerialInfo.h"
#include "Structures/TradeSystem/TradeSystemItem.h"

#include <vector>
#include <cstdint>

namespace Main
{
	namespace Classes
	{
		class TradeInfo
		{
		private:
			using TradedItem = Main::Structures::TradeBasicItem;

			std::uint32_t m_currentlyTradingWithAccountId{};
			std::vector<TradedItem> m_tradedItems{};
			bool m_hasPlayerLocked{};

		public:
			void lockTrade();
			bool hasPlayerLocked() const;
			void reset();
			void setCurrentlyTradingWithAccountId(std::uint32_t targetAccountId);
			std::uint32_t getCurrentlyTradingWithAccountId() const;
			bool addTradedItem(std::uint32_t itemId, const Main::Structures::ItemSerialInfo& serialInfo);
			void removeTradedItem(const Main::Structures::ItemSerialInfo& serialInfo);
			void resetTradedItems();
			const std::vector<TradedItem>& getTradedItems() const;
		};
	}
}

#endif
