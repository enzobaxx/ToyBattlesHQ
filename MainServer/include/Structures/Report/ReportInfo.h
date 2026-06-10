#ifndef REPORT_INFO_H
#define REPORT_INFO_H

#include <cstdint>
#include <string>
#include <ctime>

namespace Main
{
	namespace Structures
	{
		struct ReportInfo
		{
			std::uint32_t reportId{};
			std::uint32_t reporterAccountId{};
			std::uint32_t reportedAccountId{};
			std::string reporterNickname{};
			std::string reportedNickname{};
			std::string reason{};
			std::uint32_t roomNumber{};
			std::time_t timestamp{};
			bool isAcknowledged{ false };
			std::string acknowledgedBy{};
			std::time_t acknowledgedTime{};

			ReportInfo() = default;

			ReportInfo(std::uint32_t id, std::uint32_t reporterId, std::uint32_t reportedId,
					  const std::string& reporterName, const std::string& reportedName,
					  const std::string& reportReason, std::uint32_t roomNum)
				: reportId(id)
				, reporterAccountId(reporterId)
				, reportedAccountId(reportedId)
				, reporterNickname(reporterName)
				, reportedNickname(reportedName)
				, reason(reportReason)
				, roomNumber(roomNum)
				, timestamp(std::time(nullptr))
			{
			}
		};
	}
}

#endif