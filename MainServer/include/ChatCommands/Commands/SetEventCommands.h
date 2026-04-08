#ifndef SET_EVENT_COMMANDS_HEADER
#define SET_EVENT_COMMANDS_HEADER


#include "../ICommand.h"
#include "../ChatCommands.h"
#include "../../MainServer.h"

namespace Main
{
	namespace Command
	{
		struct SetEventCommands final : public ICommand
		{
			std::uint32_t m_totalHours{};

			bool parseCommand(const std::string& providedCommand) override
			{
				std::smatch match;
				if (std::regex_match(providedCommand, match, m_pattern))
				{
					const std::string& matched_str = match[1].str();
					const std::from_chars_result result = std::from_chars(matched_str.data(), matched_str.data() + matched_str.size(), m_totalHours);
					if (result.ec == std::errc())
					{
						return true;
					}
				}
				return false;
			}

			explicit SetEventCommands(const Common::Enums::PlayerGrade requiredGrade)
				: ICommand{ requiredGrade, "/seteventcommands <hours>: makes special commands available to Event Supporters for the given time",
				 R"(^\S+\s(\d+))" }
			{
			}

			void execute(const std::string& command, std::shared_ptr<Main::Network::Session> session, MN::SessionsManager& sessionsManager, MC::RoomsManager&,
				MP::MainScheduler& scheduler, std::uint32_t,
				Main::MainServer& mainSv) override
			{
				if (!parseCommand(command))
				{
					session->sendMessage("parsing error");
					return;
				}
				if (scheduler.immediatePersist(std::source_location::current(), &Main::Persistence::PersistentDatabase::setCommandEventExpirationHours, m_totalHours))
				{
					session->sendMessage("success");
				}
				else
				{
					session->sendMessage("error: database issue");
				}
			}
		};

		REGISTER_CMD(SetEventCommands, Common::Enums::PlayerGrade::GRADE_TESTER)
	}
}

#endif