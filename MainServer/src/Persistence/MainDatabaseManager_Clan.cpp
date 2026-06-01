#include <memory>
#include <string>

#include "../../include/Structures/AccountInfo/MainAccountInfo.h"
#include "../../include/Structures/Item/MainItem.h"
#include "../../include/Structures/Item/MainBoughtItem.h"
#include "../../include/Structures/Item/MainEquippedItem.h"
#include "../../include/Structures/MainEventsList.h"
#include "../../include/Structures/PlayerLists/Friend.h"
#include "../../include/Structures/PlayerLists/BlockedPlayer.h"
#include "../../include/Structures/Mailbox.h"
#include "../../include/Structures/AccountInfo/MuteInfo.h"
#include "../../include/Persistence/TransactionGuard.h"
#include "../../include/Persistence/MainDatabaseManager.h"
#include "../../include/MainEnums.h"
#include "Utils/Constants.h"
#include <mariadb/conncpp.hpp>
#include <mariadb/conncpp/Driver.hpp>
#include <mariadb/conncpp/Connection.hpp>
#include "Utils/SetupParser.h"
#include <cstring> 
#include <Utils.h>
#include <Macros.h>
#include <regex>

namespace Main
{
    namespace Persistence
    {


        bool PersistentDatabase::addClan(std::uint32_t leaderAid, const std::string& clanName, int clanFrontIcon, int clanBackIcon)
        {
            try
            {
                auto isValid = [](const std::string& str) {
                    return std::regex_match(str, std::regex(R"(^[A-Za-z0-9_ ]+$)"));
                    };

                if (!isValid(clanName))
                {
                    return false;
                }

                TransactionGuard guard(m_transactionalCon.get());

                std::unique_ptr<sql::PreparedStatement> insertStmt(m_transactionalCon->prepareStatement(
                        "INSERT INTO Clans (LeaderAid, Clanname, ClanFrontIcon, ClanBackIcon) VALUES (?, ?, ?, ?)"));

                insertStmt->setUInt(1, leaderAid);
                insertStmt->setString(2, clanName);
                insertStmt->setInt(3, clanFrontIcon);
                insertStmt->setInt(4, clanBackIcon);

                insertStmt->executeUpdate();

                std::unique_ptr<sql::ResultSet> res(m_transactionalCon->createStatement()->executeQuery("SELECT LAST_INSERT_ID() AS ClanId"));

                if (!res->next())
                {
                    return false;
                }

                std::uint32_t newClanId = res->getUInt("ClanId");
                std::unique_ptr<sql::PreparedStatement> updateUserStmt(m_transactionalCon->prepareStatement("UPDATE Users SET ClanID = ? WHERE AccountID = ?"));

                updateUserStmt->setUInt(1, newClanId);
                updateUserStmt->setUInt(2, leaderAid);
                updateUserStmt->executeUpdate();

                guard.commit();
                return true;
            }
            catch (const sql::SQLException& e)
            {
                ::Utils::Logger::log("MariaDB exception: " + std::string(e.what()), Utils::LogType::Error, "PersistentDatabase::addClan");
                return false;
            }
        }

        Main::Enums::ClanEnlist PersistentDatabase::enlistToClan(std::uint32_t accountId, const std::string& clanName)
        {
            try
            {
                TransactionGuard guard(m_transactionalCon.get());

                std::unique_ptr<sql::PreparedStatement> findClanStmt(m_transactionalCon->prepareStatement("SELECT ClanId FROM Clans WHERE Clanname = ?"));

                findClanStmt->setString(1, clanName);
                std::unique_ptr<sql::ResultSet> clanResult(findClanStmt->executeQuery());

                if (!clanResult->next())
                {
                    return Main::Enums::ClanEnlist::CLAN_NOT_FOUND;
                }

                std::uint32_t clanId = clanResult->getUInt("ClanId");
                std::unique_ptr<sql::PreparedStatement> checkUserStmt(m_transactionalCon->prepareStatement("SELECT AccountID, ClanID FROM Users WHERE AccountID = ?"));

                checkUserStmt->setUInt(1, accountId);
                std::unique_ptr<sql::ResultSet> userResult(checkUserStmt->executeQuery());

                if (!userResult->next())
                {
                    return Main::Enums::ClanEnlist::USER_NOT_FOUND;
                }

                std::uint32_t currentClanId = userResult->getUInt("ClanID");
                if (currentClanId)
                {
                    return Main::Enums::ClanEnlist::USER_ALREADY_IN_CLAN;
                }

                std::unique_ptr<sql::PreparedStatement> countMembersStmt(m_transactionalCon->prepareStatement("SELECT COUNT(*) AS MemberCount FROM Users WHERE ClanID = ?"));

                countMembersStmt->setUInt(1, clanId);
                std::unique_ptr<sql::ResultSet> countResult(countMembersStmt->executeQuery());

                countResult->next();
                int memberCount = countResult->getInt("MemberCount");

                if (memberCount >= 30)
                {
                    return Main::Enums::ClanEnlist::CLAN_FULL;
                }

                std::unique_ptr<sql::PreparedStatement> insertRequestStmt(m_transactionalCon->prepareStatement(
                    "INSERT INTO PendingClanRequests (AccountID, ClanID, RequestTime) VALUES (?, ?, NOW())"));

                insertRequestStmt->setUInt(1, accountId);
                insertRequestStmt->setUInt(2, clanId);
                insertRequestStmt->executeUpdate();

                guard.commit();

                return Main::Enums::ClanEnlist::CLAN_ENLIST_SUCCESS;
            }
            catch (const sql::SQLException& e)
            {
                ::Utils::Logger::log("MariaDB exception: " + std::string(e.what()), Utils::LogType::Error, "PersistentDatabase::enlistToClan");
                return Main::Enums::ClanEnlist::CLAN_ENLIST_DB_ERROR;
            }
        }

