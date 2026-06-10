#ifndef COMMAND_SET_NAME_FOR_HEADER
#define COMMAND_SET_NAME_FOR_HEADER

#include "../ChatCommands.h"
#include "../ICommand.h"
#include "Utils/Utils.h"
#include "../../MainServer.h"
#include <algorithm>
#include <cctype>
#include <source_location>

namespace Main
{
	namespace Command
	{
		class SetNameFor final : public ICommand
		{
		private:
			std::string m_targetNickname{};
			std::string m_newNickname{};

			bool parseCommand(const std::string& providedCommand) override
			{
				std::smatch match;
				if (std::regex_match(providedCommand, match, m_pattern))
				{
					m_targetNickname = match[1].str();
					m_newNickname = match[2].str();
					return !m_targetNickname.empty() && !m_newNickname.empty();
				}
				return false;
			}

			static bool isValidNewNickname(const std::string& nickname)
			{
				if (nickname.size() < 4 || nickname.size() > 16)
				{
					return false;
				}
				return std::ranges::all_of(nickname, [](char c) {
					return std::isalnum(static_cast<unsigned char>(c)) || c == '[' || c == ']';
				});
			}

		public:
			explicit SetNameFor(const Common::Enums::PlayerGrade requiredGrade)
				: ICommand{ requiredGrade, "/setnamefor <nickname> <newNickname>", R"(^\S+\s+(\S+)\s+(\S+))" }
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

				if (!isValidNewNickname(m_newNickname))
				{
					session->sendMessage("error: the new nickname must be 4-16 characters and contain only letters, numbers, '[' or ']'");
					return;
				}

				auto targetSession = sessionsManager.findSessionByName(m_targetNickname.c_str());
				if (targetSession && targetSession->getAccountInfo().playerGrade > session->getAccountInfo().playerGrade)
				{
					session->sendMessage("error: the target user has a greater grade than you");
					return;
				}

				if (!scheduler.immediatePersist(std::source_location::current(),
					&Main::Persistence::PersistentDatabase::updatePlayerNameByNickname, m_targetNickname, m_newNickname,
					static_cast<std::uint32_t>(session->getAccountInfo().playerGrade)))
				{
					session->sendMessage("error: player not found, nickname already taken, target has a greater grade, or a db error occurred");
					return;
				}

				if (targetSession)
				{
					targetSession->applyNicknameChange(m_newNickname);
					session->sendMessage("success: changed nickname of " + m_targetNickname + " to " + m_newNickname);
				}
				else
				{
					session->sendMessage("success: changed nickname for offline player " + m_targetNickname + " to " + m_newNickname + " (target should relog)");
				}
			}
		};

		REGISTER_CMD(SetNameFor, Common::Enums::PlayerGrade::GRADE_TESTER)
	}
}

#endif
