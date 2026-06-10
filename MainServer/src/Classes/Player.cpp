
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



		void Player::addAchievementTier1(std::uint32_t achievementId)
		{
			m_accountInfo.achievements.setAchievementTier1(achievementId);
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