        std::pair<Main::Enums::GetPendingRequestsResult, std::vector<std::string>> PersistentDatabase::getPendingRequests(std::uint32_t accountId)
        {
            std::vector<std::string> pendingNicknames;

            try
            {
                std::unique_ptr<sql::PreparedStatement> checkUserClanStmt(m_con->prepareStatement("SELECT ClanID FROM Users WHERE AccountID = ?"));

                checkUserClanStmt->setUInt(1, accountId);
                std::unique_ptr<sql::ResultSet> userResult(checkUserClanStmt->executeQuery());

                if (!userResult->next())
                {
                    return { Main::Enums::GetPendingRequestsResult::NOT_IN_CLAN, {} };
                }

                std::uint32_t userClanId = userResult->getUInt("ClanID");
                if (userClanId == 0) 
                {
                    return { Main::Enums::GetPendingRequestsResult::NOT_IN_CLAN, {} };
                }

                std::unique_ptr<sql::PreparedStatement> checkLeaderStmt(m_con->prepareStatement("SELECT LeaderAid FROM Clans WHERE ClanId = ?"));

                checkLeaderStmt->setUInt(1, userClanId);
                std::unique_ptr<sql::ResultSet> leaderResult(checkLeaderStmt->executeQuery());

                if (!leaderResult->next())
                {
                    return { Main::Enums::GetPendingRequestsResult::DB_ERROR, {} }; 
                }

                std::uint32_t leaderAid = leaderResult->getUInt("LeaderAid");
                if (leaderAid != accountId)
                {
                    return { Main::Enums::GetPendingRequestsResult::NOT_LEADER, {} };
                }

                std::unique_ptr<sql::PreparedStatement> pendingStmt(m_con->prepareStatement("SELECT u.Nickname FROM PendingClanRequests pcr "
                    "JOIN Users u ON pcr.AccountID = u.AccountID WHERE pcr.ClanID = ? ORDER BY pcr.RequestTime DESC"));

                pendingStmt->setUInt(1, userClanId);
                std::unique_ptr<sql::ResultSet> pendingResult(pendingStmt->executeQuery());

                while (pendingResult->next())
                {
                    pendingNicknames.push_back(pendingResult->getString("Nickname").c_str());
                }

                if (pendingNicknames.empty())
                {
                    return { Main::Enums::GetPendingRequestsResult::NO_PENDING_REQUESTS, {} };
                }

                return { Main::Enums::GetPendingRequestsResult::SUCCESS, std::move(pendingNicknames) };
            }
            catch (const sql::SQLException& e)
            {
                ::Utils::Logger::log("MariaDB exception in getPendingRequests: " + std::string(e.what()), Utils::LogType::Error, "PersistentDatabase::getPendingRequests");
                return { Main::Enums::GetPendingRequestsResult::DB_ERROR, {} };
            }
        }

