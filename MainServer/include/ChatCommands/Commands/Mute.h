#ifndef MUTE_COMMAND_HEADER
#define MUTE_COMMAND_HEADER

#include "ChatCommands/ICommand.h"
#include "ChatCommands/ChatCommands.h"
#include "Utils/Utils.h"
#include "MainServer.h"
#include <source_location>

namespace Main
{
	namespace Command
	{
		class Mute final : public ICommand
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
			explicit Mute(const Common::Enums::PlayerGrade requiredGrade)
				: ICommand{ requiredGrade, "/mute <nickname> <duration>(in days) <reason>",
				R"(^(\S+)\s+(\S+)\s+(\d+)\s+(.*)$)" }
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
					using namespace std::chrono;
					using namespace std::literals;
					zoned_time zt{ "UTC", local_seconds{duration_cast<seconds>(system_clock::now().time_since_epoch()) + seconds(m_durationDays * 24 * 60 * 60)} };
					const std::string mutedUntil = std::format("{:%Y-%m-%d %H:%M:%S}", zt.get_sys_time());

					if (!scheduler.immediatePersist(std::source_location::current(),
						&Main::Persistence::PersistentDatabase::updateMute, m_targetPlayerName, mutedUntil, m_reason,
						session->getAccountInfo().nickname, session->getAccountInfo().playerGrade))
					{
						session->sendMessage("error: the player was either not found in the database, or they have equal or higher grade than you");
						return;
					}
				}
				else
				{
					if (targetSession->getAccountInfo().playerGrade >= session->getAccountInfo().playerGrade)
					{
						session->sendMessage("Error: The target user has equal or greater grade than you.");
						return;
					}
					if (!targetSession->muteAccount(m_durationDays, m_reason, session->getAccountInfo().nickname))
					{
						session->sendMessage("error: unknown");
						return;
					}
				}
				session->sendMessage("success");
			}
		};

		REGISTER_CMD(Mute, Common::Enums::PlayerGrade::GRADE_MOD)



		class Unmute final : public ICommand
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
			explicit Unmute(const Common::Enums::PlayerGrade requiredGrade)
				: ICommand{ requiredGrade, "/unmute <nickname>",  R"(^\S+\s+(.*)$)" }
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
					if (!scheduler.immediatePersist(std::source_location::current(), &Main::Persistence::PersistentDatabase::unmuteAccount, m_targetPlayerName))
					{
						session->sendMessage("error: the player was not found in the database");
						return;
					}
				}
				else
				{
					if (!targetSession->unmuteAccount())
					{
						session->sendMessage("error: unknown");
						return;
					}
				}
				session->sendMessage("success");
			}
		};
		REGISTER_CMD(Unmute, Common::Enums::PlayerGrade::GRADE_MOD)
	};
}


#endif
