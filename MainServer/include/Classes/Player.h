#ifndef PLAYER_CLASS_H
#define PLAYER_CLASS_H

#include "../Structures/Item/MainEquippedItem.h"
#include "../Structures/Item/MainBoughtItem.h"
#include "../Structures/AccountInfo/MainAccountInfo.h"
#include "../Structures/PlayerLists/Friend.h"
#include "../Structures/Mailbox.h"
#include "../Structures/PlayerLists/BlockedPlayer.h"
#include "../Structures/Item/MainItem.h"
#include "../Persistence/MainScheduler.h"
#include "../Structures/TradeSystem/TradeSystemItem.h"
#include "../Structures/ClientData/Structures.h"
#include "Inventory.h"

#include <unordered_map>
#include <vector>
#include <array>
#include <optional>

namespace Common { namespace Network { class Session; } }
namespace Main { namespace Network { class Session; } }
namespace Main
{
	namespace Classes
	{
		class Player
		{
		private:
			using Item = Main::Structures::Item;
			using EquippedItem = Main::Structures::EquippedItem;
			using DetailedEquippedItem = Main::Structures::DetailedEquippedItem;
			using BoughtItem = Main::Structures::BoughtItem;
			using AccountInfo = Main::Structures::AccountInfo;
			using Session = Main::Network::Session;
			using BlockedPlayer = Main::Structures::BlockedPlayer;
			using Friend = Main::Structures::Friend;
			using Mailbox = Main::Structures::Mailbox;
			using Giftbox = Main::Structures::Giftbox;
			using TradedItem = Main::Structures::TradeBasicItem;

			AccountInfo m_accountInfo{};
			Inventory m_inventory{ m_accountInfo };
			std::unordered_map<Friend, std::weak_ptr<Session>> m_friends;
			std::vector<BlockedPlayer> m_blockedAccounts{};
			Common::Enums::PlayerState m_playerState{};
			std::uint16_t m_ping{};
			bool m_isMuted{ false };
			bool m_isRoomCreationEnabled{ true };
			bool m_isVotekickEnabled{true};
			std::string m_mutedBy{};
			std::string m_muteReason{};
			std::string m_mutedUntil{};
			std::string m_latestWeeklyRewardDay{};
			std::string m_latestMonthlyRewardDay{};

			// Mailbox/Giftbox specific
			std::vector<Mailbox> m_mailboxReceived{};
			std::vector<Mailbox> m_mailboxSent{};
			std::vector<Giftbox> m_giftboxReceived{};

			// Other
			std::uint16_t m_roomNumber{};
			std::uint16_t m_partyRoomNumber{};
			bool m_isInMatch{};
			std::uint32_t m_batteryObtainedInMatch{};

			// Trade system
			std::uint32_t m_currentlyTradingWithAccountId{};
			std::vector<TradedItem> m_tradedItems{};
			bool m_hasPlayerLocked{};

		public:
			// Inventory
			Inventory& getInventory() noexcept { return m_inventory; }
			const Inventory& getInventory() const noexcept { return m_inventory; }

			// Account info
			void setAccountInfo(const AccountInfo& accountInfo);
			void addBatteryObtainedInMatch(std::uint32_t newBattery);
			void storeBatteryObtainedInMatch();
			const AccountInfo& getAccountInfo() const;
			std::uint32_t getAccountID() const;
			const char* const getPlayerName() const;
			bool setAccountRockTotens(std::uint32_t rt);
			bool setAccountMicroPoints(std::uint32_t mp);
			bool setAccountCoins(std::uint16_t coins);
			void setAccountLatestCharacterSelected(std::uint16_t latestCharacterSelected);
			void setLevel(std::uint16_t level);
			void setExperience(std::uint32_t exp);
			void setPlayerName(const char* playerName);
			void setPlayerState(Common::Enums::PlayerState playerState);
			Common::Enums::PlayerState getPlayerState() const;
			void addLuckyPoints(std::uint32_t points);
			void setLuckyPoints(std::uint32_t points);
			std::uint32_t getLuckyPoints() const;
			void setPing(std::uint16_t ping);
			std::uint16_t getPing() const;
			bool isInLobby() const;
			void mute(const std::string& reason, const std::string& mutedBy, const std::string& mutedUntil);
			void unmute();
			void disableRoomCreation();
			void enableRoomCreation();
			bool isRoomCreationEnabled() const noexcept;
			void disableVotekick();
			void enableVotekick();
			bool isVotekickEnabled() const noexcept;
			Main::Structures::MuteInfo getMuteInfo() const;
			bool isMuted() const;
			void resetKillDeath();
			void resetRecord();
			bool expandBattery();
			bool expandInventory(std::uint32_t spaceToAdd);