        Main::Enums::AcceptClanRequestResult PersistentDatabase::acceptClanRequest(std::uint32_t ownerAccountId, const std::string& targetNickname)
        {
            try
            {
                TransactionGuard guard(m_transactionalCon.get());

                std::unique_ptr<sql::PreparedStatement> ownerStmt(m_transactionalCon->prepareStatement("SELECT u.ClanID, c.LeaderAid "
                    "FROM Users u JOIN Clans c ON u.ClanID = c.ClanId WHERE u.AccountID = ?"));

                ownerStmt->setUInt(1, ownerAccountId);
                std::unique_ptr<sql::ResultSet> ownerResult(ownerStmt->executeQuery());

                if (!ownerResult->next())
                {
                    return Main::Enums::AcceptClanRequestResult::NOT_IN_CLAN;
                }

                std::uint32_t clanId = ownerResult->getUInt("ClanID");
                std::uint32_t leaderAid = ownerResult->getUInt("LeaderAid");

                if (clanId == 0)
                {
                    return Main::Enums::AcceptClanRequestResult::NOT_IN_CLAN;
                }

                if (leaderAid != ownerAccountId)
                {
                    return Main::Enums::AcceptClanRequestResult::NOT_LEADER;
                }

                std::unique_ptr<sql::PreparedStatement> targetStmt(m_transactionalCon->prepareStatement("SELECT AccountID, ClanID FROM Users WHERE Nickname = ?"));

                targetStmt->setString(1, targetNickname);
                std::unique_ptr<sql::ResultSet> targetResult(targetStmt->executeQuery());

                if (!targetResult->next())
                {
                    return Main::Enums::AcceptClanRequestResult::TARGET_NOT_FOUND;
                }

                std::uint32_t targetAccountId = targetResult->getUInt("AccountID");
                std::uint32_t targetCurrentClanId = targetResult->getUInt("ClanID");

                if (targetCurrentClanId != 0)
                {
                    return Main::Enums::AcceptClanRequestResult::TARGET_ALREADY_IN_CLAN;
                }

                std::unique_ptr<sql::PreparedStatement> checkRequestStmt(m_transactionalCon->prepareStatement(
                    "SELECT RequestTime FROM PendingClanRequests WHERE AccountID = ? AND ClanID = ?"));

                checkRequestStmt->setUInt(1, targetAccountId);
                checkRequestStmt->setUInt(2, clanId);
                std::unique_ptr<sql::ResultSet> requestResult(checkRequestStmt->executeQuery());

                if (!requestResult->next())
                {
                    return Main::Enums::AcceptClanRequestResult::TARGET_NOT_IN_REQUESTS;
                }

                std::unique_ptr<sql::PreparedStatement> countMembersStmt(m_transactionalCon->prepareStatement(
                    "SELECT COUNT(*) AS MemberCount FROM Users WHERE ClanID = ?"));

                countMembersStmt->setUInt(1, clanId);
                std::unique_ptr<sql::ResultSet> countResult(countMembersStmt->executeQuery());

                countResult->next();
                int memberCount = countResult->getInt("MemberCount");

                if (memberCount >= 30)
                {
                    return Main::Enums::AcceptClanRequestResult::CLAN_FULL;
                }

                std::unique_ptr<sql::PreparedStatement> updateTargetStmt(m_transactionalCon->prepareStatement(
                    "UPDATE Users SET ClanID = ? WHERE AccountID = ?"));

                updateTargetStmt->setUInt(1, clanId);
                updateTargetStmt->setUInt(2, targetAccountId);
                updateTargetStmt->executeUpdate();

                std::unique_ptr<sql::PreparedStatement> deleteRequestsStmt(m_transactionalCon->prepareStatement(
                    "DELETE FROM PendingClanRequests WHERE AccountID = ?"));

                deleteRequestsStmt->setUInt(1, targetAccountId);
                deleteRequestsStmt->executeUpdate();

                guard.commit();

                return Main::Enums::AcceptClanRequestResult::SUCCESS;
            }
            catch (const sql::SQLException& e)
            {
                ::Utils::Logger::log("MariaDB exception in acceptClanRequest: " + std::string(e.what()), Utils::LogType::Error, "PersistentDatabase::acceptClanRequest");
                return Main::Enums::AcceptClanRequestResult::DB_ERROR;
            }
        }

