#include "Rooms/PartyRoom.h"

#include <algorithm>

namespace Main
{
	namespace Classes
	{
		PartyRoom::PartyRoom(std::shared_ptr<Main::Network::Session> session, std::uint16_t clanRoomNumber, const Main::ClientData::ClanRoomSettings& settings)
			: m_settings{ settings }
		{
			const auto& selfInfo = session->getAccountInfo();

			std::memcpy(m_partyInfo.leaderName, selfInfo.nickname, Common::Constants::maxNicknameSize);
			m_partyInfo.leaderLevel = selfInfo.playerLevel;
			m_partyInfo.clanRoomId = selfInfo.clanId;
			m_partyInfo.clanRoomNumber = clanRoomNumber;
			m_partyInfo.numPlayers = 1;

			Main::Structures::PartyPlayerInfo waitingPlayerInfo;
			waitingPlayerInfo.uid = selfInfo.uniqueId;
			waitingPlayerInfo.level = selfInfo.playerLevel;
			std::memcpy(waitingPlayerInfo.nickname, selfInfo.nickname, Common::Constants::maxNicknameSize);
			waitingPlayerInfo.totalClanDraws = selfInfo.clanDraws;
			waitingPlayerInfo.totalClanLosses = selfInfo.clanLosses;
			waitingPlayerInfo.totalClanWins = selfInfo.clanWins;
			waitingPlayerInfo.clanContribution = selfInfo.clanContribution;

			m_players.emplace_back(waitingPlayerInfo, session);
			session->getPlayer().getMatchContext().partyRoomNumber = m_partyInfo.clanRoomNumber;

			session->sendMessage("Created party room number: " + std::to_string(clanRoomNumber), Main::Enums::TIP);
		}

		void PartyRoom::addPlayer(std::shared_ptr<Main::Network::Session> session)
		{
			const auto& selfInfo = session->getAccountInfo();

			Main::Structures::PartyPlayerInfo waitingPlayerInfo;
			waitingPlayerInfo.uid = selfInfo.uniqueId;
			waitingPlayerInfo.level = selfInfo.playerLevel;
			std::memcpy(waitingPlayerInfo.nickname, selfInfo.nickname, Common::Constants::maxNicknameSize);
			waitingPlayerInfo.totalClanDraws = selfInfo.clanDraws;
			waitingPlayerInfo.totalClanLosses = selfInfo.clanLosses;
			waitingPlayerInfo.totalClanWins = selfInfo.clanWins;
			waitingPlayerInfo.clanContribution = selfInfo.clanContribution;

			m_players.emplace_back(waitingPlayerInfo, session);
			++m_partyInfo.numPlayers;
			session->getPlayer().getMatchContext().partyRoomNumber = m_partyInfo.clanRoomNumber;

			session->sendMessage("Joined party room number: " + std::to_string(m_partyInfo.clanRoomNumber), Main::Enums::TIP);
		}

		void PartyRoom::setClanMatchRoomNumber(std::uint16_t num)
		{
			m_clanMatchRoomNumber = num;
			for (auto& [info, session] : m_players)
			{
				if (auto s = session.lock())
				{
					if (num == 0)
					{
						s->leaveRoom();
						m_isRegistered = false;
					}
					s->getPlayer().getMatchContext().roomNumber = num;
				}
			}
		}

		std::uint16_t PartyRoom::getClanMatchRoomNumber() const noexcept
		{
			return m_clanMatchRoomNumber;
		}

		bool PartyRoom::isPlayerInRoom(std::uint32_t sessionId) const
		{
			for (const auto& [playerInfo, weakSession] : m_players)
			{
				if (auto session = weakSession.lock())
				{
					if (session->getAccountInfo().uniqueId.session == sessionId)
						return true;
				}
			}
			return false;
		}

		void PartyRoom::updatePartyStatus(bool hasStarted)
		{
			m_partyInfo.hasMatchStarted = hasStarted;
		}

		bool PartyRoom::hasMatchStarted() const noexcept
		{
			return m_partyInfo.hasMatchStarted;
		}

