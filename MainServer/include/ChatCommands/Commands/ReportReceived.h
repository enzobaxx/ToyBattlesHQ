#ifndef REPORTRECEIVED_COMPLEXCOMMAND_HEADER
#define REPORTRECEIVED_COMPLEXCOMMAND_HEADER

#include "ChatCommands/ICommand.h"
#include "ChatCommands/ChatCommands.h"
#include "Utils/Utils.h"
#include "Utils/Constants.h"
#include "MainServer.h"
#include "Managers/ReportManager.h"
#include "Structures/Mailbox/Mailbox.h"
#include <source_location>

namespace Main
{
	namespace Command
	{
		class ReportReceived final : public ICommand
		{
		private:
			std::uint32_t m_reportId{};

			bool parseCommand(const std::string& providedCommand) override
			{
				std::smatch match;
				if (std::regex_match(providedCommand, match, this->m_pattern))
				{
					const std::string& reportIdStr = match[2].str();
					auto [ptr, ec] = std::from_chars(reportIdStr.data(), reportIdStr.data() + reportIdStr.size(), m_reportId);
					if (ec != std::errc{} || ptr != reportIdStr.data() + reportIdStr.size())
					{
						return false;
					}
					return true;
				}
				return false;
			}

		public:
			ReportReceived(const Common::Enums::PlayerGrade requiredGrade)
				: ICommand{ requiredGrade, "/rr <report_id> - Acknowledge a report and thank the reporter"
				, R"(^(\S+)\s*(\d+)\s*(.*)$)" }
			{
			}

			void execute(const std::string& command, std::shared_ptr<Main::Network::Session> session, MN::SessionsManager& sessionsManager,
				MC::RoomsManager& roomsManager,
				MP::MainScheduler& scheduler, std::uint32_t roomNumber, Main::MainServer& mainServer) override
			{
				START_BENCHMARK
				if (!parseCommand(command))
				{
					session->sendMessage("error: parsing error, see /? for command usage. Usage: /rr <report_id>");
					return;
				}

				Main::Structures::ReportInfo report;
				if (!mainServer.getReportManager().getReportById(m_reportId, report))
				{
					session->sendMessage("error: report with ID " + std::to_string(m_reportId) + " not found");
					return;
				}

				if (report.isAcknowledged)
				{
					session->sendMessage("Report #" + std::to_string(m_reportId) + " was already acknowledged by " +
						report.acknowledgedBy + " at " + std::string(std::ctime(&report.acknowledgedTime)));
					return;
				}

				const auto& modInfo = session->getAccountInfo();

				auto reporterSession = sessionsManager.findSessionByName(report.reporterNickname.c_str());

				if (reporterSession)
				{
					std::string thankYouMessage = "Hello " + report.reporterNickname +
						", Thank you for reporting a player for breaking our rules. We will take appropriate action based on our findings.";

					reporterSession->sendMessage(thankYouMessage, Main::Enums::TIP);
					session->sendMessage("Thank you message sent to " + report.reporterNickname);
				}
				else
				{
					Main::Structures::Mailbox mailbox;
					mailbox.accountId = report.reporterAccountId;
					mailbox.timestamp = std::time(nullptr);
					mailbox.hasBeenRead = 0;
					std::strncpy(mailbox.nickname, modInfo.nickname, sizeof(mailbox.nickname) - 1);
					mailbox.nickname[sizeof(mailbox.nickname) - 1] = '\0';

					std::string mailboxMessage = "Hello " + report.reporterNickname +
						", Thank you for reporting a player for breaking our rules. We will take appropriate action based on our findings.";

					std::strncpy(mailbox.message, mailboxMessage.c_str(), sizeof(mailbox.message) - 1);
					mailbox.message[sizeof(mailbox.message) - 1] = '\0';

					session->sendOfflineMailbox(mailbox);
					session->sendMessage("Thank you message sent via mailbox to " + report.reporterNickname + " (offline)");
				}

				mainServer.getReportManager().acknowledgeReport(m_reportId, std::string(modInfo.nickname));

				Utils::Logger::log("REPORT #" + std::to_string(m_reportId) + " ACKNOWLEDGED: " + std::string(modInfo.nickname) +
					" acknowledged report from " + report.reporterNickname,
					Utils::LogType::Info, "ReportReceivedCommand");

				END_BENCHMARK(ReportReceived::execute, session)
			}
		};

		REGISTER_CMD(ReportReceived, Common::Enums::PlayerGrade::GRADE_MOD)
	}
}
#endif