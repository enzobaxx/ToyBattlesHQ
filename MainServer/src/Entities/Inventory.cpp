
#include "Entities/Inventory.h"
#include <unordered_map>
#include <vector>
#include <algorithm>
#include "ConstantDatabase/Structures/SetItemInfo.h"
#include "Utils/Utils.h"
#include "Utils/Constants.h"
#include <ranges>
#include <cstring>

namespace Main
{
	namespace Classes
	{
		using Item = Main::Structures::Item;
		using EquippedItem = Main::Structures::EquippedItem;
		using DetailedEquippedItem = Main::Structures::DetailedEquippedItem;
		using BoughtItem = Main::Structures::BoughtItem;
		using AccountInfo = Main::Structures::AccountInfo;
		using TradedItem = Main::Structures::TradeBasicItem;

		bool Inventory::hasEnoughInventorySpace(std::uint16_t totalNewItems) const
		{
			return (static_cast<std::int32_t>(m_accountInfo.inventorySpace) - m_totalEquippedItems + m_couponItems.size()
				- m_itemsByItemNumber.size()) >= totalNewItems;
		}

		Main::ClientData::CouponItemUseRet Inventory::tryRemoveCoupons(std::uint32_t totalCouponsNeeded)
		{
			auto it = std::find_if(m_itemsByItemNumber.begin(), m_itemsByItemNumber.end(),
				[](const auto& pair) { return pair.second.itemId.itemId == 1000000; });

			if (it != m_itemsByItemNumber.end())
			{
				Item& coupon = it->second;
				if (coupon.itemId.stock == 0)
				{
					return { Common::Enums::CouponItemAction::COUPON_ITEM_STOCK_IS_ZERO, 0, coupon.serialInfo };
				}
				else if (coupon.itemId.stock > totalCouponsNeeded)
				{
					coupon.itemId.stock -= totalCouponsNeeded;
					return { Common::Enums::CouponItemAction::COUPON_ITEM_STOCKS_REDUCED_SUCCESS, coupon.itemId.stock, coupon.serialInfo };
				}
				else if (coupon.itemId.stock == totalCouponsNeeded)
				{
					coupon.itemId.stock -= totalCouponsNeeded;
					return { Common::Enums::CouponItemAction::COUPON_ITEM_DELETE, 0, coupon.serialInfo };
				}
			}
			return { Common::Enums::CouponItemAction::COUPON_ITEM_DO_NOTHING, 0, {} };
		}

		Main::ClientData::CouponItemAddRet Inventory::addCoupon(std::uint32_t stockToAdd)
		{
			auto it = std::find_if(m_itemsByItemNumber.begin(), m_itemsByItemNumber.end(),
				[](const auto& pair) { return pair.second.itemId.itemId == 1000000; });

			if (it != m_itemsByItemNumber.end())
			{
				Item& coupon = it->second;
				if (coupon.itemId.stock == 0 || coupon.itemId.stock + stockToAdd > 250)
				{
					return { Common::Enums::AddCouponAction::UNKNOWN_COUPON_ADD_ERROR, 0, {} };
				}
				else
				{
					coupon.itemId.stock += stockToAdd;
					return { Common::Enums::AddCouponAction::EXISTING_COUPON_STOCK_UPDATED, coupon.itemId.stock, coupon.serialInfo };
				}
			}
			return { Common::Enums::AddCouponAction::MUST_CREATE_NEW_COUPON, 0, {} };
		}

		void Inventory::setUnequippedItems(const std::vector<Item>& items)
		{
			for (const auto& currentItem : items)
			{
				//m_latestItemNumber = std::max(m_latestItemNumber, currentItem.serialInfo.itemNumber);
				m_itemsByItemNumber.insert_or_assign(currentItem.serialInfo.itemNumber, currentItem);
				if (currentItem.itemId.itemId == 1000000) //if (currentItem.unknown)
				{
					m_couponItems.push_back(currentItem);
				}
			}
		}

		std::optional<std::uint32_t> Inventory::findItemIdBySerialInfo(const Main::Structures::ItemSerialInfo& itemSerialInfo) const
		{
			if (auto it = m_itemsByItemNumber.find(itemSerialInfo.itemNumber); it != m_itemsByItemNumber.end())
			{
				return it->second.itemId.itemId;
			}
			auto it = std::find_if(m_equippedItemByCharacter.begin(), m_equippedItemByCharacter.end(), [&itemSerialInfo](const auto& equippedItem)
				{
					return equippedItem.serialInfo.itemNumber == itemSerialInfo.itemNumber;
				});
			return it != m_equippedItemByCharacter.end() ? std::make_optional(it->id) : std::nullopt;
		}

