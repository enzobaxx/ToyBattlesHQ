#ifndef COMMAND_SET_LEVEL_FOR_HEADER
#define COMMAND_SET_LEVEL_FOR_HEADER

#include "../ChatCommands.h"
#include "../ICommand.h"
#include "Utils/Utils.h"
#include "../../MainServer.h"
#include <source_location>

namespace Main
{
	namespace Command
	{
		class SetLevelFor final : public ICommand
		{
		private:
			std::string m_targetPlayerName{};
			std::uint16_t m_level{};

			bool parseCommand(const std::string& providedCommand) override
			{
				std::smatch match;
				if (std::regex_match(providedCommand, match, m_pattern))
				{
					m_targetPlayerName = match[1].str();
					const std::string& levelStr = match[2].str();
					auto [ptr, ec] = std::from_chars(levelStr.data(), levelStr.data() + levelStr.size(), m_level);
					return ec == std::errc{} && ptr == levelStr.data() + levelStr.size() && !m_targetPlayerName.empty();
				}
				return false;
			}

		public:
			explicit SetLevelFor(const Common::Enums::PlayerGrade requiredGrade)
				: ICommand{ requiredGrade, "/setlevelfor <nickname> <level>", R"(^\S+\s+(\S+)\s+(\d+))" }
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

				const auto* gradeInfo = Common::ConstantDatabase::CdbSingleton<Common::ConstantDatabase::CdbGradeInfo>::getInstance().getEntry(m_level + 1);
				if (!gradeInfo)
				{
					session->sendMessage("error: could not retrieve experience for that level from gradeinfo.cdb (valid range [0, 105])");
					return;
				}

				auto targetSession = sessionsManager.findSessionByName(m_targetPlayerName.c_str());
				if (!targetSession)
				{
					if (!scheduler.immediatePersist(std::source_location::current(),
						&Main::Persistence::PersistentDatabase::setPlayerLevelByName, m_targetPlayerName, m_level, gradeInfo->gi_exp,
						static_cast<std::uint32_t>(session->getAccountInfo().playerGrade)))
					{
						session->sendMessage("error: player not found, has a greater grade, or a db error occurred");
						return;
					}
					session->sendMessage("success: set level " + std::to_string(m_level) + " for offline player " + m_targetPlayerName);
					return;
				}

				const bool isSelf = targetSession->getAccountInfo().accountID == session->getAccountInfo().accountID;
				if (targetSession->getAccountInfo().playerGrade > session->getAccountInfo().playerGrade)
				{
					session->sendMessage("error: the target user has a greater grade than you");
					return;
				}

				if (!targetSession->setLevel(m_level))
				{
					session->sendMessage("error: wrong level (must be in the range [0, 105])");
					return;
				}
				if (!targetSession->setExperience(gradeInfo->gi_exp))
				{
					session->sendMessage("error while updating the experience for the chosen level");
					return;
				}

				session->sendMessage("success: set level " + std::to_string(m_level) + " for " + m_targetPlayerName + " (target should relog)");
				if (!isSelf)
				{
					targetSession->sendMessage("A staff member set your level to " + std::to_string(m_level) + ". Please relog.");
				}
			}
		};

		REGISTER_CMD(SetLevelFor, Common::Enums::PlayerGrade::GRADE_TESTER)
	}
}

#endif
