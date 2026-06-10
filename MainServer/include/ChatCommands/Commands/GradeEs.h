#ifndef COMMAND_GRADE_ES_HEADER
#define COMMAND_GRADE_ES_HEADER

#include "../ChatCommands.h"
#include "../ICommand.h"
#include "Utils/Utils.h"
#include "../../MainServer.h"
#include <source_location>

namespace Main
{
	namespace Command
	{
		class GradeEs final : public ICommand
		{
		private:
			std::string m_targetPlayerName{};

			bool parseCommand(const std::string& providedCommand) override
			{
				std::smatch match;
				if (std::regex_match(providedCommand, match, m_pattern))
				{
					m_targetPlayerName = match[1].str();
					return !m_targetPlayerName.empty();
				}
				return false;
			}

		public:
			explicit GradeEs(const Common::Enums::PlayerGrade requiredGrade)
				: ICommand{ requiredGrade, "/gradees <nickname>", R"(^\S+\s+(\S+))" }
			{
			}

			void execute(const std::string& command, std::shared_ptr<Main::Network::Session> session, MN::SessionsManager& sessionsManager, MC::RoomsManager&,
				MP::MainScheduler& scheduler, std::uint32_t,
				Main::MainServer&) override
			{
				if (!parseCommand(command))
				{
					session->sendMessage("parsing error");
					return;
				}

				auto targetSession = sessionsManager.findSessionByName(m_targetPlayerName.c_str());
				if (targetSession && targetSession->getAccountInfo().playerGrade > session->getAccountInfo().playerGrade)
				{
					session->sendMessage("error: the target user has a greater grade than you");
					return;
				}

				if (!scheduler.immediatePersist(std::source_location::current(),
					&Main::Persistence::PersistentDatabase::setGradeByName, m_targetPlayerName,
					static_cast<std::uint32_t>(Common::Enums::GRADE_ES),
					static_cast<std::uint32_t>(session->getAccountInfo().playerGrade)))
				{
					session->sendMessage("error: player not found, has a greater grade, or a db error occurred");
					return;
				}

				if (targetSession)
				{
					Main::Structures::AccountInfo updatedInfo = targetSession->getAccountInfo();
					updatedInfo.playerGrade = Common::Enums::GRADE_ES;
					targetSession->setAccountInfo(updatedInfo);
					session->sendMessage("success: set grade 2 (ES) for " + m_targetPlayerName);
				}
				else
				{
					session->sendMessage("success: set grade 2 (ES) for offline player " + m_targetPlayerName);
				}
			}
		};

		REGISTER_CMD(GradeEs, Common::Enums::PlayerGrade::GRADE_TESTER)
	}
}

#endif