		std::optional<std::pair<std::uint32_t, std::uint32_t>>
			Inventory::findItemIdAndDurabilityBySerialInfo(const Main::Structures::ItemSerialInfo& itemSerialInfo) const
		{
			if (auto it = m_itemsByItemNumber.find(itemSerialInfo.itemNumber);
				it != m_itemsByItemNumber.end())
			{
				return std::make_pair(it->second.itemId.itemId, it->second.durability);
			}

			auto it = std::find_if(
				m_equippedItemByCharacter.begin(),
				m_equippedItemByCharacter.end(),
				[&itemSerialInfo](const auto& equippedItem)
				{
					return equippedItem.serialInfo.itemNumber == itemSerialInfo.itemNumber;
				}
			);

			if (it != m_equippedItemByCharacter.end())
			{
				return std::make_pair(it->id, it->durability);
			}

			return std::nullopt;
		}

		bool Inventory::isItemTradeable(const Main::Structures::ItemSerialInfo& itemSerialInfo) const
		{
			if (auto it = m_itemsByItemNumber.find(itemSerialInfo.itemNumber);
				it != m_itemsByItemNumber.end())
			{
				const auto& item = it->second;
				return Main::CdbUtils::isTradeable(item.itemId.itemId).value_or(false)
					&& item.itemId.itemId != 1000000
					&& item.expirationDate == 0;
			}

			auto it = std::find_if(m_equippedItemByCharacter.begin(), m_equippedItemByCharacter.end(),
				[&itemSerialInfo](const auto& equippedItem)
				{
					return equippedItem.serialInfo.itemNumber == itemSerialInfo.itemNumber;
				});

			if (it != m_equippedItemByCharacter.end())
			{
				return Main::CdbUtils::isTradeable(it->id).value_or(false)
					&& it->id != 1000000
					&& it->expirationDate == 0;
			}

			return false;
		}

		std::optional<std::uint64_t> Inventory::findMaxItemNumber() const
		{
			std::optional<std::uint64_t> maxItemNumber = std::nullopt;

			if (!m_itemsByItemNumber.empty())
			{
				maxItemNumber = std::max_element(m_itemsByItemNumber.begin(), m_itemsByItemNumber.end(),
					[](const auto& a, const auto& b) {
						return a.first < b.first;
					})->first;
			}

			if (!m_equippedItemByCharacter.empty())
			{
				auto maxEquipped = std::max_element(m_equippedItemByCharacter.begin(), m_equippedItemByCharacter.end(),
					[](const auto& a, const auto& b) {
						return a.serialInfo.itemNumber < b.serialInfo.itemNumber;
					})->serialInfo.itemNumber;

				if (!maxItemNumber || maxEquipped > *maxItemNumber)
				{
					maxItemNumber = maxEquipped;
				}
			}

			return maxItemNumber;
		}

		std::optional<Main::Structures::ItemSerialInfo> Inventory::getBossBattleTicket() const
		{
			for (const auto& [itemNumber, item] : m_itemsByItemNumber)
			{
				const auto id = item.itemId.itemId;
				if (id >= 4811300 && id <= 4811600)
				{
					return item.serialInfo;
				}
			}
			return std::nullopt;
		}

		bool Inventory::prolongItem(const Main::Structures::ItemSerialInfo& newItemSerialInfo)
		{
			if (auto it = m_itemsByItemNumber.find(newItemSerialInfo.itemNumber); it != m_itemsByItemNumber.end())
			{
				it->second.serialInfo = newItemSerialInfo;
				return true;
			}
			auto it = std::ranges::find_if(m_equippedItemByCharacter, [&newItemSerialInfo](auto& equippedItem)
				{
					return equippedItem.serialInfo.itemNumber == newItemSerialInfo.itemNumber;
				});
			if (it != m_equippedItemByCharacter.end())
			{
				it->serialInfo = newItemSerialInfo;
				return true;
			}
			return false;
		}

