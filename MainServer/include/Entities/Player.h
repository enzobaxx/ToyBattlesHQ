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
		struct Player
		{
			using AccountInfo = Main::Structures::AccountInfo;
			using ModerationInfo = Main::Structures::ModerationInfo;
			using MatchContext = Main::Structures::MatchContext;

			AccountInfo accountInfo{};
			Inventory inventory{ accountInfo };
			SocialInfo socialInfo{};
			TradeInfo tradeInfo{};
			ModerationInfo moderationInfo{};
			MatchContext matchContext{};
			Main::Classes::Mailbox mailbox{};
			Common::Enums::PlayerState playerState{};
			std::uint16_t ping{};
			std::string latestWeeklyRewardDay{};
			std::string latestMonthlyRewardDay{};
			std::uint32_t totalWrongPasswordReset{};
			std::uint32_t totalWrong2FaUsernameChange{};
			std::uint32_t totalWrongUsernameChange{};
			std::uint32_t totalWrong2FaReset{};
			std::string hwid{ "" };
			std::uint64_t hwidLastUpdatedTimestamp{};
			std::string gradedHwid{ "" };
			std::string gradedHwidSalt{ "" };

			void setPlayerName(const char* playerName);
			bool isInLobby() const;
			bool isInMatch() const;
			void leaveRoom();
			void storeBatteryObtainedInMatch();
		};
	}
}

#endif
