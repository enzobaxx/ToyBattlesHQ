#ifndef FRIENDS_GENERAL_HANDLER_HEADER
#define FRIENDS_GENERAL_HANDLER_HEADER

#include "Network/Sessions/MainSession.h"
#include "Network/MainSessionManager.h"
#include "MainEnums.h"
#include "Structures/PlayerLists/Friend.h"
#include "Network/Packet.h"
#include <cstring>

namespace Main
{
	namespace Handlers
	{
        inline void handleGeneralFriendRequests(const Common::Network::Packet& request, std::shared_ptr<Main::Network::Session> session,
            Main::Network::SessionsManager& sessionsManager)
        {
            START_BENCHMARK
            const auto& accountInfo = session->getAccountInfo();
            auto* data = request.getData(); // requestSent: { targetNickname }, requestAccepted: { targetUniqueId, targetAccountId, targetNickname }

            if (request.getExtra() == Main::Enums::ClientFriendExtra::FRIEND_REQUEST_SENT && request.getOption() == 2)
            {
                const char* targetNickname = reinterpret_cast<const char*>(data);
                session->sendFriendRequest(sessionsManager.findSessionByName(targetNickname), targetNickname);
            }
            else if (request.getExtra() == Main::Enums::ClientFriendExtra::INCOMING_FRIEND_REQUEST_ACCEPTED)
            {
                std::uint32_t accountId = Main::Details::parseData<std::uint32_t>(request, sizeof(accountInfo.uniqueId)); 
                Main::Structures::Friend target{ {}, accountId };
                std::memcpy(target.targetNickname, data + sizeof(accountInfo.uniqueId) + sizeof(accountInfo.accountID), 16);
                session->acceptFriendRequest(sessionsManager.getSessionByAccountId(accountId), target, data);
            }
            END_BENCHMARK(handleGeneralFriendRequest, session)
        }

        inline void handleFriendDeletion(const Common::Network::Packet& request, std::shared_ptr<Main::Network::Session> session,
            Main::Network::SessionsManager& sessionsManager,
            std::uint32_t targetAccountIdToDelete)
        {
            START_BENCHMARK
            session->deleteFriend(targetAccountIdToDelete);
            if (auto foundSession = sessionsManager.getSessionByAccountId(targetAccountIdToDelete))
            {
                foundSession->deleteFriend(session->getAccountInfo().accountID, false);
            }
            END_BENCHMARK(handleFriendDeletion, session)
        }
	}
}

#endif