		std::vector<EquippedItem> Inventory::getEquippedItemsFor(std::uint16_t characterID) const
		{
			std::vector<EquippedItem> equippedItems;

			if (characterID >= Common::Enums::MAX_CHARACTERS)
			{
				return equippedItems;
			}

			std::size_t startIndex = characterID * Common::Enums::MAX_ITEMTYPE;
			std::size_t endIndex = startIndex + Common::Enums::MAX_ITEMTYPE;

			for (std::size_t i = startIndex; i < endIndex && i < m_equippedItemByCharacter.size(); ++i)
			{
				const auto& item = m_equippedItemByCharacter[i];
				if (item.id != 0)
					equippedItems.push_back(item);
			}

			return equippedItems;
		}

		std::vector<EquippedItem> Inventory::getUnlimitedEquippedWeaponsFor(std::uint16_t characterID) const
		{
			std::vector<EquippedItem> unlimitedItems;

			if (characterID >= Common::Enums::MAX_CHARACTERS)
				return unlimitedItems;

			const std::size_t startIndex = characterID * Common::Enums::MAX_ITEMTYPE;
			const std::size_t endIndex = startIndex + Common::Enums::MAX_ITEMTYPE;

			for (std::size_t i = startIndex; i < endIndex && i < m_equippedItemByCharacter.size(); ++i)
			{
				const auto& item = m_equippedItemByCharacter[i];
				if (item.id != 0 && item.expirationDate == 0 && Common::Enums::isWeapon(static_cast<Common::Enums::ItemType>(item.type)))
					unlimitedItems.push_back(item);
			}

			return unlimitedItems;
		}

		const std::array<EquippedItem, Common::Enums::MAX_CHARACTERS * Common::Enums::MAX_ITEMTYPE>& Inventory::getEquippedItems() const
		{
			return m_equippedItemByCharacter;
		}

		const std::unordered_map<std::uint64_t, Item>& Inventory::getItems() const
		{
			return m_itemsByItemNumber;
		}

		const std::vector<Item> Inventory::getItemsAsVec() const
		{
			std::vector<Item> items;
			items.reserve(m_itemsByItemNumber.size());
			for (const auto& [unused, item] : m_itemsByItemNumber)
			{
				items.push_back(item);
			}
			return items;
		}

		bool Inventory::deleteItemBasic(const Main::Structures::ItemSerialInfo& itemSerialInfo)
		{
			if (auto it = m_itemsByItemNumber.find(itemSerialInfo.itemNumber); it != m_itemsByItemNumber.end())
			{
				m_itemsByItemNumber.erase(it);
				return true;
			}
			auto it = std::ranges::find_if(m_equippedItemByCharacter, [&](auto& equippedItem) {
				return equippedItem.serialInfo.itemNumber == itemSerialInfo.itemNumber;
				});
			if (it != m_equippedItemByCharacter.end())
			{
				it->serialInfo.itemNumber = 0;
				return true;
			}
			return false;
		}

		void Inventory::addItems(const std::vector<Item>& items)
		{
			m_itemsByItemNumber.reserve(m_itemsByItemNumber.size() + items.size());
			for (const auto& currentItem : items)
			{
				m_itemsByItemNumber.insert_or_assign(currentItem.serialInfo.itemNumber, currentItem);
			}
		}

		std::size_t Inventory::getTotalCoupons() const noexcept
		{
			std::size_t total = 0;
			for (const auto& current : m_couponItems)
			{
				total += current.itemId.stock;
			}
			return total;
		}

		void Inventory::addTotalCouponItems(const Item& item)
		{
			m_couponItems.push_back(item);
		}

		void Inventory::addItem(const Item& item)
		{
			m_itemsByItemNumber.insert_or_assign(item.serialInfo.itemNumber, item);
		}

		void Inventory::addItems(const std::vector<Main::Structures::BoxItem>& boxItems)
		{
			addItems(std::vector<Item>(boxItems.begin(), boxItems.end()));
		}

		void Inventory::addItems(const std::vector<BoughtItem>& boughtItems)
		{
			addItems(std::vector<Item>(boughtItems.begin(), boughtItems.end()));
		}

		void Inventory::setEquippedItems(const std::unordered_map<std::uint16_t, std::vector<EquippedItem>>& equippedItems)
		{
			for (const auto& [characterID, items] : equippedItems)
			{
				for (const auto& currentItem : items)
				{
					std::size_t index = characterID * Common::Enums::MAX_ITEMTYPE + currentItem.type;

					if (index >= m_equippedItemByCharacter.size())
					{
						continue;
					}

					m_equippedItemByCharacter[index] = currentItem;
					++m_totalEquippedItems;
				}
			}
		}

