
#include "Entities/Player.h"
#include <unordered_map>
#include <vector>
#include <algorithm>
#include "Network/Sessions/MainSession.h"
#include "Structures/AccountInfo/MuteInfo.h"
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
		using TradedItem = Main::Structures::TradeBasicItem;

		void Player::setAccountInfo(const AccountInfo& info)
		{
			accountInfo = info;
		}

		void Player::storeBatteryObtainedInMatch()
		{
			if (accountInfo.battery + matchContext.batteryObtainedInMatch >= accountInfo.maxBattery)
			{
				accountInfo.battery = accountInfo.maxBattery;
			}
			else
			{
				accountInfo.battery += matchContext.batteryObtainedInMatch;
			}
			matchContext.batteryObtainedInMatch = 0;
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
			return accountInfo.accountID;
		}

		const char* const Player::getPlayerName() const
		{
			return accountInfo.nickname;
		}

		void Player::setPlayerName(const char* playerName)
		{
			strncpy(accountInfo.nickname, playerName, sizeof(accountInfo.nickname) - 1);
			accountInfo.nickname[sizeof(accountInfo.nickname) - 1] = '\0';
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
			return matchContext.roomNumber == 0;
		}

		void Player::addAchievementTier1(std::uint32_t achievementId)
		{
			accountInfo.achievements.setAchievementTier1(achievementId);
		}



		// Room info
		bool Player::isInMatch() const
		{
			return (m_playerState == Common::Enums::STATE_NORMAL || m_playerState == Common::Enums::STATE_DYING);
		}

		void Player::leaveRoom()
		{
			matchContext.roomNumber = 0;
			matchContext.isInMatch = false;
			matchContext.batteryObtainedInMatch = 0;
		}

		void Player::decreaseRoomNumber()
		{
			if (matchContext.roomNumber > 0)
			{
				--matchContext.roomNumber;
			}
		}

	}
}