		void PartyRoom::removeAllPlayers()
		{
			Common::Network::Packet removePlayerPacket;
			removePlayerPacket.setTcpHeader(0, Common::Enums::NO_ENCRYPTION);
			removePlayerPacket.setCommand(111, 0, 1, 0);

			for (auto& [partyInfo, weakSession] : m_players)
			{
				if (auto session = weakSession.lock())
				{
					session->asyncWrite(removePlayerPacket);
					session->getPlayer().getMatchContext().partyRoomNumber = 0;
					session->leaveRoom();
				}
			}

			m_partyInfo = {};
			m_settings = {};
			m_isRegistered = {};
			m_team = {};
			m_players.clear();
		}

		void PartyRoom::broadcastChatMessage(const std::string& message)
		{
			for (auto& [partyInfo, weakSession] : m_players)
			{
				if (auto session = weakSession.lock())
					session->sendMessage(message);
			}
		}

		std::optional<std::uint32_t> PartyRoom::getPlayerIndex(std::uint32_t sessionId)
		{
			for (std::uint32_t idx = 0; const auto & [_, weakSession] : m_players)
			{
				if (auto session = weakSession.lock(); session && session->getAccountInfo().uniqueId.session == sessionId)
				{
					return idx;
				}
				++idx;
			}
			return std::nullopt;
		}

		std::expected<RemovePlayerResult, std::string> PartyRoom::removePlayer(std::uint32_t sessionId)
		{
			auto it = findPlayer(sessionId);
			if (it == m_players.end())
			{
				broadcastMessage("Player to be removed was not found in the room, please report this issue with steps-to-reproduce");
				return std::unexpected("Player not found");
			}

			auto lockedSession = it->second.lock();
			const auto sessionToRemove = lockedSession->getAccountInfo().uniqueId.session;
			size_t originalIndex = std::distance(m_players.begin(), it);
			size_t lastIndex = m_players.size() - 1;

			if (isLeader(sessionToRemove) && m_players.size() > 1)
			{
				auto newLeaderIdxOpt = changeLeaderToFirstAvailable();
				if (!newLeaderIdxOpt)
				{
					broadcastMessage("newLeaderIdxOpt nullopt - New leader could not be chosen, please report this issue with steps-to-reproduce");
					return std::unexpected("Failed to find new leader");
				}

				if (*newLeaderIdxOpt != lastIndex)
				{
					std::swap(m_players[*newLeaderIdxOpt], m_players[lastIndex]); // Leaving player always swapped with last player in the list client side
				}
				originalIndex = *newLeaderIdxOpt;  // The old leader is now in this index

				Common::Network::Packet response;
				response.setTcpHeader(0, Common::Enums::NO_ENCRYPTION);
				response.setCommand(114, 0, 0, *newLeaderIdxOpt);
				broadcast(response);
			}
			else if (!isLeader(sessionToRemove) && m_players.size() > 1)
			{
				if (originalIndex != lastIndex)
				{
					std::swap(m_players[originalIndex], m_players[lastIndex]); // Leaving player always swapped with last player in the list client side
				}
			}

			it = findPlayer(sessionId);
			if (it == m_players.end())
			{
				broadcastMessage("[2] Player to be removed was not found in the room, please report this issue with steps-to-reproduce");
				return std::unexpected("Player disappeared during swap");
			}

			if (auto session = it->second.lock())
			{
				session->getPlayer().getMatchContext().partyRoomNumber = 0;
				session->leaveRoom();
				session->sendMessage("You left the party (party number: " + std::to_string(m_partyInfo.clanRoomNumber) + ")");
			}

			if (m_partyInfo.numPlayers)
				--m_partyInfo.numPlayers;

			m_players.erase(it);
			return RemovePlayerResult{ m_players.empty(), originalIndex };
		}

		std::uint32_t PartyRoom::getRoomNumber() const noexcept
		{
			return m_partyInfo.clanRoomNumber;
		}

		std::uint32_t PartyRoom::getClanId() const noexcept
		{
			return m_partyInfo.clanRoomId;
		}

		void PartyRoom::updateMap(std::uint16_t map)
		{
			if (map >= Common::Enums::MAPS_MAX) return;
			m_settings.map = map;
		}

		std::shared_ptr<Main::Network::Session> PartyRoom::getLeaderSession() const
		{
			return m_players.empty() ? nullptr : m_players[0].second.lock();
		}