		std::pair<std::vector<Main::ClientData::SingleWeaponDurabilityDamage>,
			std::vector<std::pair<std::uint32_t, std::uint64_t>>> Inventory::reduceEquippedItemsDurabilities(
			std::size_t characterID, std::uint32_t weaponRestrictionValue)
		{
			using namespace Common::Enums;

			WeaponRestriction weaponRestriction = static_cast<WeaponRestriction>(weaponRestrictionValue);
			std::vector<Main::ClientData::SingleWeaponDurabilityDamage> damages;
			std::vector<std::pair<std::uint32_t, std::uint64_t>> idsAndItemNumbers;
			const std::size_t startIndex = characterID * MAX_ITEMTYPE;
			const std::size_t endIndex = startIndex + MAX_ITEMTYPE;

			for (std::size_t i = startIndex; i < endIndex && i < m_equippedItemByCharacter.size(); ++i)
			{
				auto& item = m_equippedItemByCharacter[i];

				if (item.id == 0 || item.expirationDate != 0 || !isWeapon(static_cast<ItemType>(item.type)))
					continue;

				if (weaponRestriction != All && weaponRestriction != WeaponSelect)
				{
					ItemType restrictedType;
					switch (weaponRestriction)
					{
					case MeleeOnly:  restrictedType = MELEE; break;
					case RifleOnly:  restrictedType = RIFLE; break;
					case ShotgunOnly: restrictedType = SHOTGUN; break;
					case SniperOnly: restrictedType = SNIPER; break;
					case GatlingOnly: restrictedType = MG; break;
					case BazookaOnly: restrictedType = BAZOOKA; break;
					case GrenadeOnly: restrictedType = GRENADE; break;
					default: continue;
					}

					if (item.type != static_cast<std::uint32_t>(restrictedType))
						continue;
				}

				const auto baseDurability = Main::CdbUtils::getItemDurability(item.id);
				if (!baseDurability || *baseDurability == 0)
					continue;

				const std::uint32_t reduction = (*baseDurability / 100) * 1;
				const std::uint32_t newDurability = (*baseDurability > reduction) ? (*baseDurability - reduction) : 0;

				item.durability = newDurability;
				damages.push_back(Main::ClientData::SingleWeaponDurabilityDamage{ item.serialInfo, reduction });
				idsAndItemNumbers.push_back(std::pair{ static_cast<std::uint32_t>(item.id), static_cast<std::uint64_t>(item.serialInfo.itemNumber) });
			}

			return std::pair{ damages, idsAndItemNumbers };
		}

		bool Inventory::updateItemDurabilityByNumber(std::uint32_t itemNumber, std::uint32_t newDurability)
		{
			auto updateDurability = [&](auto& item) {
				item.durability = newDurability;
				return true;
				};
			if (auto it = m_itemsByItemNumber.find(itemNumber); it != m_itemsByItemNumber.end())
			{
				return updateDurability(it->second);
			}

			const std::size_t offset = m_accountInfo.latestSelectedCharacter * Common::Enums::MAX_ITEMTYPE;
			if (offset + Common::Enums::MAX_ITEMTYPE > m_equippedItemByCharacter.size())
			{
				return false;
			}

			for (std::size_t i = 0; i < Common::Enums::MAX_ITEMTYPE; ++i)
			{
				auto& equippedItem = m_equippedItemByCharacter[offset + i];
				if (equippedItem.serialInfo.itemNumber == itemNumber)
				{
					return updateDurability(equippedItem);
				}
			}
			return false;
		}

		std::optional<std::pair<std::uint16_t, std::uint64_t>> Inventory::addEnergyToItem(const Main::Structures::ItemSerialInfo& itemSerialInfo, std::uint32_t energyAdded)
		{
			auto updateEnergyAndBattery = [&](auto& item) {
				item.energy += energyAdded;
				m_accountInfo.battery -= energyAdded;
				return std::pair{ item.energy, static_cast<std::uint64_t>(m_accountInfo.battery) };
				};

			if (auto it = m_itemsByItemNumber.find(itemSerialInfo.itemNumber); it != m_itemsByItemNumber.end())
			{
				return updateEnergyAndBattery(it->second);
			}

			const std::size_t offset = m_accountInfo.latestSelectedCharacter * Common::Enums::MAX_ITEMTYPE;
			for (std::size_t i = 0; i < Common::Enums::MAX_ITEMTYPE; ++i)
			{
				if (offset + i >= m_equippedItemByCharacter.size()) continue;

				auto& equippedItem = m_equippedItemByCharacter[offset + i];
				if (equippedItem.serialInfo.itemNumber == itemSerialInfo.itemNumber)
				{
					return updateEnergyAndBattery(equippedItem);
				}
			}

			return std::nullopt;
		}

