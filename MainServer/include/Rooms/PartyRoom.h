#ifndef CLASS_CLAN_HEADER
#define CLASS_CLAN_HEADER

#include <cstdint>
#include <vector>
#include <string>
#include <expected>
#include <optional>
#include <algorithm>

#include "Network/Sessions/MainSession.h"
#include "Structures/Clan/ClanStructures.h"
#include "Structures/Room/RoomPlayerInfo.h"
#include "Structures/Room/ClientRoomCreationInfo.h"
#include <source_location>
#include <cstring>

namespace Main
{
	namespace Classes
	{
		struct RemovePlayerResult
		{
			bool roomEmpty;
			size_t originalIndex;
		};

		class PartyRoom
		{
		private:
			Main::Structures::ClanRoomSettings m_settings{};
			Main::Structures::PartyInfo m_partyInfo{};
			bool m_isRegistered{};
			std::vector<std::pair<Main::Structures::PartyPlayerInfo, std::weak_ptr<Main::Network::Session>>> m_players{};
			Common::Enums::Team m_team{};
			std::uint16_t m_clanMatchRoomNumber = 0;

		public:
			PartyRoom() = default;

			explicit PartyRoom(std::shared_ptr<Main::Network::Session> session, std::uint16_t clanRoomNumber, const Main::ClientData::ClanRoomSettings& settings);

			void addPlayer(std::shared_ptr<Main::Network::Session> session);

			auto findPlayer(std::uint32_t sessionId)
			{
				return std::find_if(m_players.begin(), m_players.end(), [sessionId](const auto& player) {
					if (auto session = player.second.lock())
						return session->getAccountInfo().uniqueId.session == sessionId;
					return false;
					});
			}

			void setClanMatchRoomNumber(std::uint16_t num);
			std::uint16_t getClanMatchRoomNumber() const noexcept;
			bool isPlayerInRoom(std::uint32_t sessionId) const;
			void updatePartyStatus(bool hasStarted);
			bool hasMatchStarted() const noexcept;
			void removeAllPlayers();
			void broadcastChatMessage(const std::string& message);
			std::optional<std::uint32_t> getPlayerIndex(std::uint32_t sessionId);
			std::expected<RemovePlayerResult, std::string> removePlayer(std::uint32_t sessionId);
			std::uint32_t getRoomNumber() const noexcept;
			std::uint32_t getClanId() const noexcept;
			void updateMap(std::uint16_t map);
			std::shared_ptr<Main::Network::Session> getLeaderSession() const;
			Main::Structures::RoomSettings getRoomSettings() const;
			std::uint32_t getSpecificSetting() const noexcept;
			void updateMode(std::uint16_t mode);
			bool isLeader(std::uint32_t sessionId) const noexcept;
			bool canRegister() const noexcept;
			bool changeLeaderTo(std::uint16_t idx);
			std::optional<std::size_t> changeLeaderToFirstAvailable();
			void updatePlayersPerTeam(std::uint16_t playersPerTeam);
			std::pair<std::uint16_t, std::uint16_t> getRoomId() const;
			const Main::Structures::ClanRoomSettings& getSettings() const noexcept;
			bool isFull() const noexcept;
			std::size_t getPlayersSize() const noexcept;
			std::vector<Main::Structures::PartyPlayerInfo> getPlayers() const noexcept;
			std::vector<std::shared_ptr<Main::Network::Session>> getPlayerSessions() noexcept;
			const Main::Structures::PartyInfo& getClanMatchInfo() const noexcept;
			void broadcast(Common::Network::Packet& packet);
			void broadcastMessage(const std::string& message);
			bool isRegistered() const noexcept;
			void switchRegistered();
			void broadcastExceptSelf(const Common::Network::Packet& packet, std::uint16_t sessionId);
			std::optional<Main::Structures::RegisteredClanInfo> getInfo() const;
			void setTeam(Common::Enums::Team team);
			Common::Enums::Team getTeam() const noexcept;
			void storeStats(Main::Persistence::MainScheduler& scheduler, const Main::ClientData::ClientEndingMatchHeader& stats);

			/********** DEBUG PURPOSES /**********/
			std::string getPlayersNicknames() const;
			std::string getFormattedPartyInfo() const;
			void sendDebugMessage();
			/******** DEBUG PURPOSES END /********/
		};
	}
}

#endif
