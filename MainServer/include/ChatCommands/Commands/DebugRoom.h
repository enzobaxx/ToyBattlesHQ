#ifndef DEBUGROOM_COMMAND_HEADER
#define DEBUGROOM_COMMAND_HEADER


#include "ChatCommands/ICommand.h"
#include "Utils/Utils.h"
#include "ChatCommands/ChatCommands.h"
#include "MainServer.h"
#include "Rooms/Room.h"

namespace Main
{
	namespace Command
	{
		struct DebugRoom final : public ICommand
		{
			explicit DebugRoom(const Common::Enums::PlayerGrade requiredGrade)
				: ICommand{ requiredGrade, "/debugroom: shows players info of the current room" }
			{
			}

			void execute(const std::string&, std::shared_ptr<Main::Network::Session> session, MN::SessionsManager&, MC::RoomsManager& roomsManager, 
				MP::MainScheduler&, std::uint32_t roomNum,
				Main::MainServer&) override
			{
				if (!roomNum)
				{
					session->sendMessage("This command only works inside rooms");
					return;
				}
				else if (Main::Classes::Room* room = roomsManager.getRoomByNumber(roomNum))
				{
					auto allPlayers = room->getAllPlayersWithSessions();
					session->sendMessage("Basic Room Information:", Main::Enums::ChatExtra::TIP);
					auto roomInfo = room->getRoomInfoAsString();
					std::istringstream stream(roomInfo);
					std::string segment;
					while (stream >> segment) 
					{
						session->sendMessage("\t" + segment);
					}
					session->sendMessage("Players List:", Main::Enums::TIP);
					session->sendMessage("\tHost: " + std::string{ allPlayers[0].first.playerName });

					for (const auto& currentPlayer : allPlayers)
					{
						const std::string teamName = (currentPlayer.first.team == 1) ? "red" :
							(currentPlayer.first.team == 2) ? "blue" :
							(currentPlayer.first.team == 0) ? "all" :
							(currentPlayer.first.team == 4) ? "obs" : "unknown";

						session->sendMessage("\t- " + std::string{ currentPlayer.first.playerName } +
							" [team: " + teamName + "]" +
							" [" + std::to_string(currentPlayer.second->getPlayer().ping) + "ms]" +
							" [SEID: " + std::to_string(currentPlayer.second->getId()) + "]");
					}
				}
				else
				{
					session->sendMessage("Error: room not found");
				}
			}
		};
		REGISTER_CMD(DebugRoom, Common::Enums::PlayerGrade::GRADE_NORMAL)

		struct DebugParty final : public ICommand
		{
			explicit DebugParty(const Common::Enums::PlayerGrade requiredGrade)
				: ICommand{ requiredGrade, "/debugparty: shows info of the current party room" }
			{
			}

			void execute(const std::string&, std::shared_ptr<Main::Network::Session> session, MN::SessionsManager&, MC::RoomsManager& roomsManager,
				MP::MainScheduler&, std::uint32_t roomNum,
				Main::MainServer& srv) override
			{
				const auto& ainfo = session->getAccountInfo();
				auto partiesManager = srv.getPartiesManager();
				if (auto partyRoom = partiesManager.getExactRoomFor(ainfo.clanId, session->getPlayer().matchContext.partyRoomNumber))
				{
					session->sendMessage(partyRoom->getPlayersNicknames());
					session->sendMessage(partyRoom->getFormattedPartyInfo());
				}
				else
				{
					session->sendMessage("Error: Not in a party");
				}
			}
		};

		REGISTER_CMD(DebugParty, Common::Enums::PlayerGrade::GRADE_NORMAL)


		class PlayerInfo final : public ICommand
		{
		private:
			std::string m_targetPlayerName{};

			bool parseCommand(const std::string& providedCommand) override
			{
				std::smatch match;
				if (std::regex_match(providedCommand, match, this->m_pattern))
				{
					m_targetPlayerName = match[1].str();
					return true;
				}
				return false;
			}

		public:
			explicit PlayerInfo(const Common::Enums::PlayerGrade requiredGrade)
				: ICommand{ requiredGrade, "/playerinfo <nickname>",  R"(^\S+\s+(.*)$)" }
			{
			}