		std::optional<std::uint16_t> Inventory::getItemEnergy(const Main::Structures::ItemSerialInfo& itemSerialInfo) const
		{
			if (auto it = m_itemsByItemNumber.find(itemSerialInfo.itemNumber); it != m_itemsByItemNumber.end())
			{
				return it->second.energy;
			}

			const std::size_t offset = m_accountInfo.latestSelectedCharacter * Common::Enums::MAX_ITEMTYPE;
			for (std::size_t i = 0; i < Common::Enums::MAX_ITEMTYPE; ++i)
			{
				if (offset + i >= m_equippedItemByCharacter.size()) continue;

				const auto& equippedItem = m_equippedItemByCharacter[offset + i];
				if (equippedItem.serialInfo.itemNumber == itemSerialInfo.itemNumber)
				{
					return equippedItem.energy;
				}
			}

			return std::nullopt;
		}

		void Inventory::unequipItemImpl(std::uint64_t itemType, Main::Persistence::MainScheduler& scheduler, std::uint32_t character)
		{
			const std::uint32_t characterIndex = character == -1 ? m_accountInfo.latestSelectedCharacter : character;
			const std::size_t index = characterIndex * Common::Enums::MAX_ITEMTYPE + itemType;
			if (index >= m_equippedItemByCharacter.size() || (index < m_equippedItemByCharacter.size() && !m_equippedItemByCharacter[index].serialInfo.itemNumber))
				return;
			const std::uint64_t itemNumber = m_equippedItemByCharacter[index].serialInfo.itemNumber;

			// unequip the item
			m_itemsByItemNumber.insert_or_assign(itemNumber, m_equippedItemByCharacter[index]);
			m_equippedItemByCharacter[index].serialInfo.itemNumber = 0;
			--m_totalEquippedItems;
			scheduler.addRepetitiveCallback(std::source_location::current(), m_accountInfo.accountID, &Main::Persistence::PersistentDatabase::unequipItem,
				m_accountInfo.accountID, static_cast<std::uint64_t>(itemNumber));
		}

