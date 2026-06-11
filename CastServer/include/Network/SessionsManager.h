#ifndef CAST_SESSIONS_MANAGER_H
#define CAST_SESSIONS_MANAGER_H

#include <unordered_map>
#include <functional>
#include "Network/Sessions/CastSession.h"
#include <vector>
#include "Structures/AccountInfo/MainAccountInfo.h"
#include "Managers/RoomsManager.h"

namespace Cast
{
	namespace Network
	{
		class SessionsManager
		{
		private:
			std::unordered_map<std::uint64_t, std::shared_ptr<Cast::Network::Session>> m_sessionsBySessionId;
			Cast::Classes::RoomsManager* m_roomsManager;


		public:
			void setRoomsManager(Cast::Classes::RoomsManager* roomsManager);

			void addSession(std::shared_ptr<Cast::Network::Session> session, std::size_t sessionId);

			void removeSession(std::size_t sessionId);

			std::shared_ptr<Cast::Network::Session> getSession(std::size_t sessionId);

			const auto getAllSessions() const { return m_sessionsBySessionId; }
		};
	}
}
#endif
