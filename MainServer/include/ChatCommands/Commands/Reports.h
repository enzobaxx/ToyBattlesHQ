#ifndef REPORTS_COMPLEXCOMMAND_HEADER
#define REPORTS_COMPLEXCOMMAND_HEADER

#include "ChatCommands/ICommand.h"
#include "ChatCommands/ChatCommands.h"
#include "Utils/Utils.h"
#include "Utils/Constants.h"
#include "MainServer.h"
#include "Managers/ReportManager.h"
#include <source_location>

namespace Main
{
	namespace Command
	{
		class Reports final : public ICommand
		{
		public:
			Reports(const Common::Enums::PlayerGrade requiredGrade)
				: ICommand{ requiredGrade, "/reports - View all pending reports" }
			{
			}

			void execute(const std::string& command, std::shared_ptr<Main::Network::Session> session, MN::SessionsManager& sessionsManager,
				MC::RoomsManager& roomsManager,
				MP::MainScheduler& scheduler, std::uint32_t roomNumber, Main::MainServer& mainServer) override
			{
				START_BENCHMARK

				auto unacknowledgedReports = mainServer.getReportManager().getUnacknowledgedReports();
				auto totalReports = mainServer.getReportManager().getTotalReportsCount();
				auto unacknowledgedCount = mainServer.getReportManager().getUnacknowledgedReportsCount();

				if (unacknowledgedReports.empty())
				{
					session->sendMessage("No pending reports. Total reports in system: " + std::to_string(totalReports));
					END_BENCHMARK(Reports::execute, session)
					return;
				}

				session->sendMessage("=== PENDING REPORTS (" + std::to_string(unacknowledgedCount) + "/" + std::to_string(totalReports) + ") ===", Main::Enums::TIP);

				for (const auto& report : unacknowledgedReports)
				{
					std::string roomInfo = report.roomNumber > 0 ?
						"Room ID: " + std::to_string(report.roomNumber) :
						"Lobby";

					std::string timeAgo = "just now";
					std::time_t now = std::time(nullptr);
					std::time_t diff = now - report.timestamp;

					if (diff > 60)
					{
						std::time_t minutes = diff / 60;
						if (minutes > 60)
						{
							std::time_t hours = minutes / 60;
							timeAgo = std::to_string(hours) + "h ago";
						}
						else
						{
							timeAgo = std::to_string(minutes) + "m ago";
						}
					}

					session->sendMessage("[" + std::to_string(report.reportId) + "] " +
						report.reporterNickname + " -> " + report.reportedNickname +
						" (" + roomInfo + ") - " + timeAgo, Main::Enums::INFO);
					session->sendMessage("  Reason: " + report.reason, Main::Enums::INFO);
					session->sendMessage("  Use /rr " + std::to_string(report.reportId) +
						" to acknowledge this report", Main::Enums::TIP);
				}

				END_BENCHMARK(Reports::execute, session)
			}
		};

		REGISTER_CMD(Reports, Common::Enums::PlayerGrade::GRADE_MOD)
	}
}
#endif