		Main::Structures::RoomSettings PartyRoom::getRoomSettings() const
		{
			const std::uint32_t time = m_settings.mode == Common::Enums::Clan_TeamDeathMatch ? 10 : 2;
			return Main::Structures::RoomSettings{ time, Common::Enums::WeaponRestriction::All, 0, m_settings.mode, 0,
				0, m_settings.playersPerTeam, m_settings.map, 0
			};
		}

		std::uint32_t PartyRoom::getSpecificSetting() const noexcept
		{
			return m_settings.mode == Common::Enums::Clan_TeamDeathMatch ? 80 : 5;
		}

		void PartyRoom::updateMode(std::uint16_t mode)
		{
			if (mode >= Common::Enums::CLANMODES_MAX) return;
			m_settings.mode = mode;
		}

		bool PartyRoom::isLeader(std::uint32_t sessionId) const noexcept
		{
			if (m_players.empty())
				return false;

			if (auto leaderSession = m_players[0].second.lock())
				return leaderSession->getAccountInfo().uniqueId.session == sessionId;

			return false;
		}

		bool PartyRoom::canRegister() const noexcept
		{
			return m_players.size() >= m_partyInfo.maxPlayers;
		}

		bool PartyRoom::changeLeaderTo(std::uint16_t idx)
		{
			if (idx == 0 || idx >= m_players.size())
			{
				return false;
			}

			auto sessionPtr = m_players[idx].second.lock();
			if (!sessionPtr) return false;

			broadcastMessage("Leader changed to: " + std::string(sessionPtr->getAccountInfo().nickname));

			std::swap(m_players[0], m_players[idx]);
			m_partyInfo.leaderLevel = m_players[0].first.level;
			std::memcpy(m_partyInfo.leaderName, m_players[0].first.nickname, Common::Constants::maxNicknameSize);
			return true;
		}

		std::optional<std::size_t> PartyRoom::changeLeaderToFirstAvailable()
		{
			for (std::size_t idx = 1; idx < m_players.size(); ++idx)
			{
				if (auto session = m_players[idx].second.lock())
				{
					if (!changeLeaderTo(static_cast<std::uint16_t>(idx)))
						continue;

					return idx;
				}
			}
			return std::nullopt;
		}

		void PartyRoom::updatePlayersPerTeam(std::uint16_t playersPerTeam)
		{
			m_settings.playersPerTeam = playersPerTeam;
			m_partyInfo.maxPlayers = playersPerTeam;
		}

		std::pair<std::uint16_t, std::uint16_t> PartyRoom::getRoomId() const
		{
			return std::pair{ m_partyInfo.clanRoomId, m_partyInfo.clanRoomNumber };
		}

		const Main::Structures::ClanRoomSettings& PartyRoom::getSettings() const noexcept
		{
			return m_settings;
		}

		bool PartyRoom::isFull() const noexcept
		{
			return m_partyInfo.numPlayers >= m_partyInfo.maxPlayers;
		}

		std::size_t PartyRoom::getPlayersSize() const noexcept
		{
			return m_players.size();
		}

		std::vector<Main::Structures::PartyPlayerInfo> PartyRoom::getPlayers() const noexcept
		{
			std::vector<Main::Structures::PartyPlayerInfo> waitingPlayers;
			for (const auto& [playerInfo, session] : m_players)
			{
				waitingPlayers.push_back(playerInfo);
			}
			return waitingPlayers;
		}

		std::vector<std::shared_ptr<Main::Network::Session>> PartyRoom::getPlayerSessions() noexcept
		{
			std::vector<std::shared_ptr<Main::Network::Session>> waitingPlayers;
			for (const auto& [playerInfo, sessionWeak] : m_players)
			{
				if (auto session = sessionWeak.lock())
				{
					waitingPlayers.push_back(session);
				}
			}
			return waitingPlayers;
		}

		const Main::Structures::PartyInfo& PartyRoom::getClanMatchInfo() const noexcept
		{
			return m_partyInfo;
		}