        Main::Enums::DenyClanRequestResult PersistentDatabase::denyClanRequest(std::uint32_t ownerAccountId, const std::string& targetNickname)
        {
            try
            {
                TransactionGuard guard(m_transactionalCon.get());

                std::unique_ptr<sql::PreparedStatement> ownerStmt(m_transactionalCon->prepareStatement(
                    "SELECT u.ClanID, c.LeaderAid FROM Users u JOIN Clans c ON u.ClanID = c.ClanId WHERE u.AccountID = ?"));

                ownerStmt->setUInt(1, ownerAccountId);
                std::unique_ptr<sql::ResultSet> ownerResult(ownerStmt->executeQuery());

                if (!ownerResult->next())
                {
                    return Main::Enums::DenyClanRequestResult::NOT_IN_CLAN;
                }

                std::uint32_t clanId = ownerResult->getUInt("ClanID");
                std::uint32_t leaderAid = ownerResult->getUInt("LeaderAid");

                if (clanId == 0)
                {
                    return Main::Enums::DenyClanRequestResult::NOT_IN_CLAN;
                }

                if (leaderAid != ownerAccountId)
                {
                    return Main::Enums::DenyClanRequestResult::NOT_LEADER;
                }

                std::unique_ptr<sql::PreparedStatement> targetStmt(m_transactionalCon->prepareStatement(
                    "SELECT AccountID FROM Users WHERE Nickname = ?"));

                targetStmt->setString(1, targetNickname);
                std::unique_ptr<sql::ResultSet> targetResult(targetStmt->executeQuery());

                if (!targetResult->next())
                {
                    return Main::Enums::DenyClanRequestResult::TARGET_NOT_FOUND;
                }

                std::uint32_t targetAccountId = targetResult->getUInt("AccountID");

                std::unique_ptr<sql::PreparedStatement> checkRequestStmt(m_transactionalCon->prepareStatement(
                    "SELECT RequestTime FROM PendingClanRequests WHERE AccountID = ? AND ClanID = ?"));

                checkRequestStmt->setUInt(1, targetAccountId);
                checkRequestStmt->setUInt(2, clanId);
                std::unique_ptr<sql::ResultSet> requestResult(checkRequestStmt->executeQuery());

                if (!requestResult->next())
                {
                    return Main::Enums::DenyClanRequestResult::TARGET_NOT_IN_REQUESTS;
                }

                std::unique_ptr<sql::PreparedStatement> deleteRequestStmt(m_transactionalCon->prepareStatement(
                    "DELETE FROM PendingClanRequests WHERE AccountID = ? AND ClanID = ?"));

                deleteRequestStmt->setUInt(1, targetAccountId);
                deleteRequestStmt->setUInt(2, clanId);
                deleteRequestStmt->executeUpdate();

                guard.commit();

                return Main::Enums::DenyClanRequestResult::SUCCESS;
            }
            catch (const sql::SQLException& e)
            {
                ::Utils::Logger::log("MariaDB exception in denyClanRequest: " + std::string(e.what()),
                    Utils::LogType::Error, "PersistentDatabase::denyClanRequest");
                return Main::Enums::DenyClanRequestResult::DB_ERROR;
            }
        }

        Main::Enums::KickClanMemberResult PersistentDatabase::kickClanMember(std::uint32_t ownerAccountId, const std::string& targetNickname)
        {
            try
            {
                TransactionGuard guard(m_transactionalCon.get());

                std::unique_ptr<sql::PreparedStatement> ownerStmt(m_transactionalCon->prepareStatement(
                    "SELECT u.ClanID, c.LeaderAid FROM Users u JOIN Clans c ON u.ClanID = c.ClanId WHERE u.AccountID = ?"));

                ownerStmt->setUInt(1, ownerAccountId);
                std::unique_ptr<sql::ResultSet> ownerResult(ownerStmt->executeQuery());

                if (!ownerResult->next())
                {
                    return Main::Enums::KickClanMemberResult::NOT_IN_CLAN;
                }

                std::uint32_t clanId = ownerResult->getUInt("ClanID");
                std::uint32_t leaderAid = ownerResult->getUInt("LeaderAid");

                if (clanId == 0)
                {
                    return Main::Enums::KickClanMemberResult::NOT_IN_CLAN;
                }

                if (leaderAid != ownerAccountId)
                {
                    return Main::Enums::KickClanMemberResult::NOT_LEADER;
                }

                std::unique_ptr<sql::PreparedStatement> targetStmt(m_transactionalCon->prepareStatement(
                    "SELECT AccountID, ClanID FROM Users WHERE Nickname = ?"));

                targetStmt->setString(1, targetNickname);
                std::unique_ptr<sql::ResultSet> targetResult(targetStmt->executeQuery());

                if (!targetResult->next())
                {
                    return Main::Enums::KickClanMemberResult::TARGET_NOT_FOUND;
                }

                std::uint32_t targetAccountId = targetResult->getUInt("AccountID");
                std::uint32_t targetClanId = targetResult->getUInt("ClanID");

                if (targetAccountId == ownerAccountId)
                {
                    return Main::Enums::KickClanMemberResult::CANNOT_KICK_SELF;
                }

                if (targetClanId != clanId)
                {
                    return Main::Enums::KickClanMemberResult::TARGET_NOT_IN_CLAN;
                }

                std::unique_ptr<sql::PreparedStatement> resetTargetStmt(m_transactionalCon->prepareStatement(
                    "UPDATE Users SET "
                    "ClanID = 0, "
                    "ClanContribution = 0, "
                    "ClanKills = 0, "
                    "ClanDeaths = 0, "
                    "ClanAssists = 0, "
                    "ClanWins = 0, "
                    "ClanLoses = 0, "
                    "ClanDraws = 0 "
                    "WHERE AccountID = ?"));

                resetTargetStmt->setUInt(1, targetAccountId);
                resetTargetStmt->executeUpdate();

                guard.commit();
                return Main::Enums::KickClanMemberResult::SUCCESS;
            }
            catch (const sql::SQLException& e)
            {
                ::Utils::Logger::log("MariaDB exception in kickClanMember: " + std::string(e.what()),
                    Utils::LogType::Error, "PersistentDatabase::kickClanMember");
                return Main::Enums::KickClanMemberResult::DB_ERROR;
            }
        }

