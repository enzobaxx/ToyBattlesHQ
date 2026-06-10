#ifndef COMMAND_SET_LEVEL_HEADER
#define COMMAND_SET_LEVEL_HEADER

#include "ChatCommands/ChatCommands.h"
#include "ChatCommands/ICommand.h"
#include "Utils/Utils.h"
#include "MainServer.h"

namespace Main
{
	namespace Command
	{
		class SetLevel final : public ICommand
		{
		private:
			std::uint32_t m_level{};

			bool parseCommand(const std::string& providedCommand) override
			{
				std::smatch match;
				if (std::regex_match(providedCommand, match, m_pattern))
				{
					const std::string& matched_str = match[1].str(); 
					const std::from_chars_result result = std::from_chars(matched_str.data(), matched_str.data() + matched_str.size(), m_level);
					if (result.ec == std::errc()) 
					{
						return true;
					}
				}
				return false;
			}

		public:
			explicit SetLevel(const Common::Enums::PlayerGrade requiredGrade)
				: ICommand{ requiredGrade, "/setlevel <level>", R"(^\S+\s(\d+))" }
			{
			}

			void execute(const std::string& command, std::shared_ptr<Main::Network::Session> session, MN::SessionsManager&, MC::RoomsManager&, MP::MainScheduler&, 
				std::uint32_t,
				Main::MainServer&) override 
			{
				if (!parseCommand(command))
				{
					session->sendMessage("parsing error");
					return;
				}
				if (!session->setLevel(m_level))
				{
					session->sendMessage("error: wrong level (must be in the range [0, 105])");
					return;
				}
				
				if (auto* gradeInfo = Common::ConstantDatabase::CdbSingleton<Common::ConstantDatabase::CdbGradeInfo>::getInstance().getEntry(m_level + 1))
				{
					if (!session->setExperience(gradeInfo->gi_exp))
					{
						session->sendMessage("error while updating the experience for the chosen level");
						return;
					}
					else
					{
						session->sendMessage("New Experience set to " + std::to_string(gradeInfo->gi_exp));
					}
				}
				else
				{
					session->sendMessage("error while retrieving experience info from gradeinfo.cdb");
					return;
				}

				session->sendMessage("success (relog)");
			}
		};

		REGISTER_CMD(SetLevel, Common::Enums::PlayerGrade::GRADE_TESTER)
	}
}


#endif