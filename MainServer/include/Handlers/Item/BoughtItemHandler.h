
#ifndef BOUGHT_ITEM_HANDLER_H
#define BOUGHT_ITEM_HANDLER_H

#include <array>
#include <bit>
#include <unordered_map>
#include "Network/Sessions/MainSession.h"
#include "Structures/Item/MainBoughtItem.h"
#include "Detail/CdbUtils.h"
#include "MainEnums.h"
#include "Structures/Item/MainItem.h"
#include "Structures/AccountInfo/MainAccountInfo.h"
#include <Utils/Utils.h>
#include "Detail/Utilities.h"

namespace Main
{
	namespace Handlers
	{

		inline void handleBoughtItem(const Common::Network::Packet& request, std::shared_ptr<Main::Network::Session> session)
		{
			START_BENCHMARK

			using ItemSerialInfo = Main::Structures::ItemSerialInfo;
			const auto& player = session->getPlayer();
			const auto& ainfo = player.getAccountInfo();
			std::array<std::int32_t, Main::Enums::TOTAL_CURRENCIES> totalCurrencySpentByType{};
			std::array<std::int32_t, Main::Enums::TOTAL_CURRENCIES> sessionCurrencyByType = { ainfo.microPoints, ainfo.rockTotens, 0 /*coupons*/, ainfo.coins };

			if (request.getMission() == 1) 
			{ // item is expired and the user wants to renew it
				std::vector<Main::Structures::BoughtItemToProlong> itemsToProlong; itemsToProlong.reserve(request.getOption());
				std::vector<std::uint64_t> itemDurations; itemDurations.reserve(request.getOption());

				for (std::uint32_t idx = 0; idx < request.getOption(); ++idx)
				{
					const ItemSerialInfo itemSerialInfoToProlong = Main::Details::parseData<ItemSerialInfo>(request, idx * 8); 
					const auto itemIdOpt = player.getInventory().findItemIdBySerialInfo(itemSerialInfoToProlong);
					if (!itemIdOpt)
					{
						session->sendMessage("[handleBoughtItem] error: bought item not found through serial info");
						return;
					}
					if (!Common::ConstantDatabase::CdbSingleton<Common::ConstantDatabase::CdbVendorInfo>::itemExistsInShop(*itemIdOpt))
					{
						session->sendMessage("[handleBoughtItem] error: modified vendorinfo.cdb found");
						return;
					}

					if (const auto amountByCurrencyTypeOpt = CdbUtils::getItemPrice(*itemIdOpt); amountByCurrencyTypeOpt)
					{
						auto& currentCurrency = sessionCurrencyByType[amountByCurrencyTypeOpt->first];
						if (currentCurrency < amountByCurrencyTypeOpt->second)
						{
							return;
						}
						currentCurrency -= amountByCurrencyTypeOpt->second;
						totalCurrencySpentByType[amountByCurrencyTypeOpt->first] += amountByCurrencyTypeOpt->second;
						itemsToProlong.emplace_back(Main::Structures::BoughtItemToProlong{ itemSerialInfoToProlong });
						itemDurations.push_back(CdbUtils::getItemDuration(*itemIdOpt));
					}
					else
					{
						session->sendMessage("[handleBoughtItem] error: item cost or duration was nullopt");
						return;
					}
				}
				if (session->prolongItems(itemsToProlong, itemDurations))
				{
					session->setAccountMicroPoints(ainfo.microPoints - totalCurrencySpentByType[Main::Enums::ItemCurrencyType::ITEM_MP]);
					session->setAccountRockTotens(ainfo.rockTotens - totalCurrencySpentByType[Main::Enums::ItemCurrencyType::ITEM_RT]);
				}
			}
			else 
			{ // buy new item
				std::uint64_t latestItemNumber = player.getInventory().getLatestItemNumber();
				std::vector<Main::Structures::BoughtItem> boughtItems; 

				for (std::uint32_t idx = 0; idx < request.getOption(); ++idx)
				{
					const std::uint32_t itemId = Main::Details::parseData<std::uint32_t>(request, idx * 4); 
					Main::Structures::BoughtItem boughtItem(itemId);
					if (!Common::ConstantDatabase::CdbSingleton<Common::ConstantDatabase::CdbVendorInfo>::itemExistsInShop(boughtItem.itemId.itemId))
					{
						session->sendMessage("[handleBoughtItem] Modified vendorinfo.cdb found");
						return;
					}

					if (const auto amountByCurrencyType = CdbUtils::getItemPrice(boughtItem.itemId.itemId); amountByCurrencyType)
					{
						auto& currentCurrency = sessionCurrencyByType[amountByCurrencyType->first];
						if (currentCurrency < amountByCurrencyType->second)
						{
							::Utils::Logger::log("currentCurrency < amountByCurrencyType->second", ::Utils::LogType::Warning);
							return;
						}
						currentCurrency -= amountByCurrencyType->second;
						totalCurrencySpentByType[amountByCurrencyType->first] += amountByCurrencyType->second;
						boughtItem.serialInfo.itemNumber = ++latestItemNumber;
						boughtItems.push_back(boughtItem);
					}
					else
					{
						session->sendMessage("[handleBoughtItem] Item cost not found inside cdb files");
						return;
					}
				}
				if (session->addItems(boughtItems))
				{
					session->setAccountMicroPoints(ainfo.microPoints - totalCurrencySpentByType[Main::Enums::ItemCurrencyType::ITEM_MP]);
					session->setAccountRockTotens(ainfo.rockTotens - totalCurrencySpentByType[Main::Enums::ItemCurrencyType::ITEM_RT]);
				}
			}

			END_BENCHMARK(Handlers::handleBoughtItem, session)
		}


		inline void handleCouponBoughtItem(const Common::Network::Packet& request, std::shared_ptr<Main::Network::Session> session)
		{
			const auto& player = session->getPlayer();
			std::uint64_t latestItemNumber = player.getInventory().getLatestItemNumber();
			std::vector<Main::Structures::BoughtItem> boughtItems;
			std::uint32_t totalCouponsSpent = 0;

			for (std::uint32_t idx = 0; idx < request.getOption(); ++idx)
			{
				const std::uint32_t itemId = Main::Details::parseData<std::uint32_t>(request, idx * 4);
				Main::Structures::BoughtItem boughtItem(itemId, true);
				
				if (!Common::ConstantDatabase::CdbSingleton<Common::ConstantDatabase::CdbVendorInfo>::itemExistsInShop(itemId))
				{
					session->sendMessage("[handleCouponBoughtItem] Modified vendorinfo.cdb found");
					return;
				}

				if (const auto amountByCurrencyType = CdbUtils::getItemPrice(itemId); amountByCurrencyType)
				{
					if (amountByCurrencyType->first != Main::Enums::ItemCurrencyType::ITEM_COUPON) return;
					totalCouponsSpent += amountByCurrencyType->second;
					boughtItem.serialInfo.itemNumber = ++latestItemNumber;
					boughtItems.push_back(boughtItem);
				}
				else
				{
					session->sendMessage("[handleCouponBoughtItem] Item cost not found inside cdb files");
					return;
				}
			}

			if (bool removed = session->tryRemoveCoupons(totalCouponsSpent); removed && session->addItems(boughtItems, true))
			{ // tba?
			}
			else
			{
				session->sendMessage("Insufficient coupons. Relog to see the updated coupons count", Main::Enums::TIP);
			}
		}
	}
}
#endif

