
#include "../../include/Classes/Player.h"
#include <unordered_map>
#include <vector>
#include <algorithm>
#include "../../include/Network/MainSession.h"
#include "../../include/Structures/AccountInfo/MuteInfo.h"
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
		using Session = Main::Network::Session;
		using BlockedPlayer = Main::Structures::BlockedPlayer;
		using Friend = Main::Structures::Friend;
		using Mailbox = Main::Structures::Mailbox;
		using Session = Main::Network::Session;
		using TradedItem = Main::Structures::TradeBasicItem;

		void Player::setAccountInfo(const AccountInfo& accountInfo)
		{
			m_accountInfo = accountInfo;
		}

		std::uint32_t Player::addBattery(std::uint32_t battery)
		{
			m_accountInfo.battery = std::min(static_cast<std::uint32_t>(m_accountInfo.battery + battery), m_accountInfo.maxBattery);
			return m_accountInfo.battery;
		}


		void Player::addBatteryObtainedInMatch(std::uint32_t newBattery)
		{
			m_batteryObtainedInMatch += newBattery;
		}

		void Player::storeBatteryObtainedInMatch()
		{
			if (m_accountInfo.battery + m_batteryObtainedInMatch >= m_accountInfo.maxBattery)
			{
				m_accountInfo.battery = m_accountInfo.maxBattery;
			}
			else
			{
				m_accountInfo.battery += m_batteryObtainedInMatch;
			}
			m_batteryObtainedInMatch = 0;
		}

		const AccountInfo& Player::getAccountInfo() const
		{
			return m_accountInfo;
		}

		void Player::setPing(std::uint16_t ping)
		{
			m_ping = ping;
		}

		std::uint16_t Player::getPing() const
		{
			return m_ping;
		}

		std::uint32_t Player::getAccountID() const
		{
			return m_accountInfo.accountID;
		}

		const char* const Player::getPlayerName() const
		{
			return m_accountInfo.nickname;
		}

		bool Player::setAccountRockTotens(std::uint32_t rt)
		{
			if (rt > 0x3FFFFFFF) return false;
			m_accountInfo.rockTotens = rt;
			return true;
		}

		bool Player::setAccountMicroPoints(std::uint32_t mp)
		{
			if (mp > 0x7FFFFFFF) return false;
			m_accountInfo.microPoints = mp;
			return true;
		}

		bool Player::setAccountCoins(std::uint16_t coins)
		{
			if (coins > 0x7F) return false;
			m_accountInfo.coins = coins;
			return true;
		}

		void Player::setAccountLatestCharacterSelected(std::uint16_t latestCharacterSelected)
		{
			m_accountInfo.latestSelectedCharacter = latestCharacterSelected;
		}

		void Player::setLevel(std::uint16_t level)
		{
			m_accountInfo.playerLevel = level + 1;
		}

		void Player::setExperience(std::uint32_t exp)
		{
			m_accountInfo.experience = exp;
		}

		void Player::resetKillDeath()
		{
			m_accountInfo.totalKills = m_accountInfo.deaths = 0;
		}

		void Player::resetRecord()
		{
			m_accountInfo.wins = m_accountInfo.losses = m_accountInfo.draws = 0;
		}

		bool Player::expandBattery()
		{
			if (m_accountInfo.maxBattery > 4000) return false;
			m_accountInfo.maxBattery += 1000;
			return true;
		}

		bool Player::expandInventory(std::uint32_t spaceToAdd)
		{
			if (m_accountInfo.inventorySpace + spaceToAdd > 1000) return false;
			m_accountInfo.inventorySpace += spaceToAdd;
			return true;
		}

		void Player::setPlayerName(const char* playerName)
		{
			strncpy(m_accountInfo.nickname, playerName, sizeof(m_accountInfo.nickname) - 1);
			m_accountInfo.nickname[sizeof(m_accountInfo.nickname) - 1] = '\0';
		}

		bool Player::hasEnoughInventorySpace(std::uint16_t totalNewItems) const
		{
			return m_inventory.hasEnoughInventorySpace(totalNewItems);
		}

		Main::ClientData::CouponItemUseRet Player::tryRemoveCoupons(std::uint32_t totalCouponsNeeded)
		{
			return m_inventory.tryRemoveCoupons(totalCouponsNeeded);
		}

		Main::ClientData::CouponItemAddRet Player::addCoupon(std::uint32_t stockToAdd)
		{
			return m_inventory.addCoupon(stockToAdd);
		}

		void Player::setPlayerState(Common::Enums::PlayerState playerState)
		{
			m_playerState = playerState;
		}

		Common::Enums::PlayerState Player::getPlayerState() const
		{
			return m_playerState;
		}

		bool Player::isInLobby() const
		{
			return m_roomNumber == 0;
		}

		Main::Structures::MuteInfo Player::getMuteInfo() const
		{
			return Main::Structures::MuteInfo{ m_isMuted, m_muteReason, m_mutedBy, m_mutedUntil };
		}

		void Player::mute(const std::string& reason, const std::string& mutedBy, const std::string& mutedUntil)
		{
			m_isMuted = true;
			m_muteReason = reason;
			m_mutedBy = mutedBy;
			m_mutedUntil = mutedUntil;
		}

		void Player::disableRoomCreation()
		{
			m_isRoomCreationEnabled = false;
		}

		void Player::enableRoomCreation()
		{
			m_isRoomCreationEnabled = true;
		}

		bool Player::isRoomCreationEnabled() const noexcept
		{
			return m_isRoomCreationEnabled;
		}

		void Player::disableVotekick()
		{
			m_isVotekickEnabled = false;
		}

		void Player::enableVotekick()
		{
			m_isVotekickEnabled = true;
		}

		bool Player::isVotekickEnabled() const noexcept
		{
			return m_isVotekickEnabled;
		}


		void Player::unmute()
		{
			m_isMuted = false;
		}

		bool Player::isMuted() const
		{
			return m_isMuted;
		}

		void Player::addLuckyPoints(std::uint32_t points)
		{
			m_accountInfo.luckyPoints += points;
		}

		void Player::setLuckyPoints(std::uint32_t points)
		{
			m_accountInfo.luckyPoints = points;
		}

		std::uint32_t Player::getLuckyPoints() const
		{
			return static_cast<std::uint32_t>(m_accountInfo.luckyPoints);
		}

		const std::vector<Friend> Player::getFriendlist() const
		{
			std::vector<Friend> ret;
			for (const auto& [ffriend, unused] : m_friends)
			{
				ret.push_back(ffriend);
			}
			return ret;
		}

		std::unordered_map<Friend, std::weak_ptr<Session>>& Player::getFriendSessions()
		{
			return m_friends;
		}

		void Player::setFriendList(const std::vector<Friend>& friendlist)
		{
			for (const auto& currentFriend : friendlist)
			{
				m_friends[currentFriend] = std::weak_ptr<Session>{};
			}
		}

		void Player::updateFriend(const Friend& targetFriend, std::shared_ptr<Main::Network::Session> targetSession, bool remove = false)
		{
			if (m_friends.contains(targetFriend)) m_friends.erase(targetFriend);
			m_friends[targetFriend] = remove ? std::weak_ptr<Session>{} : std::weak_ptr<Session>{ targetSession };
		}

		// call once with default "persist", since removeFriend removes the friend for both players
		bool Player::deleteFriend(std::uint32_t targetAccountId)
		{
			Main::Structures::Friend targetFriend;
			targetFriend.targetAccountId = targetAccountId;
			auto it = m_friends.find(targetFriend);
			if (it != m_friends.end())
			{
				m_friends.erase(it);
				return true;
			}
			return false;
		}


		void Player::addOfflineFriend(const Main::Structures::Friend& ffriend)
		{
			m_friends[ffriend] = std::weak_ptr<Session>{};
		}

		std::optional<Main::Structures::ItemSerialInfo> Player::getBossBattleTicket() const
		{
			return m_inventory.getBossBattleTicket();
		}


		std::optional<Main::Structures::Friend> Player::addOnlineFriend(std::shared_ptr<Main::Network::Session> session)
		{
			if (session)
			{
				const auto& accountInfo = session->getAccountInfo();
				Main::Structures::Friend ffriend{ accountInfo.uniqueId, accountInfo.accountID };
				std::memcpy(ffriend.targetNickname, accountInfo.nickname, 16);
				m_friends[ffriend] = session;
				return ffriend;
			}
			return std::nullopt;
		}

		bool Player::isFriend(std::uint32_t accountId) const
		{
			for (const auto& [key, val] : m_friends)
			{
				if (key.targetAccountId == accountId)
				{
					return true;
				}
			}
			return false;
		}

		void Player::setUnequippedItems(const std::vector<Item>& items)
		{
			m_inventory.setUnequippedItems(items);
		}

		std::optional<std::uint32_t> Player::findItemIdBySerialInfo(const Main::Structures::ItemSerialInfo& itemSerialInfo) const
		{
			return m_inventory.findItemIdBySerialInfo(itemSerialInfo);
		}

		std::optional<std::pair<std::uint32_t, std::uint32_t>>
			Player::findItemIdAndDurabilityBySerialInfo(const Main::Structures::ItemSerialInfo& itemSerialInfo) const
		{
			return m_inventory.findItemIdAndDurabilityBySerialInfo(itemSerialInfo);
		}

		bool Player::isItemTradeable(const Main::Structures::ItemSerialInfo& itemSerialInfo) const
		{
			return m_inventory.isItemTradeable(itemSerialInfo);
		}

		std::optional<std::uint64_t> Player::findMaxItemNumber() const
		{
			return m_inventory.findMaxItemNumber();
		}

		bool Player::prolongItem(const Main::Structures::ItemSerialInfo& newItemSerialInfo)
		{
			return m_inventory.prolongItem(newItemSerialInfo);
		}

		std::vector<EquippedItem> Player::getEquippedItemsFor(std::uint16_t characterID) const
		{
			return m_inventory.getEquippedItemsFor(characterID);
		}

		std::vector<EquippedItem> Player::getUnlimitedEquippedWeaponsFor(std::uint16_t characterID) const
		{
			return m_inventory.getUnlimitedEquippedWeaponsFor(characterID);
		}

		const std::array<EquippedItem, Common::Enums::MAX_CHARACTERS * Common::Enums::MAX_ITEMTYPE>& Player::getEquippedItems() const
		{
			return m_inventory.getEquippedItems();
		}

		const std::unordered_map<std::uint64_t, Item>& Player::getItems() const
		{
			return m_inventory.getItems();
		}

		const std::vector<Item> Player::getItemsAsVec() const
		{
			return m_inventory.getItemsAsVec();
		}

		bool Player::deleteItemBasic(const Main::Structures::ItemSerialInfo& itemSerialInfo)
		{
			return m_inventory.deleteItemBasic(itemSerialInfo);
		}

		void Player::addItems(const std::vector<Item>& items)
		{
			m_inventory.addItems(items);
		}

		std::size_t Player::getTotalCoupons() const noexcept
		{
			return m_inventory.getTotalCoupons();
		}

		void Player::addTotalCouponItems(const Item& item)
		{
			m_inventory.addTotalCouponItems(item);
		}

		void Player::addItem(const Item& item)
		{
			m_inventory.addItem(item);
		}

		void Player::addItems(const std::vector<Main::Structures::BoxItem>& boxItems)
		{
			m_inventory.addItems(boxItems);
		}

		void Player::addItems(const std::vector<BoughtItem>& boughtItems)
		{
			m_inventory.addItems(boughtItems);
		}

		void Player::setEquippedItems(const std::unordered_map<std::uint16_t, std::vector<EquippedItem>>& equippedItems)
		{
			m_inventory.setEquippedItems(equippedItems);
		}

		std::pair<std::vector<Main::ClientData::SingleWeaponDurabilityDamage>,
			std::vector<std::pair<std::uint32_t, std::uint64_t>>> Player::reduceEquippedItemsDurabilities(
			std::size_t characterID, std::uint32_t weaponRestrictionValue)
		{
			return m_inventory.reduceEquippedItemsDurabilities(characterID, weaponRestrictionValue);
		}

		bool Player::updateItemDurabilityByNumber(std::uint32_t itemNumber, std::uint32_t newDurability)
		{
			return m_inventory.updateItemDurabilityByNumber(itemNumber, newDurability);
		}

		std::optional<std::pair<std::uint16_t, std::uint64_t>> Player::addEnergyToItem(const Main::Structures::ItemSerialInfo& itemSerialInfo, std::uint32_t energyAdded)
		{
			return m_inventory.addEnergyToItem(itemSerialInfo, energyAdded);
		}

		std::optional<std::uint16_t> Player::getItemEnergy(const Main::Structures::ItemSerialInfo& itemSerialInfo) const
		{
			return m_inventory.getItemEnergy(itemSerialInfo);
		}

		void Player::unequipItemImpl(std::uint64_t itemType, Main::Persistence::MainScheduler& scheduler, std::uint32_t character)
		{
			m_inventory.unequipItemImpl(itemType, scheduler, character);
		}

		void Player::equipItem(const std::uint16_t itemNumber, Main::Persistence::MainScheduler& scheduler, std::uint32_t character)
		{
			m_inventory.equipItem(itemNumber, scheduler, character);
		}

		std::optional<std::uint64_t> Player::unequipItem(std::uint64_t itemType, Main::Persistence::MainScheduler& scheduler)
		{
			return m_inventory.unequipItem(itemType, scheduler);
		}

		std::uint64_t Player::getTotalEquippedItems() const
		{
			return m_inventory.getTotalEquippedItems();
		}

		std::uint64_t Player::getLatestItemNumber() const
		{
			return m_inventory.getLatestItemNumber();
		}

		void Player::setLatestItemNumber(std::uint64_t itemNum)
		{
			m_inventory.setLatestItemNumber(itemNum);
		}

		std::pair<Common::Enums::MatchItemAction, std::uint32_t> Player::useInstantRespawn(std::uint64_t itemNum)
		{
			return m_inventory.useInstantRespawn(itemNum);
		}

		bool Player::unequipItemIfEquipped(std::uint64_t itemNumber, std::uint32_t characterId, Main::Persistence::MainScheduler& scheduler)
		{
			return m_inventory.unequipItemIfEquipped(itemNumber, characterId, scheduler);
		}

		void Player::equipItemIfNotEquipped(std::uint64_t itemNumber, std::uint32_t characterId, Main::Persistence::MainScheduler& scheduler)
		{
			m_inventory.equipItemIfNotEquipped(itemNumber, characterId, scheduler);
		}

		std::pair<std::array<std::uint32_t, 10>, std::array<std::uint32_t, 7>> Player::getEquippedItemsSeparated() const
		{
			return m_inventory.getEquippedItemsSeparated();
		}

		bool Player::blockAccount(std::uint32_t accountId, const char* nickname)
		{
			if (m_blockedAccounts.size() >= Common::Constants::maxFriends)
				return false;

			Main::Structures::BlockedPlayer blocked{ accountId };
			std::memcpy(blocked.targetNickname, nickname, sizeof(blocked.targetNickname));
			m_blockedAccounts.push_back(blocked);
			return true;
		}


		bool Player::unblockAccount(std::uint32_t accountId)
		{
			auto it = std::remove_if(m_blockedAccounts.begin(), m_blockedAccounts.end(),
				[accountId](const auto& account) { return account.targetAccountId == accountId; });

			if (it != m_blockedAccounts.end())
			{
				m_blockedAccounts.erase(it, m_blockedAccounts.end());
				return true;
			}
			return false; // Account not found
		}

		void Player::addAchievementTier1(std::uint32_t achievementId)
		{
			m_accountInfo.achievements.setAchievementTier1(achievementId);
		}

		bool Player::hasBlocked(std::uint32_t accountId) const
		{
			for (const auto& currentBlocked : m_blockedAccounts)
			{
				if (currentBlocked.targetAccountId == accountId)
				{
					return true;
				}
			}
			return false;
		}

		const std::vector<Main::Structures::BlockedPlayer>& Player::getBlockedPlayers() const
		{
			return m_blockedAccounts;
		}

		void Player::setBlockedPlayers(const std::vector<Main::Structures::BlockedPlayer>& blockedPlayers)
		{
			m_blockedAccounts = blockedPlayers;
		}

		// Mailbox
		void Player::addMailboxReceived(const Main::Structures::Mailbox& mailbox)
		{
			m_mailboxReceived.push_back(mailbox);
		}

		void Player::addGiftboxReceived(const Main::Structures::Giftbox& giftbox)
		{
			m_giftboxReceived.push_back(giftbox);
		}

		void Player::addMailboxSent(const Main::Structures::Mailbox& mailbox)
		{
			m_mailboxSent.push_back(mailbox);
		}

		bool Player::deleteSentMailbox(std::uint32_t timestamp)
		{
			auto it = std::find_if(m_mailboxSent.begin(), m_mailboxSent.end(), [timestamp](const auto& mailbox) {
				return mailbox.timestamp == timestamp;
				});
			if (it != m_mailboxSent.end())
			{
				m_mailboxSent.erase(it);
				return true;
			}
			return false;
		}

		bool Player::deleteReceivedMailbox(std::uint32_t timestamp)
		{
			auto it = std::find_if(m_mailboxReceived.begin(), m_mailboxReceived.end(), [timestamp](const auto& mailbox) {
				return mailbox.timestamp == timestamp;
				});
			if (it != m_mailboxReceived.end())
			{
				m_mailboxReceived.erase(it);
				return true;
			}
			return false;
		}

		const std::vector<Main::Structures::Mailbox>& Player::getMailboxReceived() const
		{
			return m_mailboxReceived;
		}

		const std::vector<Main::Structures::Mailbox>& Player::getMailboxSent() const
		{
			return m_mailboxSent;
		}

		const std::vector<Main::Structures::Giftbox>& Player::getGiftboxReceived() const
		{
			return m_giftboxReceived;
		}

		void Player::deleteGiftbox(std::uint32_t timestamp)
		{
			auto it = std::remove_if(m_giftboxReceived.begin(), m_giftboxReceived.end(), [timestamp](const auto& giftbox) {
				return giftbox.timestamp == timestamp;
				});

			if (it != m_giftboxReceived.end())
			{
				m_giftboxReceived.erase(it, m_giftboxReceived.end());
			}
		}

		void Player::setMailbox(const std::vector<Main::Structures::Mailbox>& mailbox, bool sent)
		{
			if (sent) m_mailboxSent = mailbox;
			else m_mailboxReceived = mailbox;
		}

		std::optional<std::uint32_t> Player::getItemIdFromGiftbox(std::uint32_t timestamp) const
		{
			for (const auto& currentGiftbox : m_giftboxReceived)
			{
				if (currentGiftbox.timestamp == timestamp)
				{
					return currentGiftbox.id;
				}
			}
			return std::nullopt;
		}

		std::optional<Main::Structures::Giftbox> Player::getGiftbox(std::uint32_t timestamp) const
		{
			for (const auto& currentGiftbox : m_giftboxReceived)
			{
				if (currentGiftbox.timestamp == timestamp)
				{
					return currentGiftbox;
				}
			}
			return std::nullopt;
		}


		void Player::setReceivedGiftboxes(const std::vector<Main::Structures::Giftbox>& giftbox)
		{
			m_giftboxReceived = giftbox;
		}

		// Room info
		void Player::setRoomNumber(std::uint16_t roomNumber)
		{
			m_roomNumber = roomNumber;
		}

		void Player::setPartyRoomNumber(std::uint16_t partyRoomNumber)
		{
			m_partyRoomNumber = partyRoomNumber;
		}

		std::uint16_t Player::getRoomNumber() const
		{
			return m_roomNumber;
		}

		std::uint16_t Player::getPartyRoomNumber() const noexcept
		{
			return m_partyRoomNumber;
		}

		void Player::setIsInMatch(bool val)
		{
			m_isInMatch = val;
		}

		bool Player::isInMatch() const
		{
			return (m_playerState == Common::Enums::STATE_NORMAL || m_playerState == Common::Enums::STATE_DYING);
		}

		void Player::leaveRoom()
		{
			setRoomNumber(0);
			setIsInMatch(false);
			m_batteryObtainedInMatch = 0;
		}

		void Player::decreaseRoomNumber()
		{
			if (m_roomNumber > 0)
			{
				--m_roomNumber;
			}
		}

		// Trade system
		std::vector<Main::Structures::Item> Player::addItems(const std::vector<Main::Structures::TradeBasicItem>& tradedItems)
		{
			return m_inventory.addItems(tradedItems);
		}

		Item Player::addItemFromTrade(TradedItem tradeItem)
		{
			return m_inventory.addItemFromTrade(tradeItem);
		}

		void Player::lockTrade()
		{
			m_hasPlayerLocked = true;
		}

		bool Player::hasPlayerLocked() const
		{
			return m_hasPlayerLocked;
		}

		void Player::resetTradeInfo()
		{
			m_hasPlayerLocked = false;
			m_tradedItems.clear();
			m_currentlyTradingWithAccountId = 0;
			setPlayerState(Common::Enums::PlayerState::STATE_INVENTORY);
		}

		void Player::setCurrentlyTradingWithAccountId(std::uint32_t targetAccountId)
		{
			m_currentlyTradingWithAccountId = targetAccountId;
		}

		std::uint32_t Player::getCurrentlyTradingWithAccountId() const
		{
			return m_currentlyTradingWithAccountId;
		}

		bool Player::addTradedItem(std::uint32_t itemId, const Main::Structures::ItemSerialInfo& serialInfo)
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


		void Player::removeTradedItem(const Main::Structures::ItemSerialInfo& serialInfo)
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

		void Player::resetTradedItems()
		{
			m_tradedItems.clear();
		}

		const std::vector<TradedItem>& Player::getTradedItems() const
		{
			return m_tradedItems;
		}

	}
}
