#ifndef COMMAND_SEND_RT_HEADER
#define COMMAND_SEND_RT_HEADER

#include "ChatCommands/ChatCommands.h"
#include "ChatCommands/ICommand.h"
#include "Utils/Utils.h"
#include "MainServer.h"
#include <charconv>
#include <source_location>

namespace Main
{
	namespace Command
	{
		class SendRt final : public ICommand
		{
		private:
			std::string m_targetPlayerName{};
			std::uint32_t m_amount{};

			bool parseCommand(const std::string& providedCommand) override
			{
				std::smatch match;
				if (std::regex_match(providedCommand, match, m_pattern))
				{
					m_targetPlayerName = match[1].str();
					if (m_targetPlayerName.empty())
					{
						return false;
					}
					const std::string& amountStr = match[2].str();
					auto [ptr, ec] = std::from_chars(amountStr.data(), amountStr.data() + amountStr.size(), m_amount);
					return ec == std::errc{} && ptr == amountStr.data() + amountStr.size();
				}
				return false;
			}

		public:
			explicit SendRt(const Common::Enums::PlayerGrade requiredGrade)
				: ICommand{ requiredGrade, "/sendrt <nickname> <amount>", R"(^\S+\s+(\S+)\s+(\d+))" }
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
				if (!targetSession)
				{
					if (!scheduler.immediatePersist(std::source_location::current(),
						&Main::Persistence::PersistentDatabase::addRockTotensByName, m_targetPlayerName, m_amount,
						static_cast<std::uint32_t>(session->getAccountInfo().playerGrade)))
					{
						session->sendMessage("error: player not found, has a greater grade, or a db error occurred");
						return;
					}
					session->sendMessage("success: sent " + std::to_string(m_amount) + " RT to offline player " + m_targetPlayerName + " (target should relog)");
					return;
				}

				const bool isSelf = targetSession->getAccountInfo().accountID == session->getAccountInfo().accountID;
				if (targetSession->getAccountInfo().playerGrade > session->getAccountInfo().playerGrade)
				{
					session->sendMessage("error: the target user has a greater grade than you");
					return;
				}

				targetSession->sendRt(m_amount);

				session->sendMessage("success: sent " + std::to_string(m_amount) + " RT to " + m_targetPlayerName);
				if (!isSelf)
				{
					targetSession->sendMessage("A staff member sent you " + std::to_string(m_amount) + " RT.");
				}
			}
		};

		REGISTER_CMD(SendRt, Common::Enums::PlayerGrade::GRADE_TESTER)
	}
}

#endif
