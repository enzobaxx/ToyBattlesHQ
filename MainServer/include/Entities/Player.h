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
#include "Structures/ModerationInfo.h"
#include "Structures/MatchContext.h"
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
			using ModerationInfo = Main::Structures::ModerationInfo;
			using MatchContext = Main::Structures::MatchContext;

			Common::Enums::PlayerState m_playerState{};
			std::uint16_t m_ping{};
			std::string m_latestWeeklyRewardDay{};
			std::string m_latestMonthlyRewardDay{};

		public:
						AccountInfo accountInfo{};
			Inventory inventory{ accountInfo };
			SocialInfo socialInfo{};
			TradeInfo tradeInfo{};
			ModerationInfo moderationInfo{};
			MatchContext matchContext{};
			Main::Classes::Mailbox mailbox{};

						void setAccountInfo(const AccountInfo& info);
			std::uint32_t getAccountID() const;
			const char* const getPlayerName() const;
			void setPlayerName(const char* playerName);
			void setPlayerState(Common::Enums::PlayerState playerState);
			Common::Enums::PlayerState getPlayerState() const;
			void setPing(std::uint16_t ping);
			std::uint16_t getPing() const;
			bool isInLobby() const;

						void setLatestWeeklyRewardDate(const std::string& date) { m_latestWeeklyRewardDay = date; }
			const std::string getLatestWeeklyRewardDate() const { return m_latestWeeklyRewardDay; }
			void setLatestMonthlyRewardDate(const std::string& date) { m_latestMonthlyRewardDay = date; }
			const std::string getLatestMonthlyRewardDate() const { return m_latestMonthlyRewardDay; }

			// Room info
			void decreaseRoomNumber();
			bool isInMatch() const;
			void leaveRoom();
			void storeBatteryObtainedInMatch();

			// Achievements
			void addAchievementTier1(std::uint32_t achievementId);

			std::string getPlayerInfoAsString() const
			{
				return "(PlayerName: " + std::string(accountInfo.nickname) + ", RoomNumber: " + std::to_string(matchContext.roomNumber) + ", IsInMatch : " + std::to_string(matchContext.isInMatch) + "\n";
			}
		};
	}
}

#endif
