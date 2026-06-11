#ifndef KICK_COMMAND_HEADER
#define KICK_COMMAND_HEADER

#include "ChatCommands/ICommand.h"
#include "ChatCommands/ChatCommands.h"
#include "Utils/Utils.h"
#include "MainServer.h"
#include "Rooms/Room.h"

namespace Main
{
	namespace Command
	{
		class Kick final : public ICommand
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
			explicit Kick(const Common::Enums::PlayerGrade requiredGrade)
				: ICommand{ requiredGrade, "/kick <nickname>",  R"(^\S+\s+(.*)$)" }
			{
			}

			virtual Common::Enums::PlayerGrade getRequiredGrade(Main::Persistence::MainScheduler& scheduler) const override
			{
				if (scheduler.immediatePersist(std::source_location::current(), &Main::Persistence::PersistentDatabase::isCommandEventExpired))
				{
					return m_requiredGrade;
				}
				return Common::Enums::PlayerGrade::GRADE_ES;
			}

			void execute(const std::string& command, std::shared_ptr<Main::Network::Session> session, MN::SessionsManager&, MC::RoomsManager& roomsManager, 
				MP::MainScheduler&, std::uint32_t,
				Main::MainServer&) override 
			{
				if (!parseCommand(command))
				{
					session->sendMessage("parsing error");
					return;
				}

				if (session->getPlayer().matchContext.roomNumber >= Common::Constants::clanRoomNumberStart)
				{
					session->sendMessage("Error: kicking someone from clan war rooms is currently disabled!");
					return;
				}

				if (Main::Classes::Room* room = roomsManager.getRoomByNumber(session->getPlayer().matchContext.roomNumber))
				{
					if (!room->kickPlayer(m_targetPlayerName.c_str()))
					{
						session->sendMessage("error: the player is either not in the room or they're >= MOD grade.");
					}
					else
					{
						session->sendMessage("success");
					}
				}
				else
				{
					session->sendMessage("error: you must be in a room to kick someone");
					return;
				}
			}
		};

		REGISTER_CMD(Kick, Common::Enums::PlayerGrade::GRADE_MOD)


		class Unkick final : public ICommand
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
			explicit Unkick(const Common::Enums::PlayerGrade requiredGrade)
				: ICommand{ requiredGrade, "/unkick <nickname>",  R"(^\S+\s+(.*)$)" }
			{
			}

			virtual Common::Enums::PlayerGrade getRequiredGrade(Main::Persistence::MainScheduler& scheduler) const override
			{
				if (scheduler.immediatePersist(std::source_location::current(), &Main::Persistence::PersistentDatabase::isCommandEventExpired))
				{
					return m_requiredGrade;
				}
				return Common::Enums::PlayerGrade::GRADE_ES;
			}

			void execute(const std::string& command, std::shared_ptr<Main::Network::Session> session, MN::SessionsManager&, MC::RoomsManager& roomsManager,
				MP::MainScheduler&, std::uint32_t,
				Main::MainServer&) override
			{
				if (!parseCommand(command))
				{
					session->sendMessage("parsing error");
					return;
				}

				if (Main::Classes::Room* room = roomsManager.getRoomByNumber(session->getPlayer().matchContext.roomNumber))
				{
					if (!room->removeKickedPlayerByNickname(m_targetPlayerName.c_str()))
					{
						session->sendMessage("error: this player was not kicked before.");
					}
					else
					{
						session->sendMessage("success");
					}
				}
				else
				{
					session->sendMessage("error: you must be in a room to kick someone");
					return;
				}
			}
		};

		REGISTER_CMD(Unkick, Common::Enums::PlayerGrade::GRADE_MOD)


		struct GetKickedPlayers final : public ICommand
		{
			explicit GetKickedPlayers(const Common::Enums::PlayerGrade requiredGrade)
				: ICommand{ requiredGrade, "/getkickedplayers" }
			{
			}

			void execute(const std::string& command, std::shared_ptr<Main::Network::Session> session, MN::SessionsManager&, MC::RoomsManager& roomsManager, 
				MP::MainScheduler&, std::uint32_t,
				Main::MainServer&) override
			{
				if (Main::Classes::Room* room = roomsManager.getRoomByNumber(session->getPlayer().matchContext.roomNumber))
				{
					auto kickedPlayers = room->getKickedPlayerNicknames();
					if (kickedPlayers.empty())
					{
						session->sendMessage("No kicked players found!");
						return;
					}
					session->sendMessage("Kicked Players List: ", Main::Enums::TIP);
					for (const auto& name : kickedPlayers)
					{
						session->sendMessage(" - " + name);
					}
				}
				else
				{
					session->sendMessage("error: you must be in a room to kick someone");
					return;
				}
			}
		};

		REGISTER_CMD(GetKickedPlayers, Common::Enums::PlayerGrade::GRADE_MOD)
	};
}


#endif
