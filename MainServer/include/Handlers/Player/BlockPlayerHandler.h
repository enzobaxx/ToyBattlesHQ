#ifndef BLOCK_PLAYER_HANDLER_H
#define BLOCK_PLAYER_HANDLER_H

#include "Network/Sessions/MainSession.h"
#include "Network/MainSessionManager.h"
#include "Persistence/MainDatabaseManager.h"
#include "Network/Packet.h"
#include <source_location>

namespace Main
{
	namespace Handlers
	{
		template<std::size_t N>
		inline void handlePlayerBlock(const Common::Network::Packet& request, std::shared_ptr<Main::Network::Session> session,
			Main::Network::SessionsManager& sessionsManager,
			Main::Persistence::MainScheduler& m_scheduler, const std::array<char, N>& targetAccountName)
		{
			START_BENCHMARK

			if (request.getOption() == 2)
			{
				if (auto targetSession = sessionsManager.findSessionByName(targetAccountName.data()))
				{
					if (targetSession->getAccountInfo().playerGrade >= Common::Enums::GRADE_ES)
					{
						session->sendMessage("Cannot block a staff member!");
						return;
					}
					if (const std::uint32_t targetAccountId = targetSession->getAccountInfo().accountID;
						session->blockAccount(targetAccountId, targetAccountName.data()) && session->getPlayer().getSocialInfo().isFriend(targetAccountId))
					{
						session->deleteFriend(targetAccountId);
						targetSession->deleteFriend(session->getAccountInfo().accountID, false);
					}
				}
				else if (const std::optional<std::uint32_t> targetAccountId = m_scheduler.immediatePersist(std::source_location::current(), 
					&Main::Persistence::PersistentDatabase::blockPlayerByNickname,
					session->getAccountInfo().accountID, targetAccountName.data());
					targetAccountId.has_value() && session->blockAccount(*targetAccountId, targetAccountName.data()) && session->getPlayer().getSocialInfo().isFriend(*targetAccountId))
				{
					session->deleteFriend(*targetAccountId);
				}
				else
				{
					session->sendMessage("(Block Info) Player not found or the target user is a team member", Main::Enums::INFO);
				}
			}

			END_BENCHMARK(handlePlayerBlock, session)
		}
	}
}

#endif
