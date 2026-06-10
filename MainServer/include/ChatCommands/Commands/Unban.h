#ifndef UNBAN_COMMAND_HEADER
#define UNBAN_COMMAND_HEADER

#include "ChatCommands/ICommand.h"
#include "ChatCommands/ChatCommands.h"
#include "Utils/Utils.h"
#include "MainServer.h"
#include <source_location>

namespace Main
{
	namespace Command
	{
		class Unban final : public ICommand
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
			explicit Unban(const Common::Enums::PlayerGrade requiredGrade)
				: ICommand{ requiredGrade, "/unban <nickname>",  R"(^\S+\s+(.*)$)" }
			{
			}

			void execute(const std::string& command, std::shared_ptr<Main::Network::Session> session, MN::SessionsManager&, MC::RoomsManager&, 
				MP::MainScheduler& scheduler, std::uint32_t,
				Main::MainServer&) override 
			{
				if (!parseCommand(command))
				{
					session->sendMessage("parsing error");
					return;
				}

				const bool unbanned = scheduler.immediatePersist(std::source_location::current(), &Main::Persistence::PersistentDatabase::unbanPlayer, m_targetPlayerName);
				if (!unbanned)
				{
					session->sendMessage("database error");
				}
				else
				{
					session->sendMessage("success");
				}
			}
		};

		REGISTER_CMD(Unban, Common::Enums::PlayerGrade::GRADE_MOD)
	};
}


#endif
