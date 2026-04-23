#ifndef CLASS_CLAN_HEADER
#define CLASS_CLAN_HEADER

#include <cstdint>
#include <vector>
#include <string>
#include <expected>

#include "../Network/MainSession.h"
#include "../Structures/Clan/ClanStructures.h"
#include "../Structures/Room/RoomPlayerInfo.h"
#include "../Structures/Room/ClientRoomCreationInfo.h"
#include <source_location>
#include <expected>
#include <cstring> 

namespace Main
{
	namespace Classes
	{
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

			explicit PartyRoom(std::shared_ptr<Main::Network::Session> session, std::uint16_t clanRoomNumber, const Main::ClientData::ClanRoomSettings& settings)
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
				session->setPartyRoomNumber(m_partyInfo.clanRoomNumber);

				session->sendMessage("Created party room number: " + std::to_string(clanRoomNumber), Main::Enums::TIP);
				sendDebugMessage();
				//logPartyState("Party Created");
			}

			void addPlayer(std::shared_ptr<Main::Network::Session> session)
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
				session->setPartyRoomNumber(m_partyInfo.clanRoomNumber);

				session->sendMessage("Joined party room number: " + std::to_string(m_partyInfo.clanRoomNumber), Main::Enums::TIP);
				sendDebugMessage();
				//logPartyState("Player Added");
			}

			auto findPlayer(std::uint32_t sessionId)
			{
				return std::find_if(m_players.begin(), m_players.end(), [sessionId](const auto& player) {
					if (auto session = player.second.lock())
						return session->getAccountInfo().uniqueId.session == sessionId;
					return false;
					});
			}

			void setClanMatchRoomNumber(std::uint16_t num)
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
						s->setRoomNumber(num);
					}
				}
				//logPartyState("ClanMatchRoomNumber Set");
			}

			std::uint16_t getClanMatchRoomNumber() const noexcept
			{
				return m_clanMatchRoomNumber;
			}

			bool isPlayerInRoom(std::uint32_t sessionId) const
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

			void updatePartyStatus(bool hasStarted)
			{
				m_partyInfo.hasMatchStarted = hasStarted;
				//logPartyState("Party Status Updated");
			}

			bool hasMatchStarted() const noexcept
			{
				return m_partyInfo.hasMatchStarted;
			}

			void removeAllPlayers()
			{
				Common::Network::Packet removePlayerPacket;
				removePlayerPacket.setTcpHeader(0, Common::Enums::NO_ENCRYPTION);
				removePlayerPacket.setCommand(111, 0, 1, 0);

				for (auto& [partyInfo, weakSession] : m_players)
				{
					if (auto session = weakSession.lock())
					{
						session->asyncWrite(removePlayerPacket);
						session->setPartyRoomNumber(0);
						session->leaveRoom();
					}
				}

				m_partyInfo = {};
				m_settings = {};
				m_isRegistered = {};
				m_team = {};
				m_players.clear();
				//logPartyState("All Players Removed");
			}

			void broadcastChatMessage(const std::string& message)
			{
				for (auto& [partyInfo, weakSession] : m_players)
				{
					if (auto session = weakSession.lock())
						session->sendMessage(message);
				}
			}

			std::optional<std::uint32_t> getPlayerIndex(std::uint32_t sessionId)
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

			std::optional<bool> removePlayer(std::uint32_t sessionId)
			{
				auto it = findPlayer(sessionId);
				if (it == m_players.end())
				{
					broadcastMessage("Player to be removed was not found in the room, please report this issue with steps-to-reproduce");
					return std::nullopt;
				}

				auto lockedSession = it->second.lock();
				const auto sessionToRemove = lockedSession->getAccountInfo().uniqueId.session;

				if (isLeader(sessionToRemove))
				{
					auto newLeaderIdxOpt = changeLeaderToFirstAvailable();
					if (!newLeaderIdxOpt)
					{
						broadcastMessage("newLeaderIdxOpt nullopt - New leader could not be chosen, please report this issue with steps-to-reproduce");
						return std::nullopt;
					}

					if (*newLeaderIdxOpt != 0)
					{
						Common::Network::Packet response;
						response.setTcpHeader(0, Common::Enums::NO_ENCRYPTION);
						response.setCommand(114, 0, 0, *newLeaderIdxOpt);
						broadcast(response);
					}
				}

				it = findPlayer(sessionId);
				if (it == m_players.end())
				{
					broadcastMessage("[2] Player to be removed was not found in the room, please report this issue with steps-to-reproduce");
					return std::nullopt;
				}

				if (auto session = it->second.lock())
				{
					session->setPartyRoomNumber(0);
					session->leaveRoom();
					session->sendMessage("You left the party (party number: " + std::to_string(m_partyInfo.clanRoomNumber) + ")");
				}

				if (m_partyInfo.numPlayers)
					--m_partyInfo.numPlayers;

				sendDebugMessage();

				m_players.erase(it);
				//logPartyState("Player Removed");
				return m_players.empty();
			}

			std::uint32_t getRoomNumber() const noexcept
			{
				return m_partyInfo.clanRoomNumber;
			}

			std::uint32_t getClanId() const	noexcept
			{
				return m_partyInfo.clanRoomId;
			}

			void updateMap(std::uint16_t map)
			{
				if (map >= Common::Enums::MAPS_MAX) return;
				m_settings.map = map;
				//logPartyState("Map Updated");
			}

			std::shared_ptr<Main::Network::Session> getLeaderSession() const
			{
				return m_players.empty() ? nullptr : m_players[0].second.lock();
			}

			Main::Structures::RoomSettings getRoomSettings() const
			{
				const std::uint32_t time = m_settings.mode == Common::Enums::Clan_TeamDeathMatch ? 10 : 2;
				return Main::Structures::RoomSettings{ time, Common::Enums::WeaponRestriction::All, 0, m_settings.mode, 0,
					0, m_settings.playersPerTeam, m_settings.map, 0
				};
			}

			std::uint32_t getSpecificSetting() const noexcept
			{
				return m_settings.mode == Common::Enums::Clan_TeamDeathMatch ? 80 : 5;
			}

			void updateMode(std::uint16_t mode)
			{
				if (mode >= Common::Enums::CLANMODES_MAX) return;
				m_settings.mode = mode;
				//logPartyState("Mode Updated");
			}

			bool isLeader(std::uint32_t sessionId) const noexcept
			{
				if (m_players.empty())
					return false;

				if (auto leaderSession = m_players[0].second.lock())
					return leaderSession->getAccountInfo().uniqueId.session == sessionId;

				return false;
			}

			bool canRegister() const noexcept
			{
				return m_players.size() >= m_partyInfo.maxPlayers;
			}

			bool changeLeaderTo(std::uint16_t idx)
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
				//logPartyState("Leader Changed");
				return true;
			}

			std::optional<std::size_t> changeLeaderToFirstAvailable()
			{
				if (m_players.size() <= 1)
					return 0;

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

			void updatePlayersPerTeam(std::uint16_t playersPerTeam)
			{
				m_settings.playersPerTeam = playersPerTeam;
				m_partyInfo.maxPlayers = playersPerTeam;
				//logPartyState("Players Per Team Updated");
			}

			std::pair<std::uint16_t, std::uint16_t> getRoomId() const
			{
				return std::pair{ m_partyInfo.clanRoomId, m_partyInfo.clanRoomNumber };
			}

			const Main::Structures::ClanRoomSettings& getSettings() const noexcept
			{
				return m_settings;
			}

			bool isFull() const noexcept
			{
				return m_partyInfo.numPlayers >= m_partyInfo.maxPlayers;
			}

			std::size_t getPlayersSize() const noexcept
			{
				return m_players.size();
			}

			std::vector<Main::Structures::PartyPlayerInfo> getPlayers() const noexcept
			{
				std::vector<Main::Structures::PartyPlayerInfo> waitingPlayers;
				for (const auto& [playerInfo, session] : m_players)
				{
					waitingPlayers.push_back(playerInfo);
				}
				return waitingPlayers;
			}

			std::vector<std::shared_ptr<Main::Network::Session>> getPlayerSessions() noexcept
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

			const Main::Structures::PartyInfo& getClanMatchInfo() const noexcept
			{
				return m_partyInfo;
			}

			void broadcast(Common::Network::Packet& packet)
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

			void broadcastMessage(const std::string& message)
			{
				for (const auto& [u, sessionWeak] : m_players)
				{
					if (auto session = sessionWeak.lock())
					{
						session->sendMessage(message, Main::Enums::TIP);
					}
				}
			}

			bool isRegistered() const noexcept
			{
				return m_isRegistered;
			}

			void switchRegistered()
			{
				m_isRegistered = !m_isRegistered;
				//logPartyState("Registered Switched, Is Registered: " + std::to_string(m_isRegistered));
			}

			void broadcastExceptSelf(const Common::Network::Packet& packet, std::uint16_t sessionId)
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

			std::optional<Main::Structures::RegisteredClanInfo> getInfo() const
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

			void setTeam(Common::Enums::Team team)
			{
				m_team = team;
				//logPartyState("Team Set");
			}

			Common::Enums::Team getTeam() const noexcept
			{
				return m_team;
			}

			void storeStats(Main::Persistence::MainScheduler& scheduler, const Main::ClientData::ClientEndingMatchHeader& stats)
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
				//logPartyState("Stats Stored");
			}

		private:
			/********** DEBUG PURPOSES /**********/
			std::string getWaitingPlayersNicknames() const
			{
				if (m_players.empty())
					return "Waiting Players: None";

				std::string result = "Waiting Players: ";
				for (const auto& [playerInfo, weakSession] : m_players)
				{
					if (result != "Waiting Players: ")
						result += ", ";

					result += playerInfo.toString();

					if (auto session = weakSession.lock())
					{
						result += " (Room: " + std::to_string(session->getPlayer().getRoomNumber()) +
							", PartyRoom: " + std::to_string(session->getPlayer().getPartyRoomNumber()) + ")";
					}
					else
					{
						result += " (Session expired)";
					}
				}
				return result;
			}

			std::string getFormattedPartyInfo() const
			{
				return "Party Info: " + m_partyInfo.toString();
			}

			void sendDebugMessage()
			{
				broadcastMessage(getWaitingPlayersNicknames());
				broadcastMessage(getFormattedPartyInfo());
			}

			void logPartyState(const std::string& action)
			{
				std::cout << "\n========== " << action << " ==========\n";

				if (m_players.empty())
				{
					std::cout << "Players: (none)\n";
				}
				else
				{
					std::cout << "Players (first = Leader):\n";
					for (size_t i = 0; i < m_players.size(); ++i)
					{
						if (auto session = m_players[i].second.lock())
						{
							std::cout << "  ";
							if (i == 0) std::cout << "[LEADER] ";
							std::cout << session->getAccountInfo().nickname;
							std::cout << "\n";
						}
					}
				}

				std::cout << "\nParty Info:\n";
				std::cout << "  Room Number: " << m_partyInfo.clanRoomNumber << "\n";
				std::cout << "  Clan ID: " << m_partyInfo.clanRoomId << "\n";
				std::cout << "  Num Players: " << m_partyInfo.numPlayers << "\n";
				std::cout << "  Max Players: " << m_partyInfo.maxPlayers << "\n";
				std::cout << "  Leader: " << m_partyInfo.leaderName << " (Level " << m_partyInfo.leaderLevel << ")\n";
				std::cout << "  Match Started: " << (m_partyInfo.hasMatchStarted ? "Yes" : "No") << "\n";

				std::cout << "=========================================\n\n";
			}
			/******** DEBUG PURPOSES END /********/
		};
	}
}

#endif