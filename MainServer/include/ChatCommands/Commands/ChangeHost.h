#ifndef CHANGEHOST_COMMAND_HEADER
#define CHANGEHOST_COMMAND_HEADER

#include "ChatCommands/ICommand.h"
#include "ChatCommands/ChatCommands.h"
#include "Utils/Utils.h"
#include "MainServer.h"

namespace Main
{
	namespace Command
	{
		class ChangeHost final : public ICommand
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
			explicit ChangeHost(const Common::Enums::PlayerGrade requiredGrade)
				: ICommand{ requiredGrade, "/changehost <nickname>",  R"(^\S+\s+(.*)$)" }
			{
			}

			void execute(const std::string& command, std::shared_ptr<Main::Network::Session> session, MN::SessionsManager& sessionsManager, 
				MC::RoomsManager& roomsManager, MP::MainScheduler&, std::uint32_t,
				Main::MainServer&) override
			{
				if (!parseCommand(command))
				{
					session->sendMessage("parsing error");
					return;
				}

				if (session->getPlayer().getRoomNumber() >= Common::Constants::clanRoomNumberStart)
				{
					session->sendMessage("Error: changing host in clan war rooms is currently disabled!");
					return;
				}

				if (Main::Classes::Room* room = roomsManager.getRoomByNumber(session->getPlayer().getRoomNumber()))
				{
					const bool changed = room->changeHostByNickname(m_targetPlayerName);
					if (!changed)
					{
						session->sendMessage("Error while attempting to change the host");
					}
					else
					{
						session->sendMessage("success");
					}
				}
				else
				{
					session->sendMessage("Error: not in a room");
				}
			}
		};

		REGISTER_CMD(ChangeHost, Common::Enums::PlayerGrade::GRADE_MOD)
	}
}


#endif
