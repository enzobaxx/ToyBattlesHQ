#ifndef ITEM_ENERGY_INSERTION_HANDLER
#define ITEM_ENERGY_INSERTION_HANDLER

#include "Network/Sessions/MainSession.h"
#include "Network/MainSessionManager.h"
#include "Network/Packet.h"
#include "Structures/PlayerLists/Friend.h"
#include "Handlers/Item/DeleteItemHandler.h"
#include "Structures/Item/SpawnedItem.h"

#include <Utils/Utils.h>

namespace Main
{
	namespace Handlers
	{

        enum SpecialItems
        {
            SUPER_GLUE = 4305004,
            ENERGY_REFUND_100 = 4305016,
            ENERGY_REFUND_30 = 4305014,
            ENERGY_REFUND_50 = 4305015,
        };
		inline void handleItemUpgrade(const Common::Network::Packet& request, std::shared_ptr<Main::Network::Session> session)
		{
			START_BENCHMARK

			if (session->getPlayer().getPlayerState() != Common::Enums::STATE_INVENTORY)
			{
				session->sendMessage("Error: An item can be upgraded only while your state is STATE_INVENTORY!");
				return;
			}

			const auto receivedExtra = request.getExtra();
			if (receivedExtra == 0) 
			{ // Client ACK: user adds energy to a weapon
				const Main::ClientData::ItemAddEnergy itemAddEnergy = Main::Details::parseData<Main::ClientData::ItemAddEnergy>(request);
				if (itemAddEnergy.usedEnergy > session->getAccountInfo().battery) return;
				session->addEnergyToItem(itemAddEnergy, request.getOption(), request.getMission());
			}
            else if (receivedExtra == 37 && request.getMission() < Enums::UPGRADE_TYPE_MAX)
            { // upgrade
                std::vector<Main::Structures::ItemSerialInfo> itemSerialInfos;
                if (request.getDataSize() % 8 != 0) return;
                const std::uint32_t maxLoops = request.getDataSize() / 8;
                for (std::uint32_t i = 0; i < maxLoops; ++i)
                {
                    itemSerialInfos.push_back(Main::Details::parseData<Main::Structures::ItemSerialInfo>(request, i * 8));
                }

                if (itemSerialInfos.empty())
                {
                    session->sendMessage("[handleItemUpgrade] error: no items in the upgrade request.");
                    return;
                }

                const auto& firstItemSerialInfo = itemSerialInfos.front(); 
                const auto firstItemIdOpt = session->getPlayer().inventory.findItemIdBySerialInfo(firstItemSerialInfo); // itemID of new weapon after upgrade
                if (!firstItemIdOpt)
                {
                    session->sendMessage("[handleItemUpgrade] error: itemIdOpt was nullopt for first item: " + std::to_string(firstItemSerialInfo.itemNumber));
                    return;
                }

                const auto upgradeInfo = Common::ConstantDatabase::CdbSingleton<Common::ConstantDatabase::CdbUpgradeInfo>::getInstance().getEntry(*firstItemIdOpt);
                if (!upgradeInfo)
                {
                    session->sendMessage("[handleItemUpgrade] error: upgradeInfo not found for itemID " + std::to_string(firstItemSerialInfo.itemNumber));
                    return;
                }

                bool useEnergyRefund = false;
                bool useGlue = false;
                for (const auto& itemSerialInfo : itemSerialInfos)
                {
                    const auto itemIdOpt = session->getPlayer().inventory.findItemIdBySerialInfo(itemSerialInfo);
                    if (!itemIdOpt)
                    {
                        session->sendMessage("[handleItemUpgrade] error: itemIdOpt was nullopt for item: " + std::to_string(itemSerialInfo.itemNumber));
                        return;
                    }
                    if (!useEnergyRefund)
                        useEnergyRefund = *itemIdOpt == SpecialItems::ENERGY_REFUND_100 || *itemIdOpt == SpecialItems::ENERGY_REFUND_30 
                        || *itemIdOpt == SpecialItems::ENERGY_REFUND_50;
                    if (!useGlue)
                        useGlue = *itemIdOpt == SpecialItems::SUPER_GLUE;
                }

                if (session->upgradeWeapon(*firstItemIdOpt, firstItemSerialInfo, upgradeInfo->ui_parentid, request.getMission(), 
                    request.getOption(), useEnergyRefund, useGlue))
                {
                    for (std::size_t i = 1; i < itemSerialInfos.size(); ++i)
                    {
                        session->deleteItem(itemSerialInfos[i], "Item deleted after it was used while upgrading a weapon (Example: Super Glue item");
                    }
                }
            }
			else if (receivedExtra == 53)
			{ // upgrade reset
				session->resetUpgrade(Main::Details::parseData<Main::ClientData::UpgradeReset>(request)); 
			}

			END_BENCHMARK(handleItemUpgrade, session)
		}
	}
}

#endif