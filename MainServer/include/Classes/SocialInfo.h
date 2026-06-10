#ifndef SOCIAL_INFO_CLASS_H
#define SOCIAL_INFO_CLASS_H

#include "../Structures/PlayerLists/Friend.h"
#include "../Structures/PlayerLists/BlockedPlayer.h"

#include <unordered_map>
#include <vector>
#include <memory>
#include <optional>
#include <cstdint>

namespace Common { namespace Network { class Session; } }
namespace Main { namespace Network { class Session; } }
namespace Main
{
	namespace Classes
	{
		class SocialInfo
		{
		private:
			using Session = Main::Network::Session;
			using BlockedPlayer = Main::Structures::BlockedPlayer;
			using Friend = Main::Structures::Friend;

			std::unordered_map<Friend, std::weak_ptr<Session>> m_friends;
			std::vector<BlockedPlayer> m_blockedAccounts{};

		public:
			// Friends
			const std::vector<Friend> getFriendlist() const;
			std::unordered_map<Friend, std::weak_ptr<Session>>& getFriendSessions();
			void setFriendList(const std::vector<Friend>& friendlist);
			void updateFriend(const Friend& targetFriend, std::shared_ptr<Main::Network::Session> targetSession, bool remove = false);
			// call once with default "persist", since removeFriend removes the friend for both players
			bool deleteFriend(std::uint32_t targetAccountId);
			void addOfflineFriend(const Main::Structures::Friend& ffriend);
			std::optional<Main::Structures::Friend> addOnlineFriend(std::shared_ptr<Main::Network::Session> session);
			bool isFriend(std::uint32_t accountId) const;

			// Blocked players
			bool blockAccount(std::uint32_t accountId, const char* nickname);
			bool unblockAccount(std::uint32_t accountId);
			bool hasBlocked(std::uint32_t accountId) const;
			const std::vector<Main::Structures::BlockedPlayer>& getBlockedPlayers() const;
			void setBlockedPlayers(const std::vector<Main::Structures::BlockedPlayer>& blockedPlayers);
		};
	}
}

#endif
