#ifndef MAIN_ITEM_INFO_H
#define MAIN_ITEM_INFO_H

#include <cstdint>
#include "Structures/Item/MainItemSerialInfo.h"
#include "Structures/Item/SpawnedItem.h"
#include "Structures/Item/MainBoughtItem.h"
#include "Structures/Item/MainEquippedItem.h"
#include "Structures/Capsule/CapsuleSpin.h"
#include "Detail/CdbUtils.h"
#include "Structures/Mailbox/Mailbox.h"
#include "Structures/TradeSystem/TradeSystemItem.h"
#include "Macros.h"

namespace Main
{
	namespace Structures
	{
PACK_PUSH(1)
		struct Item
		{
			// note: If itemnumber = 0 AND creationDate = 0 ==> basic item
			Main::Structures::ItemId itemId;
           	time32_t expirationDate{};
			ItemSerialInfo serialInfo{};
			std::uint16_t durability{};
			std::uint16_t energy{};
			std::uint32_t isSealed{};
			std::uint32_t sealLevel{}; // num seals needed for a given item
			std::uint32_t experienceEnhancement{};
			std::uint32_t mpEnhancement{};
			std::uint32_t unknown{}; // we use this to check whether an item is a coupon (which isn't calculated in the total inventory space used)

			explicit Item(std::uint32_t itemId)
				: itemId{itemId}
			{
			}

			explicit Item(const Main::Structures::CapsuleSpin& capsuleItem)
				: itemId{ capsuleItem.itemId }, serialInfo{ capsuleItem.itemSerialInfo }
				, durability{ Main::CdbUtils::getItemDurability(capsuleItem.itemId.itemId ).value_or(0) }
                , expirationDate{ static_cast<time32_t>(capsuleItem.expirationDate) }
			{
				serialInfo.itemOrigin = Main::Enums::ItemFrom::SHOP;
			}

			explicit Item(const Main::Structures::BoughtItem& boughtItem)
				: itemId{ boughtItem.itemId }, serialInfo{ boughtItem.serialInfo }
				, durability{ Main::CdbUtils::getItemDurability(boughtItem.itemId.itemId).value_or(0) }
			{
				const std::uint32_t duration = Main::CdbUtils::getItemDuration(boughtItem.itemId.itemId); 
				expirationDate = duration <= 3 ? duration : serialInfo.itemCreationDate + duration;
				serialInfo.itemOrigin = Main::Enums::ItemFrom::SHOP;
			}

			/*explicit removed on purpose*/ Item(const Main::Structures::EquippedItem& equippedItem)
				: itemId{ equippedItem.id }, serialInfo{ equippedItem.serialInfo }
				, durability{ equippedItem.durability }, energy{equippedItem.energy}, isSealed { equippedItem.isSealed }, sealLevel{equippedItem.sealLevel}
				, experienceEnhancement{ equippedItem.experienceEnhancement }, mpEnhancement{ equippedItem.mpEnhancement }
              	, expirationDate{ static_cast<time32_t>(equippedItem.expirationDate) }
			{
			}

			Item(const Main::Structures::SpawnedItem& spawnedItem)
				: itemId{ spawnedItem.itemId }, serialInfo{ spawnedItem.serialInfo } 
				, durability{ Main::CdbUtils::getItemDurability(spawnedItem.itemId.itemId).value_or(0) }
              	, expirationDate{ static_cast<time32_t>(spawnedItem.expirationDate ) }
			{
				serialInfo.itemOrigin = Main::Enums::ItemFrom::SHOP;
				itemId.stock = spawnedItem.itemId.stock;
			}

			explicit Item(const Main::Structures::BoxItem& boxItem)
				: itemId{ boxItem.itemId }, serialInfo{ boxItem.serialInfo }, expirationDate{ boxItem.expirationDate }
				, durability{ Main::CdbUtils::getItemDurability(boxItem.itemId.itemId).value_or(0) }
			{
				serialInfo.itemOrigin = Main::Enums::ItemFrom::SHOP;
			}

			/*explicit*/ Item(const Main::Structures::Giftbox2& giftItem)
				: itemId{ giftItem.itemId }, serialInfo{ giftItem.serialInfo }
				, durability{ Main::CdbUtils::getItemDurability(giftItem.itemId.itemId).value_or(0) }
			{
				// on purpose, in case of failure the item is expired (same as before)
				const std::uint32_t duration = Main::CdbUtils::getItemDuration(giftItem.itemId.itemId);
				expirationDate = duration <= 3 ? duration : serialInfo.itemCreationDate + duration;
				serialInfo.itemOrigin = 8;// Main::Enums::ItemFrom::SHOP;
			}

			Item(const Main::Structures::TradeBasicItem& tradedItem)
				: itemId{ tradedItem.itemId }, serialInfo{ tradedItem.itemSerialInfo }
			{
				expirationDate = 0; // trade only works with unlimited items
				durability = Main::CdbUtils::getItemDurability(tradedItem.itemId.itemId).value_or(0);
				serialInfo.itemOrigin = 8;// Main::Enums::ItemFrom::SHOP;
			}
		};
PACK_POP()

		struct ItemLogInfo
		{
			std::uint64_t itemNumber{};
			std::uint64_t itemId;
			std::int64_t expirationDate{};
			std::string action;
		};
	}
}
#endif
