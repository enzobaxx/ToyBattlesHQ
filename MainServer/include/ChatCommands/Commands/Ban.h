#ifndef BAN_COMMAND_HEADER
#define BAN_COMMAND_HEADER

#include "ChatCommands/ICommand.h"
#include "ChatCommands/ChatCommands.h"
#include "Utils/Utils.h"
#include "MainServer.h"
#include <source_location>

namespace Main
{
	namespace Command
	{
		class Ban final : public ICommand
		{
		private:
			std::string m_targetPlayerName{};
			std::uint16_t m_durationDays{};
			std::string m_reason{};

			bool parseCommand(const std::string& providedCommand) override
			{
				std::smatch match;
				if (std::regex_match(providedCommand, match, this->m_pattern))
				{
					m_targetPlayerName = match[2].str(); 
					const std::string& durationStr = match[3].str();
					auto [ptr, ec] = std::from_chars(durationStr.data(), durationStr.data() + durationStr.size(), m_durationDays);
					if (ec != std::errc{} || ptr != durationStr.data() + durationStr.size())
					{
						return false;
					}
					m_reason = match[4].str();
					return true;
				}
				return false; 
			}

		public:
			explicit Ban(const Common::Enums::PlayerGrade requiredGrade)
				: ICommand{ requiredGrade, "/ban <nickname> <duration>(in days) <reason>", 
				R"(^(\S+)\s*(\S+)\s*(\d+)\s*(.*)$)" }
			{
			}

			void execute(const std::string& command, std::shared_ptr<Main::Network::Session> session, MN::SessionsManager& sessionsManager, 
				MC::RoomsManager&, MP::MainScheduler& scheduler, std::uint32_t,
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
					using namespace std::chrono;
					using namespace std::literals;
					zoned_time zt{ "UTC", local_seconds{duration_cast<seconds>(system_clock::now().time_since_epoch()) + seconds(m_durationDays * 24 * 60 * 60)} };
					const std::string bannedUntil = std::format("{:%Y-%m-%d %H:%M:%S}", zt.get_sys_time());

					if (!scheduler.immediatePersist(std::source_location::current(),
						&Main::Persistence::PersistentDatabase::updateSuspension, m_targetPlayerName, bannedUntil, m_reason,
						static_cast<std::uint32_t>(session->getAccountInfo().playerGrade)))
					{
						session->sendMessage("error: the player was either not found in the database, or they have equal or higher grade than you");
						return;
					}

				}
				else
				{
					if (targetSession->getAccountInfo().playerGrade >= session->getAccountInfo().playerGrade)
					{
						session->sendMessage("error: The target user has equal or greater grade than you.");
						return;
					}
					sessionsManager.removeSession(targetSession->getId());
					if (!targetSession->banAccount(this->m_durationDays, this->m_reason))
					{
						session->sendMessage("error: unknown error");
						return;
					}
				}
				session->sendMessage("success");
			}
		};

		REGISTER_CMD(Ban, Common::Enums::PlayerGrade::GRADE_MOD)
	};
}


#endif