        std::vector<std::string> PersistentDatabase::getClanMembers(std::uint32_t clanId)
        {
            std::vector<std::string> members;

            try
            {
                std::unique_ptr<sql::PreparedStatement> stmt(m_con->prepareStatement(
                    "SELECT Nickname FROM Users WHERE ClanID = ?"));

                stmt->setUInt(1, clanId);
                std::unique_ptr<sql::ResultSet> result(stmt->executeQuery());

                while (result->next())
                {
                    members.push_back(result->getString("Nickname").c_str());
                }

                return members;
            }
            catch (const sql::SQLException& e)
            {
                ::Utils::Logger::log("MariaDB exception in getClanMembers: " + std::string(e.what()),
                    Utils::LogType::Error, "PersistentDatabase::getClanMembers");
                return {};
            }
        }

        Main::Enums::LeaveClanResult PersistentDatabase::leaveClan(std::uint32_t accountId)
        {
            try
            {
                TransactionGuard guard(m_transactionalCon.get());

                std::unique_ptr<sql::PreparedStatement> userStmt(m_transactionalCon->prepareStatement(
                    "SELECT u.ClanID, c.LeaderAid FROM Users u LEFT JOIN Clans c ON u.ClanID = c.ClanId WHERE u.AccountID = ?"));

                userStmt->setUInt(1, accountId);
                std::unique_ptr<sql::ResultSet> userResult(userStmt->executeQuery());

                if (!userResult->next())
                {
                    return Main::Enums::LeaveClanResult::NOT_IN_CLAN;
                }

                std::uint32_t clanId = userResult->getUInt("ClanID");

                if (clanId == 0)
                {
                    return Main::Enums::LeaveClanResult::NOT_IN_CLAN;
                }

                std::uint32_t leaderAid = userResult->getUInt("LeaderAid");
                if (leaderAid == accountId)
                {
                    return Main::Enums::LeaveClanResult::IS_LEADER;
                }

                std::unique_ptr<sql::PreparedStatement> resetUserStmt(m_transactionalCon->prepareStatement(
                    "UPDATE Users SET "
                    "ClanID = 0, "
                    "ClanContribution = 0, "
                    "ClanKills = 0, "
                    "ClanDeaths = 0, "
                    "ClanAssists = 0, "
                    "ClanWins = 0, "
                    "ClanLoses = 0, "
                    "ClanDraws = 0 "
                    "WHERE AccountID = ?"));

                resetUserStmt->setUInt(1, accountId);
                resetUserStmt->executeUpdate();

                std::unique_ptr<sql::PreparedStatement> deleteRequestsStmt(m_transactionalCon->prepareStatement(
                    "DELETE FROM PendingClanRequests WHERE AccountID = ?"));

                deleteRequestsStmt->setUInt(1, accountId);
                deleteRequestsStmt->executeUpdate();

                guard.commit();
                return Main::Enums::LeaveClanResult::SUCCESS;
            }
            catch (const sql::SQLException& e)
            {
                ::Utils::Logger::log("MariaDB exception in leaveClan: " + std::string(e.what()),
                    Utils::LogType::Error, "PersistentDatabase::leaveClan");
                return Main::Enums::LeaveClanResult::DB_ERROR;
            }
        }

