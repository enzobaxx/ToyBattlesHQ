#ifndef AUTHORIZATION_HANDLER_H
#define AUTHORIZATION_HANDLER_H

#include "Network/Sessions/MainSession.h"
#include "Structures/AccountInfo/MainAccountInfo.h"
#include "Network/Packet.h"
#include "Utils/Constants.h"
#include <optional>
#include <source_location>
#include <string>

#include "Detail/Utilities.h"
#include <Enums/PlayerEnums.h>

namespace Main
{
    namespace Handlers
    {
        
        inline bool securityLog(Main::Persistence::MainScheduler& scheduler, std::uint32_t accountGrade, const std::string& message, const std::string& severity)
        {
            return scheduler.immediatePersist(std::source_location::current(), &Main::Persistence::PersistentDatabase::logGameEvent,
                accountGrade >= 3 ? "MainAuthGraded" : "MainAuthUngraded", message, severity);
        }

        inline bool isWithinLastMinute(const std::string& timestampUtc)
        {
            std::istringstream iss(timestampUtc);
            std::chrono::utc_time<std::chrono::seconds> loggedTime;
            iss >> std::chrono::parse("%Y-%m-%d %H:%M:%S", loggedTime);
            if (iss.fail()) return false;
            auto now = std::chrono::floor<std::chrono::seconds>(std::chrono::utc_clock::now());
            auto diff = now - loggedTime;
            if (diff < std::chrono::seconds(0)) return false;
            return diff <= std::chrono::minutes(1);
        }

