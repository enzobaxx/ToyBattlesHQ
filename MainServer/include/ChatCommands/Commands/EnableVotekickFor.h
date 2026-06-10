#ifndef ENABLE_VOTEKICK_FOR_H
#define ENABLE_VOTEKICK_FOR_H

#include "ChatCommands/ICommand.h"
#include "ChatCommands/ChatCommands.h"
#include "Utils/Utils.h"
#include "MainServer.h"
#include <source_location>

namespace Main
{
	namespace Command
	{
		class DisableVotekickFor final : public ICommand
		{
		private:
			std::string m_targetPlayerName{};
			std::uint16_t m_durationDays{};

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
					return true;
				}
				return false;
			}


		public:
			explicit DisableVotekickFor(const Common::Enums::PlayerGrade requiredGrade)
				: ICommand{ requiredGrade, "/disablevotekickfor <nickname> <duration>(in days) <reason>",
				R"(^(\S+)\s+(\S+)\s+(\d+)\s+(.*)$)" }
			{
			}

			void execute(const std::string& command, std::shared_ptr<Main::Network::Session> session,
				MN::SessionsManager& sessionsManager, MC::RoomsManager&, MP::MainScheduler& scheduler, std::uint32_t, Main::MainServer&) override
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
					zoned_time zt{ "UTC",
						local_seconds{duration_cast<seconds>(system_clock::now().time_since_epoch()) + seconds(m_durationDays * 24 * 60 * 60)}
					};
					const std::string votekickDisabledUntil = std::format("{:%Y-%m-%d %H:%M:%S}", zt.get_sys_time());

					if (!scheduler.immediatePersist(
						std::source_location::current(),
						&Main::Persistence::PersistentDatabase::updateVotekickDisabledUntil,
						m_targetPlayerName,
						votekickDisabledUntil))
					{
						session->sendMessage("error: player not found");
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
					if (!targetSession->disableVotekick(m_durationDays))
					{
						session->sendMessage("error: unknown");
						return;
					}
				}
				session->sendMessage("success");
			}
		};

		REGISTER_CMD(DisableVotekickFor, Common::Enums::PlayerGrade::GRADE_MOD)



			class EnableVotekickFor final : public ICommand
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
			explicit EnableVotekickFor(const Common::Enums::PlayerGrade requiredGrade)
				: ICommand{ requiredGrade, "/enablevotekickfor <nickname>", R"(^\S+\s+(.*)$)" }
			{
			}

			void execute(const std::string& command, std::shared_ptr<Main::Network::Session> session,
				MN::SessionsManager& sessionsManager, MC::RoomsManager&, MP::MainScheduler& scheduler, std::uint32_t, Main::MainServer&) override
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
						&Main::Persistence::PersistentDatabase::resetVotekickDisabledUntil, m_targetPlayerName))
					{
						session->sendMessage("error: the player was not found in the database");
						return;
					}
				}
				else
				{
					if (!targetSession->enableVotekick())
					{
						session->sendMessage("error: unknown");
						return;
					}
				}
				session->sendMessage("success");
			}
		};
		REGISTER_CMD(EnableVotekickFor, Common::Enums::PlayerGrade::GRADE_MOD)

	};
}


#endif