        Main::Enums::DisbandClanResult PersistentDatabase::disbandClan(std::uint32_t ownerAccountId)
        {
            try
            {
                TransactionGuard guard(m_transactionalCon.get());

                std::unique_ptr<sql::PreparedStatement> ownerStmt(m_transactionalCon->prepareStatement(
                    "SELECT u.ClanID, c.LeaderAid FROM Users u JOIN Clans c ON u.ClanID = c.ClanId WHERE u.AccountID = ?"));

                ownerStmt->setUInt(1, ownerAccountId);
                std::unique_ptr<sql::ResultSet> ownerResult(ownerStmt->executeQuery());

                if (!ownerResult->next())
                {
                    return Main::Enums::DisbandClanResult::NOT_IN_CLAN;
                }

                std::uint32_t clanId = ownerResult->getUInt("ClanID");
                std::uint32_t leaderAid = ownerResult->getUInt("LeaderAid");

                if (clanId == 0)
                {
                    return Main::Enums::DisbandClanResult::NOT_IN_CLAN;
                }

                if (leaderAid != ownerAccountId)
                {
                    return Main::Enums::DisbandClanResult::NOT_LEADER;
                }

                std::unique_ptr<sql::PreparedStatement> removeMembersStmt(m_transactionalCon->prepareStatement(
                    "UPDATE Users SET "
                    "ClanID = 0, "
                    "ClanContribution = 0, "
                    "ClanKills = 0, "
                    "ClanDeaths = 0, "
                    "ClanAssists = 0, "
                    "ClanWins = 0, "
                    "ClanLoses = 0, "
                    "ClanDraws = 0 "
                    "WHERE ClanID = ?"));

                removeMembersStmt->setUInt(1, clanId);
                int membersAffected = removeMembersStmt->executeUpdate();

                std::unique_ptr<sql::PreparedStatement> deleteRequestsStmt(m_transactionalCon->prepareStatement(
                    "DELETE FROM PendingClanRequests WHERE ClanID = ?"));

                deleteRequestsStmt->setUInt(1, clanId);
                deleteRequestsStmt->executeUpdate();

                std::unique_ptr<sql::PreparedStatement> deleteClanStmt(m_transactionalCon->prepareStatement(
                    "DELETE FROM Clans WHERE ClanId = ?"));

                deleteClanStmt->setUInt(1, clanId);
                deleteClanStmt->executeUpdate();

                guard.commit();
                return Main::Enums::DisbandClanResult::SUCCESS;
            }
            catch (const sql::SQLException& e)
            {
                ::Utils::Logger::log("MariaDB exception in disbandClan: " + std::string(e.what()),
                    Utils::LogType::Error, "PersistentDatabase::disbandClan");
                return Main::Enums::DisbandClanResult::DB_ERROR;
            }
        }

        Main::Enums::TransferOwnershipResult PersistentDatabase::transferOwnership(std::uint32_t ownerAccountId, const std::string& targetNickname)
        {
            try
            {
                TransactionGuard guard(m_transactionalCon.get());

                std::unique_ptr<sql::PreparedStatement> ownerStmt(m_transactionalCon->prepareStatement(
                    "SELECT u.ClanID, c.LeaderAid FROM Users u JOIN Clans c ON u.ClanID = c.ClanId WHERE u.AccountID = ?"));

                ownerStmt->setUInt(1, ownerAccountId);
                std::unique_ptr<sql::ResultSet> ownerResult(ownerStmt->executeQuery());

                if (!ownerResult->next())
                {
                    return Main::Enums::TransferOwnershipResult::NOT_IN_CLAN;
                }

                std::uint32_t clanId = ownerResult->getUInt("ClanID");
                std::uint32_t leaderAid = ownerResult->getUInt("LeaderAid");

                if (clanId == 0)
                {
                    return Main::Enums::TransferOwnershipResult::NOT_IN_CLAN;
                }

                if (leaderAid != ownerAccountId)
                {
                    return Main::Enums::TransferOwnershipResult::NOT_LEADER;
                }

                std::unique_ptr<sql::PreparedStatement> targetStmt(m_transactionalCon->prepareStatement(
                    "SELECT AccountID, ClanID FROM Users WHERE Nickname = ?"));

                targetStmt->setString(1, targetNickname);
                std::unique_ptr<sql::ResultSet> targetResult(targetStmt->executeQuery());

                if (!targetResult->next())
                {
                    return Main::Enums::TransferOwnershipResult::TARGET_NOT_FOUND;
                }

                std::uint32_t targetAccountId = targetResult->getUInt("AccountID");
                std::uint32_t targetClanId = targetResult->getUInt("ClanID");

                if (targetAccountId == ownerAccountId)
                {
                    return Main::Enums::TransferOwnershipResult::TARGET_IS_SELF;
                }

                if (targetClanId != clanId)
                {
                    return Main::Enums::TransferOwnershipResult::TARGET_NOT_IN_CLAN;
                }

                std::unique_ptr<sql::PreparedStatement> updateLeaderStmt(m_transactionalCon->prepareStatement(
                    "UPDATE Clans SET LeaderAid = ? WHERE ClanId = ?"));

                updateLeaderStmt->setUInt(1, targetAccountId);
                updateLeaderStmt->setUInt(2, clanId);
                updateLeaderStmt->executeUpdate();

                guard.commit();
                return Main::Enums::TransferOwnershipResult::SUCCESS;
            }
            catch (const sql::SQLException& e)
            {
                ::Utils::Logger::log("MariaDB exception in transferOwnership: " + std::string(e.what()),
                    Utils::LogType::Error, "PersistentDatabase::transferOwnership");
                return Main::Enums::TransferOwnershipResult::DB_ERROR;
            }
        }