			// Friends
			const std::vector<Friend> getFriendlist() const;
			std::unordered_map<Friend, std::weak_ptr<Session>>& getFriendSessions();
			void setFriendList(const std::vector<Friend>& friendlist);
			void updateFriend(const Friend& targetFriend, std::shared_ptr<Main::Network::Session> targetSession, bool remove);
			// call once with default "persist", since removeFriend removes the friend for both players
			bool deleteFriend(std::uint32_t targetAccountId);
			void addOfflineFriend(const Main::Structures::Friend& ffriend);
			std::optional<Main::Structures::Friend> addOnlineFriend(std::shared_ptr<Main::Network::Session> session);
			bool isFriend(std::uint32_t accountId) const;

			std::uint32_t addBattery(std::uint32_t battery);


			// Blocked players
			bool blockAccount(std::uint32_t accountId, const char* nickname);
			bool unblockAccount(std::uint32_t accountId);
			bool hasBlocked(std::uint32_t accountId) const;
			const std::vector<Main::Structures::BlockedPlayer>& getBlockedPlayers() const;
			void setBlockedPlayers(const std::vector<Main::Structures::BlockedPlayer>& blockedPlayers);

			// Mailbox, giftbox
			void addMailboxReceived(const Main::Structures::Mailbox& mailbox);
			void addGiftboxReceived(const Main::Structures::Giftbox& giftbox);
			void addMailboxSent(const Main::Structures::Mailbox& mailbox);
			bool deleteSentMailbox(std::uint32_t timestamp);
			bool deleteReceivedMailbox(std::uint32_t timestamp);
			const std::vector<Main::Structures::Mailbox>& getMailboxReceived() const;
			const std::vector<Main::Structures::Mailbox>& getMailboxSent() const;
			const std::vector<Main::Structures::Giftbox>& getGiftboxReceived() const;
			std::optional<Main::Structures::Giftbox> getGiftbox(std::uint32_t timestamp) const;
			void setMailbox(const std::vector<Main::Structures::Mailbox>& mailbox, bool sent);
			void setReceivedGiftboxes(const std::vector<Main::Structures::Giftbox>& gifbox);
			std::optional<std::uint32_t> getItemIdFromGiftbox(std::uint32_t timestamp) const;
			void deleteGiftbox(std::uint32_t timestamp);

			// Rewards
			void setLatestWeeklyRewardDate(const std::string& date) { m_latestWeeklyRewardDay = date; }
			const std::string getLatestWeeklyRewardDate() const { return m_latestWeeklyRewardDay; }
			void setLatestMonthlyRewardDate(const std::string& date) { m_latestMonthlyRewardDay = date; }
			const std::string getLatestMonthlyRewardDate() const { return m_latestMonthlyRewardDay; }

			// Room info
			void setRoomNumber(std::uint16_t roomNumber);
			void setPartyRoomNumber(std::uint16_t clanRoomNumber);
			std::uint16_t getRoomNumber() const;
			std::uint16_t getPartyRoomNumber() const noexcept;
			void decreaseRoomNumber();
			void setIsInMatch(bool val);
			bool isInMatch() const;
			void leaveRoom();

			// Achievements
			void addAchievementTier1(std::uint32_t achievementId);

			// Trade system
			void lockTrade();
			bool hasPlayerLocked() const;
			void resetTradeInfo();
			void setCurrentlyTradingWithAccountId(std::uint32_t targetAccountId);
			std::uint32_t getCurrentlyTradingWithAccountId() const;
			bool addTradedItem(std::uint32_t itemId, const Main::Structures::ItemSerialInfo& serialInfo);
			void removeTradedItem(const Main::Structures::ItemSerialInfo& serialInfo);
			void resetTradedItems();
			const std::vector<TradedItem>& getTradedItems() const;


			std::string getPlayerInfoAsString() const
			{
				return "(PlayerName: " + std::string(m_accountInfo.nickname) + ", RoomNumber: " + std::to_string(m_roomNumber) + ", IsInMatch : " + std::to_string(m_isInMatch) + "\n";
			}
		};
	}
}

#endif
