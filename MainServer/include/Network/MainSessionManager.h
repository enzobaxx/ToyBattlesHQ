#ifndef MAIN_SESSIONS_MANAGER_H
#define MAIN_SESSIONS_MANAGER_H

#include <unordered_map>
#include <vector>
#include <functional>
#include "MainSession.h"
#include "../Classes/RoomsManager.h"
#include "../Classes/PartiesManager.h"

namespace Main
{
	namespace Network
	{
		class SessionsManager
		{
		private:
			std::unordered_map<std::uint64_t, std::shared_ptr<Main::Network::Session>> m_sessionsBySessionId;
			std::vector<std::shared_ptr<Main::Network::Session>> m_sessionsVector;
			Main::Classes::RoomsManager* roomsManager;
			Main::Classes::PartiesManager* m_partiesManager;

			Common::Network::Packet prepareMessage(const std::string& message) const;

		public:
			void setRoomsManager(Main::Classes::RoomsManager* roomsManager);

			void setClansManager(Main::Classes::PartiesManager* clansManager);

			void addSession(std::shared_ptr<Main::Network::Session> session);

			void removeSession(std::size_t sessionId);

			const std::unordered_map<std::uint64_t, std::shared_ptr<Main::Network::Session>>& getAllSessions() const;

			std::unordered_map<std::uint64_t, std::shared_ptr<Main::Network::Session>>& getAllSessions();

			std::vector<std::shared_ptr<Main::Network::Session>> getAllSessionsVec() const;

			void broadcast(const Common::Network::Packet& message) const;

			void broadcastExceptSelf(std::size_t selfSessionId, const Common::Network::Packet& message) const;

			void broadcastToLobbyExceptSelf(std::size_t selfSessionId, const Common::Network::Packet& message) const;

			void broadcastToClan(std::uint64_t selfSessionId, const Common::Network::Packet& message) const;

			void broadcastMessage(const std::string& message) const;
			void broadcastMessageExceptSelf(std::size_t selfSessionId, const std::string& message) const;
			void broadcastMessageToLobby(const std::string& message) const;

			std::shared_ptr<Main::Network::Session> findSessionByName(const char* nickname);
			std::shared_ptr<Main::Network::Session> getSessionByAccountId(std::uint32_t aid);
			std::shared_ptr<Main::Network::Session> getSessionBySessionId(std::size_t sessionId);

			std::uint32_t getTotalSessions() const;

			bool sendTo(std::size_t sessionId, const Common::Network::Packet& packet);

			Main::Classes::RoomsManager* getRoomsManager()
			{
				return roomsManager;
			}
		};
	}
}
#endif