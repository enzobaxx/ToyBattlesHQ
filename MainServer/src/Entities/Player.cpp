
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
		void Player::setPlayerName(const char* playerName)
		{
			strncpy(accountInfo.nickname, playerName, sizeof(accountInfo.nickname) - 1);
			accountInfo.nickname[sizeof(accountInfo.nickname) - 1] = '\0';
		}

		bool Player::isInLobby() const
		{
			return matchContext.roomNumber == 0;
		}

		bool Player::isInMatch() const
		{
			return (playerState == Common::Enums::STATE_NORMAL || playerState == Common::Enums::STATE_DYING);
		}

		void Player::leaveRoom()
		{
			matchContext.roomNumber = 0;
			matchContext.isInMatch = false;
			matchContext.batteryObtainedInMatch = 0;
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
	}
}
