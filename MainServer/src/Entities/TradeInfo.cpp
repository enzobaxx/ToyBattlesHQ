#include "Entities/TradeInfo.h"

#include <algorithm>

namespace Main
{
	namespace Classes
	{
		using TradedItem = Main::Structures::TradeBasicItem;

		void TradeInfo::lockTrade()
		{
			m_hasPlayerLocked = true;
		}

		bool TradeInfo::hasPlayerLocked() const
		{
			return m_hasPlayerLocked;
		}

		void TradeInfo::reset()
		{
			m_hasPlayerLocked = false;
			m_tradedItems.clear();
			m_currentlyTradingWithAccountId = 0;
		}

		void TradeInfo::setCurrentlyTradingWithAccountId(std::uint32_t targetAccountId)
		{
			m_currentlyTradingWithAccountId = targetAccountId;
		}

		std::uint32_t TradeInfo::getCurrentlyTradingWithAccountId() const
		{
			return m_currentlyTradingWithAccountId;
		}

		bool TradeInfo::addTradedItem(std::uint32_t itemId, const Main::Structures::ItemSerialInfo& serialInfo)
		{
			auto it = std::find_if(m_tradedItems.begin(), m_tradedItems.end(),
				[&serialInfo](const Main::Structures::TradeBasicItem& item)
				{
					return item.itemSerialInfo.itemNumber == serialInfo.itemNumber;
				});

			if (it != m_tradedItems.end())
			{
				return false;
			}

			m_tradedItems.push_back(Main::Structures::TradeBasicItem{ itemId, serialInfo });
			return true;
		}

		void TradeInfo::removeTradedItem(const Main::Structures::ItemSerialInfo& serialInfo)
		{
			for (auto it = m_tradedItems.begin(); it != m_tradedItems.end(); ++it)
			{
				if (it->itemSerialInfo == serialInfo)
				{
					m_tradedItems.erase(it);
					return;
				}
			}
		}

		void TradeInfo::resetTradedItems()
		{
			m_tradedItems.clear();
		}

		const std::vector<TradedItem>& TradeInfo::getTradedItems() const
		{
			return m_tradedItems;
		}
	}
}