		// ugly design but easier to write, ideally we shouldn't pass the scheduler to this function though
		void Inventory::equipItem(const std::uint16_t itemNumber, Main::Persistence::MainScheduler& scheduler, std::uint32_t character)
		{
			using setItems = Common::ConstantDatabase::CdbSingleton<Common::ConstantDatabase::SetItemInfo>;

			auto it = m_itemsByItemNumber.find(itemNumber);
			if (it == m_itemsByItemNumber.end()) return;

			EquippedItem equippedItem = EquippedItem{ it->second };

			if (const auto entry = setItems::getInstance().getEntry(it->second.itemId.itemId);
				entry && equippedItem.type == Common::Enums::ItemType::SET)
			{ // Case 1: the user is equipping a set. We need to unequip item types that are already present in such set first
				for (auto currentTypeNotNull : Common::Utils::getPartTypesWhereSetItemInfoTypeNotNull(*entry, m_accountInfo.latestSelectedCharacter))
				{
					unequipItemImpl(currentTypeNotNull, scheduler, character);
				}
			}

			const std::size_t charIndex = character == -1 ? m_accountInfo.latestSelectedCharacter : character;
			const std::size_t setIndex = charIndex * Common::Enums::MAX_ITEMTYPE + Common::Enums::ItemType::SET;

			if (setIndex >= m_equippedItemByCharacter.size())
				return;

			const auto& setItem = m_equippedItemByCharacter[setIndex];

			if (const auto entry = setItems::getInstance().getEntry(setItem.id);
				entry && setItem.serialInfo.itemNumber)
			{ // Case 2: the user has an already equipped set. We need to unequip it if the user is now equipping a part type that is already present in said set
				for (auto currentTypeNotNull : Common::Utils::getPartTypesWhereSetItemInfoTypeNotNull(*entry, m_accountInfo.latestSelectedCharacter))
				{
					if (equippedItem.type == currentTypeNotNull)
					{
						unequipItemImpl(Common::Enums::SET, scheduler, character);
						break;
					}
				}
			}

			// Delete this item from the Non-Equipped items, since we'll add it to the Equipped items.
			m_itemsByItemNumber.erase(itemNumber);

			// Now proceed normally: check if equippedItems already has such a type
			const std::size_t itemIndex = charIndex * Common::Enums::MAX_ITEMTYPE + equippedItem.type;
			if (itemIndex >= m_equippedItemByCharacter.size()) return;
			if (m_equippedItemByCharacter[itemIndex].serialInfo.itemNumber)
			{
				const std::uint64_t toUnequipItemNumber = m_equippedItemByCharacter[itemIndex].serialInfo.itemNumber;
				m_itemsByItemNumber.insert_or_assign(toUnequipItemNumber, Item{ m_equippedItemByCharacter[itemIndex] });
				m_equippedItemByCharacter[itemIndex] = equippedItem;

				scheduler.addRepetitiveCallback(std::source_location::current(), m_accountInfo.accountID, &Main::Persistence::PersistentDatabase::swapItems,
					m_accountInfo.accountID, toUnequipItemNumber, static_cast<std::uint64_t>(equippedItem.serialInfo.itemNumber),
					static_cast<std::uint16_t>(character == -1 ? m_accountInfo.latestSelectedCharacter : character));
				return;
			}

			// Otherwise just add the to-be-added item to the equipped items.
			m_equippedItemByCharacter[itemIndex] = equippedItem;
			++m_totalEquippedItems;

			scheduler.addRepetitiveCallback(std::source_location::current(), m_accountInfo.accountID, &Main::Persistence::PersistentDatabase::equipItem,
				m_accountInfo.accountID, static_cast<std::uint64_t>(equippedItem.serialInfo.itemNumber),
				static_cast<std::uint16_t>(character == -1 ? m_accountInfo.latestSelectedCharacter : character));
		}

		std::optional<std::uint64_t> Inventory::unequipItem(std::uint64_t itemType, Main::Persistence::MainScheduler& scheduler)
		{
			const std::size_t itemIndex = m_accountInfo.latestSelectedCharacter * Common::Enums::MAX_ITEMTYPE + itemType;
			if (itemIndex >= m_equippedItemByCharacter.size()) return std::nullopt;
			if (!m_equippedItemByCharacter[itemIndex].serialInfo.itemNumber)
			{
				return std::nullopt;
			}
			const std::uint64_t itemNumber = m_equippedItemByCharacter[itemIndex].serialInfo.itemNumber;
			m_itemsByItemNumber.insert_or_assign(itemNumber, Item{ m_equippedItemByCharacter[itemIndex] });
			m_equippedItemByCharacter[itemIndex].serialInfo.itemNumber = 0;
			--m_totalEquippedItems;
			return itemNumber;
		}

		std::uint64_t Inventory::getTotalEquippedItems() const
		{
			return m_totalEquippedItems;
		}

		std::uint64_t Inventory::getLatestItemNumber() const
		{
			return findMaxItemNumber().value_or(0); // on purpose, we're sure this always find the latest item number (greatest)
		}

		void Inventory::setLatestItemNumber(std::uint64_t itemNum)
		{
			//m_latestItemNumber = itemNum;
		}

		std::pair<Common::Enums::MatchItemAction, std::uint32_t> Inventory::useInstantRespawn(std::uint64_t itemNum)
		{
			if (auto it = m_itemsByItemNumber.find(itemNum); it != m_itemsByItemNumber.end())
			{
				if (it->second.itemId.stock > 0)
				{
					if (it->second.itemId.stock == 1)
					{
						return { Common::Enums::MATCHITEM_DELETE, 0 };
					}
					else
					{
						--it->second.itemId.stock;
						return { Common::Enums::MATCHITEM_STOCKS_REDUCED_SUCCESS, static_cast<std::uint32_t>(it->second.itemId.stock) };
					}
				}
				else
				{
					return { Common::Enums::MATCHITEM_STOCK_ZERO, 0 };
				}
			}
			return { Common::Enums::MATCHITEM_DO_NOTHING, 0 };
		}