        inline std::optional<Main::Structures::AccountInfo> handleAuthorization(const Common::Network::Packet& request, std::shared_ptr<Main::Network::Session> session,
            std::size_t totalOnlinePlayers, bool isServerOffline, Main::Persistence::MainScheduler& scheduler, bool isPublic, Main::Classes::ReportManager& reportManager)
        {

            Common::Network::Packet response;
            response.setTcpHeader(request.getSession(), Common::Enums::NO_ENCRYPTION);
            response.setOrder(request.getOrder());
            response.setExtra(static_cast<std::uint8_t>(Main::Enums::AuthorizationExtra::SUCCESS));

            if (request.getDataSize() < sizeof(Main::ClientData::ClientAuthorization))
            {
                response.setExtra(static_cast<std::uint8_t>(Main::Enums::AuthorizationExtra::AUTHORIZATION_FAILED));
                session->asyncWrite(response);
                return std::nullopt;
            }

            const auto clientInfo = Main::Details::parseData<Main::ClientData::ClientAuthorization>(request);
            const auto& clientVersionRequired = Common::Utils::SetupParser::getInstance().getClientSetup();

            if (auto accountInfoOpt =
                scheduler.immediatePersist(std::source_location::current(), &Main::Persistence::PersistentDatabase::getPlayerInfo, clientInfo.accountID);
                accountInfoOpt)
            {
                // Reset AccountKey to 0 for security measures
                if (!scheduler.immediatePersist(std::source_location::current(), &Main::Persistence::PersistentDatabase::resetAccountKey, accountInfoOpt->accountID))
                {
                    securityLog(scheduler, accountInfoOpt->playerGrade, "Failed to reset AccountKey for account " + std::to_string(accountInfoOpt->accountID), "MEDIUM");
                    response.setExtra(static_cast<std::uint8_t>(Main::Enums::AuthorizationExtra::AUTHORIZATION_FAILED));
                    session->asyncWrite(response);
                    return std::nullopt;
                }

                const bool clientVersionMatches = clientInfo.clientVersion.matches(clientVersionRequired.version1, clientVersionRequired.version2, clientVersionRequired.version3);
                const bool serverUnavailable = totalOnlinePlayers >= Common::Constants::maxServerCapacity || isServerOffline;

                std::optional<std::string> lastLogged =
                    scheduler.immediatePersist(std::source_location::current(), &Main::Persistence::PersistentDatabase::getLastLogged, accountInfoOpt->accountID);
                if (!lastLogged || !isWithinLastMinute(*lastLogged))
                {
                    securityLog(scheduler, accountInfoOpt->playerGrade, "MainAuth fail due to LastLogged being too old " + std::to_string(accountInfoOpt->accountID), "MEDIUM");
                    response.setExtra(static_cast<std::uint8_t>(Main::Enums::AuthorizationExtra::AUTHORIZATION_FAILED));
                    session->asyncWrite(response);
                    return std::nullopt;
                }
                if ((serverUnavailable || !clientVersionMatches || !isPublic) && accountInfoOpt->playerGrade < Common::Enums::PlayerGrade::GRADE_ES)
                {
                    response.setExtra(static_cast<std::uint8_t>(Main::Enums::AuthorizationExtra::WRONG_CLIENT_VER_OR_SERVER_FULL_OR_OFFLINE));
                    session->asyncWrite(response);
                    return std::nullopt;
                }
                else if (accountInfoOpt->accountID != clientInfo.accountID || clientInfo.accountHash != accountInfoOpt->accountKey)
                {
                    if (accountInfoOpt->playerGrade >= Common::Enums::GRADE_MOD && Common::Utils::SetupParser::getInstance().getAuthSetup().enhancedSecurity)
                    {
                        if (!Common::Utils::sendEmailAlert("[SEVERE Alert] Graded AccountKey Mismatch", "Graded accountID: " + std::to_string(accountInfoOpt->accountID) +
                            " clicked on a channel, but the AccountKey was not correct (possible account exploit abuse - detected and stopped)"))
                        {
                            securityLog(scheduler, accountInfoOpt->playerGrade,
                                "Failed to send email alert after graded account had wrong AccountKey on channel selection" + std::to_string(accountInfoOpt->accountID), "HIGH");
                        }
                    }
                    securityLog(scheduler, accountInfoOpt->playerGrade, 
                        "AccountID or AccountHash doesn't match to the one in DB " + std::to_string(accountInfoOpt->accountID), "SEVERE");
                    response.setExtra(static_cast<std::uint8_t>(Main::Enums::AuthorizationExtra::AUTHORIZATION_FAILED));
                    session->asyncWrite(response);
                    return std::nullopt;
                }

                if (Common::Utils::SetupParser::getInstance().getAuthSetup().enhancedSecurity && accountInfoOpt->playerGrade >= Common::Enums::GRADE_MOD)
                {
                    auto gradedHwidOpt = scheduler.immediatePersist(std::source_location::current(),&Main::Persistence::PersistentDatabase::getGradedHwid,accountInfoOpt->accountID);
                    if (!gradedHwidOpt.has_value())
                    {
                        securityLog(scheduler,accountInfoOpt->playerGrade,"Failed to retrieve dbGradedHWID and dbGradedHwidSalt for accountID "
                            + std::to_string(accountInfoOpt->playerGrade),"LOW");

                        response.setExtra(static_cast<std::uint8_t>(Main::Enums::AuthorizationExtra::AUTHORIZATION_FAILED));
                        session->asyncWrite(response);
                        return std::nullopt;
                    }
                    std::tie(session->m_gradedHwid, session->m_gradedHwidSalt) = *gradedHwidOpt;
                }

                session->asyncWrite(response);
                session->sendMessage("Welcome! To see all commands, type /commands", Main::Enums::ChatExtra::INFO);

                Common::Enums::PlayerGrade playerGrade = static_cast<Common::Enums::PlayerGrade>(accountInfoOpt->playerGrade);
                if (playerGrade == Common::Enums::PlayerGrade::GRADE_MOD ||
                    playerGrade == Common::Enums::PlayerGrade::GRADE_TESTER ||
                    playerGrade == Common::Enums::PlayerGrade::GRADE_GM)
                {
                    std::size_t pendingReports = reportManager.getUnacknowledgedReportsCount();
                    if (pendingReports > 0)
                    {
                        session->sendMessage("You got " + std::to_string(pendingReports) + " pending reports /reports", Main::Enums::TIP);
                    }
                    else
                    {
                        session->sendMessage("No pending reports", Main::Enums::TIP);
                    }
                }

                session->sendMessage("Client Version: " + std::to_string(clientInfo.clientVersion.ver2) + "." + std::to_string(clientInfo.clientVersion.ver3) + "." +
                    std::to_string(clientInfo.clientVersion.ver4));

                return *accountInfoOpt;
            }

            return std::nullopt;
        }

        inline void handleHwid(const Common::Network::Packet& request, std::shared_ptr<Main::Network::Session> session)
        {
            if (session->getAccountInfo().playerGrade < Common::Enums::GRADE_MOD) return;

            char hwid[64]{};
            std::memcpy(hwid, request.getData(), std::min(request.getDataSize(), 64u));
            session->m_hwid = hwid;

            session->m_hwidLastUpdatedTimestamp = std::chrono::duration_cast<std::chrono::seconds>(std::chrono::system_clock::now().time_since_epoch()).count();
        }
    }
}

#endif

