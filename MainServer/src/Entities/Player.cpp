
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

		void Player::setAccountInfo(const AccountInfo& accountInfo)
		{
			m_accountInfo = accountInfo;
		}

		void Player::storeBatteryObtainedInMatch()
		{
			if (m_accountInfo.battery + m_matchContext.batteryObtainedInMatch >= m_accountInfo.maxBattery)
			{
				m_accountInfo.battery = m_accountInfo.maxBattery;
			}
			else
			{
				m_accountInfo.battery += m_matchContext.batteryObtainedInMatch;
			}
			m_matchContext.batteryObtainedInMatch = 0;
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

		void Player::setPlayerName(const char* playerName)
		{
			strncpy(m_accountInfo.nickname, playerName, sizeof(m_accountInfo.nickname) - 1);
			m_accountInfo.nickname[sizeof(m_accountInfo.nickname) - 1] = '\0';
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
			return m_matchContext.roomNumber == 0;
		}

		void Player::addAchievementTier1(std::uint32_t achievementId)
		{
			m_accountInfo.achievements.setAchievementTier1(achievementId);
		}



		// Room info
		bool Player::isInMatch() const
		{
			return (m_playerState == Common::Enums::STATE_NORMAL || m_playerState == Common::Enums::STATE_DYING);
		}

		void Player::leaveRoom()
		{
			m_matchContext.roomNumber = 0;
			m_matchContext.isInMatch = false;
			m_matchContext.batteryObtainedInMatch = 0;
		}

		void Player::decreaseRoomNumber()
		{
			if (m_matchContext.roomNumber > 0)
			{
				--m_matchContext.roomNumber;
			}
		}

	}
}
