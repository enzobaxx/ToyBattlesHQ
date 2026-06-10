#ifndef REPORT_MANAGER_H
#define REPORT_MANAGER_H

#include "Structures/Report/ReportInfo.h"
#include <vector>
#include <mutex>
#include <memory>
#include <string>
#include <cstdint>

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

			std::uint32_t addReport(const Main::Structures::ReportInfo& report);
			std::vector<Main::Structures::ReportInfo> getAllReports();
			std::vector<Main::Structures::ReportInfo> getUnacknowledgedReports();
			bool acknowledgeReport(std::uint32_t reportId, const std::string& acknowledgedBy);
			bool getReportById(std::uint32_t reportId, Main::Structures::ReportInfo& outReport);
			std::size_t getTotalReportsCount();
			std::size_t getUnacknowledgedReportsCount();
			void cleanOldReports();
		};
	}
}

#endif
