#ifndef REPORT_MANAGER_H
#define REPORT_MANAGER_H

#include "Structures/Report/ReportInfo.h"
#include <vector>
#include <mutex>
#include <memory>

namespace Main
{
	namespace Classes
	{
		class ReportManager
		{
		private:
			std::vector<Main::Structures::ReportInfo> m_reports;
			std::uint32_t m_nextReportId{ 1 };
			std::mutex m_mutex;

		public:
			ReportManager() = default;

			std::uint32_t addReport(const Main::Structures::ReportInfo& report)
			{
				std::lock_guard<std::mutex> lock(m_mutex);
				auto newReport = report;
				newReport.reportId = m_nextReportId++;
				newReport.timestamp = std::time(nullptr);
				m_reports.push_back(newReport);
				return newReport.reportId;
			}

			std::vector<Main::Structures::ReportInfo> getAllReports()
			{
				std::lock_guard<std::mutex> lock(m_mutex);
				return m_reports;
			}

			std::vector<Main::Structures::ReportInfo> getUnacknowledgedReports()
			{
				std::lock_guard<std::mutex> lock(m_mutex);
				std::vector<Main::Structures::ReportInfo> unacknowledged;
				for (const auto& report : m_reports)
				{
					if (!report.isAcknowledged)
					{
						unacknowledged.push_back(report);
					}
				}
				return unacknowledged;
			}

			bool acknowledgeReport(std::uint32_t reportId, const std::string& acknowledgedBy)
			{
				std::lock_guard<std::mutex> lock(m_mutex);
				for (auto& report : m_reports)
				{
					if (report.reportId == reportId)
					{
						report.isAcknowledged = true;
						report.acknowledgedBy = acknowledgedBy;
						report.acknowledgedTime = std::time(nullptr);
						return true;
					}
				}
				return false;
			}

			bool getReportById(std::uint32_t reportId, Main::Structures::ReportInfo& outReport)
			{
				std::lock_guard<std::mutex> lock(m_mutex);
				for (const auto& report : m_reports)
				{
					if (report.reportId == reportId)
					{
						outReport = report;
						return true;
					}
				}
				return false;
			}

			std::size_t getTotalReportsCount()
			{
				std::lock_guard<std::mutex> lock(m_mutex);
				return m_reports.size();
			}

			std::size_t getUnacknowledgedReportsCount()
			{
				std::lock_guard<std::mutex> lock(m_mutex);
				std::size_t count = 0;
				for (const auto& report : m_reports)
				{
					if (!report.isAcknowledged)
					{
						count++;
					}
				}
				return count;
			}

			void cleanOldReports()
			{
				std::lock_guard<std::mutex> lock(m_mutex);
				std::time_t weekAgo = std::time(nullptr) - (7 * 24 * 60 * 60);
				m_reports.erase(
					std::remove_if(m_reports.begin(), m_reports.end(),
						[weekAgo](const Main::Structures::ReportInfo& report) {
							return report.timestamp < weekAgo;
						}),
					m_reports.end());
			}
		};
	}
}

#endif