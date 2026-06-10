#ifndef ITEM_AND_CAPSULE_SPIN_HANDLER_H
#define ITEM_AND_CAPSULE_SPIN_HANDLER_H

#include "../../../include/Network/MainSession.h"
#include "Network/Packet.h"
#include "../../Detail/CdbUtils.h"
#include <Utils/Utils.h>

namespace Main
{
	namespace Handlers
	{
		inline std::optional<std::vector<std::uint32_t>> itemSelectionAlgorithm(std::uint32_t boxItemId)
		{
			if (auto entries = Common::ConstantDatabase::CdbSingleton<Common::ConstantDatabase::CdbItemPackageInfo>::getInstance().getEntriesFor(boxItemId))
			{
				std::vector<std::uint32_t> itemIDs; itemIDs.reserve(entries->size());
				std::vector<double> probabilities; probabilities.reserve(entries->size());
				std::vector<std::uint32_t> selectedItems;

				static thread_local std::random_device rd;
				static thread_local std::mt19937 gen(rd());

				for (const auto& item : *entries)
				{
					if (item.ip_prob == Common::Constants::maxItemProbability) 
					{ // this item *must* be given in any case, e.g. 10-pack super glues
						selectedItems.emplace_back(item.ip_itemid);
					}
					else
					{
						const double probability = static_cast<double>(item.ip_prob) / 200'000 * 100.0;
						itemIDs.emplace_back(item.ip_itemid);
						probabilities.push_back(probability);
					}
				}
				if (!probabilities.empty())
				{
					std::discrete_distribution<int> itemDistribution(probabilities.begin(), probabilities.end());
					selectedItems.emplace_back(itemIDs[itemDistribution(gen)]);
				}
				return selectedItems;
			}
			return std::nullopt;
		}

		inline void handleGeneralItem(const Common::Network::Packet& request, std::shared_ptr<Main::Network::Session> session,
			const ClientData::BoxOpen& boxData,
			const std::unordered_map<std::uint32_t, std::function<void(std::shared_ptr<Main::Network::Session>)>> generalItemCallbacks,
			const std::unordered_map<std::uint32_t, std::function<void(std::shared_ptr<Main::Network::Session>)>>& cashItemCallbacks,
			const std::unordered_map<std::uint32_t, std::function<bool(std::shared_ptr<Main::Network::Session>, const Main::Structures::ItemSerialInfo&,
				const Common::Network::Packet&)>>& matchItemIds)
		{
			START_BENCHMARK

			const auto& userItems = session->getPlayer().getInventory().getItems();
			if (auto it = userItems.find(boxData.serialInfo.itemNumber); it != userItems.end())
			{
				const Main::Structures::Item boxItem = it->second;

				if (auto callbackIt = generalItemCallbacks.find(boxItem.itemId.itemId); callbackIt != generalItemCallbacks.end())
				{ // "normal" general item, e.g. K/D reset, battery
					callbackIt->second(session);
					auto response = request;
					response.setCommand(102, 1, 1, 0);
					session->asyncWrite(response);
					session->deleteItem(boxItem.serialInfo, "The item was deleted automatically after it was used (example: K/D reset item)");
				}
				else if (auto callbackIt = matchItemIds.find(boxItem.itemId.itemId); callbackIt != matchItemIds.end())
				{
					callbackIt->second(session, boxItem.serialInfo, request);
				}
				else if (auto wonItemIdsOpt = itemSelectionAlgorithm(boxItem.itemId.itemId); wonItemIdsOpt)
				{ // box item
					std::vector<Main::Structures::BoxItem> boxWonItems;
					Main::Structures::ItemSerialInfo wonItemSerialInfo;
					std::uint64_t latestItemNumber = session->getPlayer().getInventory().getLatestItemNumber();

					for (std::uint32_t currentWonItemId : *wonItemIdsOpt)
					{
						if (auto callbackIt = cashItemCallbacks.find(currentWonItemId); callbackIt != cashItemCallbacks.end())
						{ // cash item: mp/coin/coupon
							if (!session->deleteItem(boxItem.serialInfo, "The item was deleted after being used (example: MP/coin/coupon items"))
							{
								session->sendMessage("Debug) Error while deleting opened cash item");
							}
							callbackIt->second(session);
						}
						else
						{ // normal won item
							wonItemSerialInfo.itemNumber = ++latestItemNumber;
							const std::uint32_t duration = CdbUtils::getItemDuration(currentWonItemId);
							boxWonItems.emplace_back(currentWonItemId, duration <= 3 ? duration : wonItemSerialInfo.itemCreationDate + duration,
								wonItemSerialInfo);
						}
					}
					session->addItems(boxWonItems, boxData, request.getExtra());
				}
				else
				{
					session->sendMessage("[handleGeneralItem] This item is not implemented yet. (ItemID: " + std::to_string(boxItem.itemId.itemId));
				}
			}

			END_BENCHMARK(handleGeneralItem, session)
		}
	}
}

#endif