		void PartyRoom::broadcast(Common::Network::Packet& packet)
		{
			for (const auto& [u, sessionWeak] : m_players)
			{
				if (auto session = sessionWeak.lock())
				{
					packet.setTcpHeader(session->getId(), Common::Enums::NO_ENCRYPTION);
					session->asyncWrite(packet);
				}
			}
		}

		void PartyRoom::broadcastMessage(const std::string& message)
		{
			for (const auto& [u, sessionWeak] : m_players)
			{
				if (auto session = sessionWeak.lock())
				{
					session->sendMessage(message, Main::Enums::TIP);
				}
			}
		}

		bool PartyRoom::isRegistered() const noexcept
		{
			return m_isRegistered;
		}

		void PartyRoom::switchRegistered()
		{
			m_isRegistered = !m_isRegistered;
		}

		void PartyRoom::broadcastExceptSelf(const Common::Network::Packet& packet, std::uint16_t sessionId)
		{
			for (const auto& [u, sessionWeak] : m_players)
			{
				if (u.uid.session == sessionId) continue;
				if (auto session = sessionWeak.lock())
				{
					session->asyncWrite(packet);
				}
			}
		}

		std::optional<Main::Structures::RegisteredClanInfo> PartyRoom::getInfo() const
		{
			if (m_players.empty())
			{
				return std::nullopt;
			}

			auto session = m_players[0].second.lock();
			if (!session)
			{
				return std::nullopt;
			}

			const auto& leaderInfo = session->getAccountInfo();
			return Main::Structures::RegisteredClanInfo{
				static_cast<std::uint64_t>(m_settings.mode),
				static_cast<std::uint64_t>(m_settings.playersPerTeam * 2),
				static_cast<std::uint64_t>(m_settings.map),
				static_cast<std::uint64_t>(leaderInfo.playerLevel),
				static_cast<std::uint16_t>(leaderInfo.clanLogoFrontId),
				static_cast<std::uint16_t>(leaderInfo.clanLogoBackId),
				leaderInfo.clanName,
				leaderInfo.nickname,
				m_partyInfo.clanRoomId,
				m_partyInfo.clanRoomNumber
			};
		}

		void PartyRoom::setTeam(Common::Enums::Team team)
		{
			m_team = team;
		}

		Common::Enums::Team PartyRoom::getTeam() const noexcept
		{
			return m_team;
		}

		void PartyRoom::storeStats(Main::Persistence::MainScheduler& scheduler, const Main::ClientData::ClientEndingMatchHeader& stats)
		{
			Main::Enums::MatchEnd type = Main::Enums::MatchEnd::MATCH_DRAW;

			if ((stats.blueScore > stats.redScore && m_team == Common::Enums::TEAM_BLUE)
				|| (stats.redScore > stats.blueScore && m_team == Common::Enums::TEAM_RED))
			{
				type = Main::Enums::MATCH_WON;
			}
			else if ((stats.blueScore > stats.redScore && m_team == Common::Enums::TEAM_RED)
				|| (stats.redScore > stats.blueScore && m_team == Common::Enums::TEAM_BLUE))
			{
				type = Main::Enums::MATCH_LOST;
			}

			scheduler.immediatePersist(std::source_location::current(),
				&Main::Persistence::PersistentDatabase::updateClanStats, m_partyInfo.clanRoomId, type);
		}

		std::string PartyRoom::getPlayersNicknames() const
		{
			if (m_players.empty())
				return "Players: None";

			std::string result = "Waiting Players: ";
			for (const auto& [playerInfo, weakSession] : m_players)
			{
				if (result != "Players: ")
					result += ", ";

				result += playerInfo.toString();

				if (auto session = weakSession.lock())
				{
					result += " (Room: " + std::to_string(session->getPlayer().getMatchContext().roomNumber) +
						", PartyRoom: " + std::to_string(session->getPlayer().getMatchContext().partyRoomNumber) + ")";
				}
				else
				{
					result += " (Session expired)";
				}
			}
			return result;
		}

		std::string PartyRoom::getFormattedPartyInfo() const
		{
			return "Party Info: " + m_partyInfo.toString();
		}

		void PartyRoom::sendDebugMessage()
		{
			broadcastMessage(getPlayersNicknames());
			broadcastMessage(getFormattedPartyInfo());
		}
	}
}
