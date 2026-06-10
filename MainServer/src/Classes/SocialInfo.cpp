#include "../../include/Classes/SocialInfo.h"
#include "../../include/Network/MainSession.h"
#include "Utils/Constants.h"

#include <algorithm>
#include <cstring>

namespace Main
{
	namespace Classes
	{
		using Session = Main::Network::Session;
		using BlockedPlayer = Main::Structures::BlockedPlayer;
		using Friend = Main::Structures::Friend;

		const std::vector<Friend> SocialInfo::getFriendlist() const
		{
			std::vector<Friend> ret;
			for (const auto& [ffriend, unused] : m_friends)
			{
				ret.push_back(ffriend);
			}
			return ret;
		}

		std::unordered_map<Friend, std::weak_ptr<Session>>& SocialInfo::getFriendSessions()
		{
			return m_friends;
		}

		void SocialInfo::setFriendList(const std::vector<Friend>& friendlist)
		{
			for (const auto& currentFriend : friendlist)
			{
				m_friends[currentFriend] = std::weak_ptr<Session>{};
			}
		}

		void SocialInfo::updateFriend(const Friend& targetFriend, std::shared_ptr<Main::Network::Session> targetSession, bool remove)
		{
			if (m_friends.contains(targetFriend)) m_friends.erase(targetFriend);
			m_friends[targetFriend] = remove ? std::weak_ptr<Session>{} : std::weak_ptr<Session>{ targetSession };
		}

		bool SocialInfo::deleteFriend(std::uint32_t targetAccountId)
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

		void SocialInfo::addOfflineFriend(const Main::Structures::Friend& ffriend)
		{
			m_friends[ffriend] = std::weak_ptr<Session>{};
		}

		std::optional<Main::Structures::Friend> SocialInfo::addOnlineFriend(std::shared_ptr<Main::Network::Session> session)
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

		bool SocialInfo::isFriend(std::uint32_t accountId) const
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

		bool SocialInfo::blockAccount(std::uint32_t accountId, const char* nickname)
		{
			if (m_blockedAccounts.size() >= Common::Constants::maxFriends)
				return false;

			Main::Structures::BlockedPlayer blocked{ accountId };
			std::memcpy(blocked.targetNickname, nickname, sizeof(blocked.targetNickname));
			m_blockedAccounts.push_back(blocked);
			return true;
		}

		bool SocialInfo::unblockAccount(std::uint32_t accountId)
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

		bool SocialInfo::hasBlocked(std::uint32_t accountId) const
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

		const std::vector<Main::Structures::BlockedPlayer>& SocialInfo::getBlockedPlayers() const
		{
			return m_blockedAccounts;
		}

		void SocialInfo::setBlockedPlayers(const std::vector<Main::Structures::BlockedPlayer>& blockedPlayers)
		{
			m_blockedAccounts = blockedPlayers;
		}
	}
}