		bool Inventory::unequipItemIfEquipped(std::uint64_t itemNumber, std::uint32_t characterId, Main::Persistence::MainScheduler& scheduler)
		{
			if (characterId >= Common::Enums::MAX_CHARACTERS) return false;

			const std::size_t startIndex = characterId * Common::Enums::MAX_ITEMTYPE;
			for (std::size_t itemIndex = 0; itemIndex < Common::Enums::MAX_ITEMTYPE; ++itemIndex)
			{
				if (startIndex + itemIndex >= m_equippedItemByCharacter.size())
					break;

				const auto& equippedItem = m_equippedItemByCharacter[startIndex + itemIndex];
				if (equippedItem.serialInfo.itemNumber == itemNumber)
				{
					unequipItemImpl(itemIndex, scheduler, characterId);
					return true;
				}
			}
			return false;
		}

		void Inventory::equipItemIfNotEquipped(std::uint64_t itemNumber, std::uint32_t characterId, Main::Persistence::MainScheduler& scheduler)
		{
			if (characterId >= Common::Enums::MAX_CHARACTERS) return;

			const std::size_t startIndex = characterId * Common::Enums::MAX_ITEMTYPE;
			for (std::size_t itemIndex = 0; itemIndex < Common::Enums::MAX_ITEMTYPE; ++itemIndex)
			{
				if (startIndex + itemIndex >= m_equippedItemByCharacter.size()) continue;

				const auto& equippedItem = m_equippedItemByCharacter[startIndex + itemIndex];
				if (equippedItem.serialInfo.itemNumber == itemNumber) return;
			}

			equipItem(static_cast<std::uint16_t>(itemNumber), scheduler, characterId);
		}

		std::pair<std::array<std::uint32_t, 10>, std::array<std::uint32_t, 7>> Inventory::getEquippedItemsSeparated() const
		{
			std::array<std::uint32_t, 10> equippedPlayerItems{};
			std::array<std::uint32_t, 7> equippedPlayerWeapons{};
			const std::size_t startIndex = m_accountInfo.latestSelectedCharacter * Common::Enums::MAX_ITEMTYPE;

			// Set is a special case
			const std::size_t setIndex = startIndex + Common::Enums::ItemType::SET;
			if (setIndex >= m_equippedItemByCharacter.size()) return std::pair{ equippedPlayerItems, equippedPlayerWeapons };
			if (m_equippedItemByCharacter[setIndex].serialInfo.itemNumber)
			{
				using setItems = Common::ConstantDatabase::CdbSingleton<Common::ConstantDatabase::SetItemInfo>;
				const std::uint32_t setItemId = m_equippedItemByCharacter[setIndex].id;
				if (auto entry = setItems::getInstance().getEntry(setItemId); entry)
				{
					for (const auto& currentTypeNotNull : Common::Utils::getPartTypesWhereSetItemInfoTypeNotNull(*entry, m_accountInfo.latestSelectedCharacter))
						equippedPlayerItems[currentTypeNotNull] = setItemId;
				}
			}
			for (std::size_t i = 0; i < Common::Enums::MAX_ITEMTYPE; ++i)
			{
				const auto& equippedItem = m_equippedItemByCharacter[startIndex + i];
				if (equippedItem.serialInfo.itemNumber)
				{
					if (i < Common::Enums::MELEE)
						equippedPlayerItems[i] = equippedItem.id;
					else if (i < Common::Enums::MAX_ITEMTYPE && (i - 10 < equippedPlayerWeapons.size()))
						equippedPlayerWeapons[i - 10] = equippedItem.id;
				}
			}
			return { equippedPlayerItems, equippedPlayerWeapons };
		}

		// Trade system
		std::vector<Main::Structures::Item> Inventory::addItems(const std::vector<Main::Structures::TradeBasicItem>& tradedItems)
		{
			std::vector<Item> items;
			for (auto& currentItem : tradedItems)
			{
				m_itemsByItemNumber.insert_or_assign(currentItem.itemSerialInfo.itemNumber, currentItem);
			}
			return items;
		}

		Item Inventory::addItemFromTrade(TradedItem tradeItem)
		{
			auto latestItemNum = getLatestItemNumber();
			tradeItem.itemSerialInfo.itemNumber = ++latestItemNum;
			Main::Structures::Item item{ tradeItem };
			m_itemsByItemNumber.insert_or_assign(tradeItem.itemSerialInfo.itemNumber, item);
			return item;
		}
	}
}