			void execute(const std::string& command, std::shared_ptr<Main::Network::Session> session, MN::SessionsManager& sm, MC::RoomsManager& roomsManager,
				MP::MainScheduler& scheduler, std::uint32_t,
				Main::MainServer&) override
			{
				if (!parseCommand(command))
				{
					session->sendMessage("parsing error");
					return;
				}

				auto targetSession = sm.findSessionByName(m_targetPlayerName.c_str());
				if (targetSession)
				{
					const auto& ainfo = targetSession->getAccountInfo();
					auto banInfoOpt = scheduler.immediatePersist(std::source_location::current(),
						&Main::Persistence::PersistentDatabase::getBanInfoByNickname,
						m_targetPlayerName);
					auto muteInfoOpt = scheduler.immediatePersist(std::source_location::current(),
						&Main::Persistence::PersistentDatabase::getMuteInfoByNickname,
						m_targetPlayerName);
					auto roomCreationDisabledUntilOpt = scheduler.immediatePersist(std::source_location::current(),
						&Main::Persistence::PersistentDatabase::getRoomCreationDisabledUntil,
						m_targetPlayerName);
					auto votekickDisabledUntilOpt = scheduler.immediatePersist(std::source_location::current(),
						&Main::Persistence::PersistentDatabase::getVotekickDisabledUntil,
						m_targetPlayerName);

					session->sendMessage("User Information: " + m_targetPlayerName, Main::Enums::TIP);
					session->sendMessage(" - Player Status: ONLINE");
					session->sendMessage(" - AccountID: " + std::to_string(ainfo.accountID));
					session->sendMessage(" - SessionID: " + std::to_string(targetSession->getId()));
					session->sendMessage(" - Grade: " + std::to_string(ainfo.playerGrade));
					session->sendMessage(" - Current room: " + std::to_string(targetSession->getPlayer().matchContext.roomNumber));
					session->sendMessage(" - Level: " + std::to_string(ainfo.playerLevel));
					session->sendMessage(" - MicroPoints: " + std::to_string(ainfo.microPoints));
					session->sendMessage(" - RockTotens: " + std::to_string(ainfo.rockTotens));

					if (banInfoOpt && banInfoOpt->isBanned)
					{
						session->sendMessage("This player is banned. More information: ", Main::Enums::TIP);
						session->sendMessage(" - Ban Reason: " + banInfoOpt->reason);
						session->sendMessage(" - Banned until (UTC): " + banInfoOpt->bannedUntil);
					}
					if (muteInfoOpt && muteInfoOpt->isMuted)
					{
						session->sendMessage("This player is muted. More information:", Main::Enums::TIP);
						session->sendMessage(" - Mute Reason: " + muteInfoOpt->reason);
						session->sendMessage(" - Muted By: " + muteInfoOpt->mutedBy);
						session->sendMessage(" - Mute Expiration (UTC): " + muteInfoOpt->mutedUntil);
					}
					if (roomCreationDisabledUntilOpt && *roomCreationDisabledUntilOpt != "0" && !roomCreationDisabledUntilOpt->empty())
					{
						session->sendMessage("This player cannot create rooms.", Main::Enums::TIP);
						session->sendMessage(" - Expiration (UTC): " + *roomCreationDisabledUntilOpt);
					}
					if (votekickDisabledUntilOpt && *votekickDisabledUntilOpt != "0" && !votekickDisabledUntilOpt->empty())
					{
						session->sendMessage("This player cannot start votekicks.", Main::Enums::TIP);
						session->sendMessage(" - Expiration (UTC): " + *votekickDisabledUntilOpt);
					}
				}
				else
				{ // offline
					auto ainfoOpt = scheduler.immediatePersist(std::source_location::current(), &Main::Persistence::PersistentDatabase::getPlayerInfoByNickname,
						m_targetPlayerName);
					auto banInfoOpt = scheduler.immediatePersist(std::source_location::current(), &Main::Persistence::PersistentDatabase::getBanInfoByNickname,
						m_targetPlayerName);
					auto muteInfoOpt = scheduler.immediatePersist(std::source_location::current(), &Main::Persistence::PersistentDatabase::getMuteInfoByNickname,
						m_targetPlayerName);
					auto roomCreationDisabledUntilOpt =
						scheduler.immediatePersist(std::source_location::current(), &Main::Persistence::PersistentDatabase::getRoomCreationDisabledUntil,
							m_targetPlayerName);
					auto votekickDisabledUntilOpt =
						scheduler.immediatePersist(std::source_location::current(), &Main::Persistence::PersistentDatabase::getVotekickDisabledUntil,
							m_targetPlayerName);

					if (ainfoOpt)
					{
						session->sendMessage("User Information: " + m_targetPlayerName, Main::Enums::TIP);
						session->sendMessage(" - Player Status: OFFLINE");
						session->sendMessage(" - Last Login: " + ainfoOpt->second);
						session->sendMessage(" - AccountID: " + std::to_string(ainfoOpt->first.accountID));
						session->sendMessage(" - Grade: " + std::to_string(ainfoOpt->first.playerGrade));
						session->sendMessage(" - Level: " + std::to_string(ainfoOpt->first.playerLevel));
						session->sendMessage(" - MicroPoints: " + std::to_string(ainfoOpt->first.microPoints));
						session->sendMessage(" - RockTotens: " + std::to_string(ainfoOpt->first.rockTotens));

						if (banInfoOpt && banInfoOpt->isBanned)
						{
							session->sendMessage("This player is banned. More information: ", Main::Enums::TIP);
							session->sendMessage(" - Ban Reason: " + banInfoOpt->reason);
							session->sendMessage(" - Banned until (UTC): " + banInfoOpt->bannedUntil);
						}
						if (muteInfoOpt && muteInfoOpt->isMuted)
						{
							session->sendMessage("This player is muted. More information:", Main::Enums::TIP);
							session->sendMessage(" - Mute Reason: " + muteInfoOpt->reason);
							session->sendMessage(" - Muted By: " + muteInfoOpt->mutedBy);
							session->sendMessage(" - Mute Expiration (UTC): " + muteInfoOpt->mutedUntil);
						}
						if (roomCreationDisabledUntilOpt && *roomCreationDisabledUntilOpt != "0" && !roomCreationDisabledUntilOpt->empty())
						{
							session->sendMessage("This player cannot create rooms.", Main::Enums::TIP);
							session->sendMessage(" - Expiration (UTC): " + *roomCreationDisabledUntilOpt);
						}
						if (votekickDisabledUntilOpt && *votekickDisabledUntilOpt != "0" && !votekickDisabledUntilOpt->empty())
						{
							session->sendMessage("This player cannot initiate votekicks.", Main::Enums::TIP);
							session->sendMessage(" - Expiration (UTC): " + *votekickDisabledUntilOpt);
						}
					}
					else
					{
						session->sendMessage("Player not found");
					}
				}
			}
		};

		REGISTER_CMD(PlayerInfo, Common::Enums::PlayerGrade::GRADE_MOD)
	}
}
#endif