        Main::Enums::UpdateClanIconResult PersistentDatabase::updateClanIcons(std::uint32_t ownerAccountId, std::optional<std::uint16_t> newFrontIcon,
            std::optional<std::uint16_t> newBackIcon)
        {
            try
            {
                if (!newFrontIcon.has_value() && !newBackIcon.has_value())
                {
                    return Main::Enums::UpdateClanIconResult::DB_ERROR; 
                }

                if (newFrontIcon.has_value())
                {
                    std::uint16_t frontIcon = newFrontIcon.value();
                    if (frontIcon < 1 || frontIcon > 295)
                    {
                        return Main::Enums::UpdateClanIconResult::INVALID_FRONT_ICON;
                    }
                }

                if (newBackIcon.has_value())
                {
                    std::uint16_t backIcon = newBackIcon.value();
                    if (backIcon < 1 || backIcon > 169)
                    {
                        return Main::Enums::UpdateClanIconResult::INVALID_BACK_ICON;
                    }
                }

                TransactionGuard guard(m_transactionalCon.get());

                std::unique_ptr<sql::PreparedStatement> ownerStmt(m_transactionalCon->prepareStatement(
                    "SELECT u.ClanID, c.LeaderAid "
                    "FROM Users u "
                    "JOIN Clans c ON u.ClanID = c.ClanId "
                    "WHERE u.AccountID = ?"));

                ownerStmt->setUInt(1, ownerAccountId);
                std::unique_ptr<sql::ResultSet> ownerResult(ownerStmt->executeQuery());

                if (!ownerResult->next())
                {
                    return Main::Enums::UpdateClanIconResult::NOT_IN_CLAN;
                }

                std::uint32_t clanId = ownerResult->getUInt("ClanID");
                std::uint32_t leaderAid = ownerResult->getUInt("LeaderAid");

                if (clanId == 0)
                {
                    return Main::Enums::UpdateClanIconResult::NOT_IN_CLAN;
                }

                if (leaderAid != ownerAccountId)
                {
                    return Main::Enums::UpdateClanIconResult::NOT_LEADER;
                }

                std::string query = "UPDATE Clans SET ";
                std::vector<std::string> setClauses;
                std::vector<std::pair<std::string, std::uint16_t>> parameters;

                if (newFrontIcon.has_value())
                {
                    setClauses.push_back("ClanFrontIcon = ?");
                    parameters.emplace_back("front", newFrontIcon.value());
                }
                if (newBackIcon.has_value())
                {
                    setClauses.push_back("ClanBackIcon = ?");
                    parameters.emplace_back("back", newBackIcon.value());
                }

                for (size_t i = 0; i < setClauses.size(); ++i)
                {
                    query += setClauses[i];
                    if (i < setClauses.size() - 1)
                    {
                        query += ", ";
                    }
                }

                query += " WHERE ClanId = ?";

                std::unique_ptr<sql::PreparedStatement> updateStmt(m_transactionalCon->prepareStatement(query));

                int paramIndex = 1;
                if (newFrontIcon.has_value())
                {
                    updateStmt->setInt(paramIndex++, newFrontIcon.value());
                }
                if (newBackIcon.has_value())
                {
                    updateStmt->setInt(paramIndex++, newBackIcon.value());
                }
                updateStmt->setUInt(paramIndex, clanId);
                updateStmt->executeUpdate();
                guard.commit();

                return Main::Enums::UpdateClanIconResult::SUCCESS;
            }
            catch (const sql::SQLException& e)
            {
                ::Utils::Logger::log("MariaDB exception in updateClanIcons: " + std::string(e.what()), Utils::LogType::Error, "PersistentDatabase::updateClanIcons");
                return Main::Enums::UpdateClanIconResult::DB_ERROR;
            }
        }

