#ifndef PLAYER_CLASS_H
#define PLAYER_CLASS_H

#include "Structures/Item/MainEquippedItem.h"
#include "Structures/Item/MainBoughtItem.h"
#include "Structures/AccountInfo/MainAccountInfo.h"
#include "Structures/PlayerLists/Friend.h"
#include "Structures/Mailbox/Mailbox.h"
#include "Structures/PlayerLists/BlockedPlayer.h"
#include "Structures/Item/MainItem.h"
#include "Persistence/MainScheduler.h"
#include "Structures/TradeSystem/TradeSystemItem.h"
#include "Structures/ClientData/Structures.h"
#include "Entities/Inventory.h"
#include "Entities/SocialInfo.h"
#include "Entities/TradeInfo.h"
#include "Entities/Mailbox.h"

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

			AccountInfo m_accountInfo{};
			Inventory m_inventory{ m_accountInfo };
			SocialInfo m_socialInfo{};
			TradeInfo m_tradeInfo{};
			Main::Classes::Mailbox m_mailbox{};
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

			// Other
			std::uint16_t m_roomNumber{};
			std::uint16_t m_partyRoomNumber{};
			bool m_isInMatch{};
			std::uint32_t m_batteryObtainedInMatch{};

		public:
			// Inventory
			Inventory& getInventory() noexcept { return m_inventory; }
			const Inventory& getInventory() const noexcept { return m_inventory; }

			// Social (friends + blocked)
			SocialInfo& getSocialInfo() noexcept { return m_socialInfo; }
			const SocialInfo& getSocialInfo() const noexcept { return m_socialInfo; }

			// Trade system
			TradeInfo& getTradeInfo() noexcept { return m_tradeInfo; }
			const TradeInfo& getTradeInfo() const noexcept { return m_tradeInfo; }

			// Mailbox / giftbox
			Main::Classes::Mailbox& getMailbox() noexcept { return m_mailbox; }
			const Main::Classes::Mailbox& getMailbox() const noexcept { return m_mailbox; }

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


			std::uint32_t addBattery(std::uint32_t battery);



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

			std::string getPlayerInfoAsString() const
			{
				return "(PlayerName: " + std::string(m_accountInfo.nickname) + ", RoomNumber: " + std::to_string(m_roomNumber) + ", IsInMatch : " + std::to_string(m_isInMatch) + "\n";
			}
		};
	}
}

#endif
