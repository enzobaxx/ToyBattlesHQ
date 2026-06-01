#ifndef COMMAND_SET_CURRENCY_FOR_HEADER
#define COMMAND_SET_CURRENCY_FOR_HEADER

#include "../ChatCommands.h"
#include "../ICommand.h"
#include "Utils/Utils.h"
#include "../../MainServer.h"
#include <source_location>

namespace Main
{
	namespace Command
	{
		class SetCurrencyFor final : public ICommand
		{
		private:
			std::string m_targetPlayerName{};
			std::uint32_t m_rockTotens{};
			std::uint32_t m_microPoints{};
			std::uint16_t m_coins{};

			static bool parseNumber(const std::string& str, auto& out)
			{
				auto [ptr, ec] = std::from_chars(str.data(), str.data() + str.size(), out);
				return ec == std::errc{} && ptr == str.data() + str.size();
			}

			bool parseCommand(const std::string& providedCommand) override
			{
				std::smatch match;
				if (std::regex_match(providedCommand, match, m_pattern))
				{
					m_targetPlayerName = match[1].str();
					if (m_targetPlayerName.empty())
						return false;
					return parseNumber(match[2].str(), m_rockTotens)
						&& parseNumber(match[3].str(), m_microPoints)
						&& parseNumber(match[4].str(), m_coins);
				}
				return false;
			}

		public:
			explicit SetCurrencyFor(const Common::Enums::PlayerGrade requiredGrade)
				: ICommand{ requiredGrade, "/setcurrencyfor <nickname> <rt> <mp> <coins>", R"(^\S+\s+(\S+)\s+(\d+)\s+(\d+)\s+(\d+))" }
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
						&Main::Persistence::PersistentDatabase::setCurrencyByName, m_targetPlayerName, m_rockTotens, m_microPoints, m_coins,
						static_cast<std::uint32_t>(session->getAccountInfo().playerGrade)))
					{
						session->sendMessage("error: player not found, has a greater grade, or a db error occurred");
						return;
					}
					session->sendMessage("success: set currency for offline player " + m_targetPlayerName + " (target should relog)");
					return;
				}

				const bool isSelf = targetSession->getAccountInfo().accountID == session->getAccountInfo().accountID;
				if (targetSession->getAccountInfo().playerGrade > session->getAccountInfo().playerGrade)
				{
					session->sendMessage("error: the target user has a greater grade than you");
					return;
				}

				if (!targetSession->setAccountRockTotens(m_rockTotens)
					|| !targetSession->setAccountMicroPoints(m_microPoints)
					|| !targetSession->setAccountCoins(m_coins))
				{
					session->sendMessage("error: one or more values are out of range (rt/mp too large, or coins > 127)");
					return;
				}
				targetSession->sendCurrency();

				session->sendMessage("success: set currency for " + m_targetPlayerName);
				if (!isSelf)
				{
					targetSession->sendMessage("A staff member updated your currency.");
				}
			}
		};

		REGISTER_CMD(SetCurrencyFor, Common::Enums::PlayerGrade::GRADE_TESTER)
	}
}

#endif