        Main::Enums::UpdateClanIconResult PersistentDatabase::updateClanFrontIcon(std::uint32_t ownerAccountId, std::uint16_t newFrontIcon)
        {
            return updateClanIcons(ownerAccountId, newFrontIcon, std::nullopt);
        }

        Main::Enums::UpdateClanIconResult PersistentDatabase::updateClanBackIcon(std::uint32_t ownerAccountId, std::uint16_t newBackIcon)
        {
            return updateClanIcons(ownerAccountId, std::nullopt, newBackIcon);
        }

        bool PersistentDatabase::clanExists(const std::string& clanName)
        {
            try
            {
                std::unique_ptr<sql::PreparedStatement> stmt(m_con->prepareStatement("SELECT 1 FROM Clans WHERE ClanName = ? LIMIT 1"));
                stmt->setString(1, clanName);
                std::unique_ptr<sql::ResultSet> res(stmt->executeQuery());
                return res->next();
            }
            catch (const sql::SQLException& e)
            {
                ::Utils::Logger::log("MariaDB exception: " + std::string(e.what()), Utils::LogType::Error, "PersistentDatabase::doesClanExist");
                return true; // on purpose
            }
        }

        bool PersistentDatabase::isUserInClan(std::uint32_t accountID)
        {
            try
            {
                std::unique_ptr<sql::PreparedStatement> stmt(m_con->prepareStatement("SELECT ClanID FROM Users WHERE AccountID = ? LIMIT 1"));
                stmt->setUInt(1, accountID);
                std::unique_ptr<sql::ResultSet> res(stmt->executeQuery());
                if (res->next())
                {
                    std::uint32_t clanID = res->getUInt("ClanID");
                    return clanID > 8;
                }
                return true; // on purpose block
            }
            catch (const sql::SQLException& e)
            {
                ::Utils::Logger::log("MariaDB exception: " + std::string(e.what()), Utils::LogType::Error, "PersistentDatabase::isUserInClan");
                return true; // on purpose block
            }
        }

        void PersistentDatabase::updateClanContribution(std::uint32_t clanId, std::uint32_t newContribution)
        {
            try
            {
                std::string updateQueryStr =
                    "UPDATE Clans SET TotalContribution = TotalContribution + ?  WHERE ClanId = ?";

                std::unique_ptr<sql::PreparedStatement> stmt(m_con->prepareStatement(updateQueryStr));
                stmt->setUInt(1, newContribution);
                stmt->setUInt(2, clanId);

                if (stmt->executeUpdate() == 0)
                {
                    ::Utils::Logger::log("Update failed.", Utils::LogType::Warning, "PersistentDatabase::updateClanContribution");
                    return;
                }
            }
            catch (const sql::SQLException& e)
            {
                ::Utils::Logger::log("MariaDB exception: " + std::string(e.what()), Utils::LogType::Error, "PersistentDatabase::updateClanContribution");
                return;
            }
        }

        void PersistentDatabase::updateClanStats(std::uint32_t clanId, Main::Enums::MatchEnd endType)
        {
            try
            {
                std::string updateQueryStr = "UPDATE Clans SET TotalWins = TotalWins + ?, TotalLosses = TotalLosses + ?, "
                    "TotalDraws = TotalDraws + ? WHERE ClanId = ?";

                std::unique_ptr<sql::PreparedStatement> stmt(m_con->prepareStatement(updateQueryStr));

                stmt->setUInt(1, (endType == Main::Enums::MATCH_WON) ? 1 : 0);  
                stmt->setUInt(2, (endType == Main::Enums::MATCH_LOST) ? 1 : 0);
                stmt->setUInt(3, (endType == Main::Enums::MATCH_DRAW) ? 1 : 0);  
                stmt->setUInt(4, clanId);

                if (stmt->executeUpdate() == 0)
                {
                    ::Utils::Logger::log("Update failed.", Utils::LogType::Warning, "PersistentDatabase::updateClanStats");
                    return;
                }
            }
            catch (const sql::SQLException& e)
            {
                ::Utils::Logger::log("MariaDB exception: " + std::string(e.what()), Utils::LogType::Error, "PersistentDatabase::updateClanStats");
                return;
            }
        }

    } // end namespace Main
} // end namespace Persistence
