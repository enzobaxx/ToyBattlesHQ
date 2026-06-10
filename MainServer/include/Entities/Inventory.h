#ifndef INVENTORY_CLASS_H
#define INVENTORY_CLASS_H

#include "Structures/Item/MainEquippedItem.h"
#include "Structures/Item/MainBoughtItem.h"
#include "Structures/AccountInfo/MainAccountInfo.h"
#include "Structures/Item/MainItem.h"
#include "Persistence/MainScheduler.h"
#include "Structures/TradeSystem/TradeSystemItem.h"
#include "Structures/ClientData/Structures.h"

#include <unordered_map>
#include <vector>
#include <array>
#include <optional>

namespace Main
{
	namespace Classes
	{
		class Inventory
		{
		private:
			using Item = Main::Structures::Item;
			using EquippedItem = Main::Structures::EquippedItem;
			using DetailedEquippedItem = Main::Structures::DetailedEquippedItem;
			using BoughtItem = Main::Structures::BoughtItem;
			using AccountInfo = Main::Structures::AccountInfo;
			using TradedItem = Main::Structures::TradeBasicItem;

			AccountInfo& m_accountInfo;
			std::unordered_map<std::uint64_t, Item> m_itemsByItemNumber{};
			std::array<EquippedItem, Common::Enums::MAX_ITEMTYPE* Common::Enums::MAX_CHARACTERS> m_equippedItemByCharacter{}; // originally 2D, then flattened
			std::uint64_t m_totalEquippedItems{};
			std::vector<Item> m_couponItems;

		public:
			explicit Inventory(AccountInfo& accountInfo) : m_accountInfo(accountInfo) {}

			bool hasEnoughInventorySpace(std::uint16_t totalNewItems) const;
			Main::ClientData::CouponItemAddRet addCoupon(std::uint32_t stockToAdd);
			void addTotalCouponItems(const Item& item);
			Main::ClientData::CouponItemUseRet tryRemoveCoupons(std::uint32_t totalCouponsNeeded);
			std::size_t getTotalCoupons() const noexcept;

			void setUnequippedItems(const std::vector<Item>& items);
			bool isItemTradeable(const Main::Structures::ItemSerialInfo& itemSerialInfo) const;
			std::optional<std::uint32_t> findItemIdBySerialInfo(const Main::Structures::ItemSerialInfo& itemSerialInfo) const;
			std::optional<Main::Structures::ItemSerialInfo> getBossBattleTicket() const;
			std::optional<std::pair<std::uint32_t, std::uint32_t>>
			findItemIdAndDurabilityBySerialInfo(const Main::Structures::ItemSerialInfo& itemSerialInfo) const;
			std::optional<std::uint64_t> findMaxItemNumber() const;
			bool prolongItem(const Main::Structures::ItemSerialInfo& newItemSerialInfo);
			const std::array<EquippedItem, Common::Enums::MAX_CHARACTERS* Common::Enums::MAX_ITEMTYPE>& getEquippedItems() const;
			std::vector<EquippedItem> getEquippedItemsFor(std::uint16_t characterID) const;
			std::vector<EquippedItem> getUnlimitedEquippedWeaponsFor(std::uint16_t characterID) const;
			const std::unordered_map<std::uint64_t, Item>& getItems() const;
			const std::vector<Item> getItemsAsVec() const;
			bool deleteItemBasic(const Main::Structures::ItemSerialInfo& itemSerialInfo);
			void addItems(const std::vector<Item>& items);
			void addItem(const Item& item);
			void addItems(const std::vector<BoughtItem>& boughtItems);
			void addItems(const std::vector<Main::Structures::BoxItem>& boxItems);
			void setEquippedItems(const std::unordered_map<std::uint16_t, std::vector<EquippedItem>>& equippedItems);
			std::optional<std::pair<std::uint16_t, std::uint64_t>>
				addEnergyToItem(const Main::Structures::ItemSerialInfo& itemSerialInfo, std::uint32_t energyAdded);
			std::pair<std::vector<Main::ClientData::SingleWeaponDurabilityDamage>,
				std::vector<std::pair<std::uint32_t, std::uint64_t>>> reduceEquippedItemsDurabilities(std::size_t characterID, std::uint32_t weaponRestriction);
			bool updateItemDurabilityByNumber(std::uint32_t itemNumber, std::uint32_t newDurability);
			std::optional<std::uint16_t> getItemEnergy(const Main::Structures::ItemSerialInfo& itemSerialInfo) const;

			// ugly design but easier to write, ideally we shouldn't pass the scheduler to this function...
			void equipItem(const std::uint16_t itemNumber, Main::Persistence::MainScheduler& scheduler, std::uint32_t character = -1);
			std::optional<std::uint64_t> unequipItem(uint64_t itemType, Main::Persistence::MainScheduler& scheduler);
			std::uint64_t getTotalEquippedItems() const;

			void unequipItemImpl(std::uint64_t itemType, Main::Persistence::MainScheduler& scheduler, std::uint32_t character = -1);

			std::uint64_t getLatestItemNumber() const;
			std::pair<Common::Enums::MatchItemAction, std::uint32_t> useInstantRespawn(std::uint64_t itemNum);
			void setLatestItemNumber(std::uint64_t itemNum);
			std::pair<std::array<std::uint32_t, 10>, std::array<std::uint32_t, 7>> getEquippedItemsSeparated() const;
			bool unequipItemIfEquipped(std::uint64_t itemNumber, std::uint32_t characterId, Main::Persistence::MainScheduler& scheduler);
			void equipItemIfNotEquipped(std::uint64_t itemNumber, std::uint32_t characterId, Main::Persistence::MainScheduler& scheduler);

			// Trade system
			std::vector<Item> addItems(const std::vector<Main::Structures::TradeBasicItem>& tradedItems);
			Item addItemFromTrade(TradedItem tradeItem);
		};
	}
}

#endif
