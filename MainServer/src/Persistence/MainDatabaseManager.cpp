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
        PersistentDatabase::PersistentDatabase()
        {
            connectWithRetry();
            initialize();
        }

        void PersistentDatabase::initialize()
        {
            std::unique_ptr<sql::Statement> stmt(m_con->createStatement());

            stmt->execute("CREATE TABLE IF NOT EXISTS EventMissionsInfo (StartDate DATETIME NOT NULL, EndDate DATETIME NOT NULL)");
            stmt->execute("CREATE TABLE IF NOT EXISTS TradeEvents (StartDate DATETIME NOT NULL, EndDate DATETIME NOT NULL)");
            stmt->execute("CREATE TABLE IF NOT EXISTS CapsuleEvents (StartDate DATETIME NOT NULL, EndDate DATETIME NOT NULL, NewMpPrice INT NOT NULL, NewRtPrice INT NOT NULL)");
            stmt->execute("CREATE TABLE IF NOT EXISTS ExpMpBonusEvents (StartDate DATETIME NOT NULL, EndDate DATETIME NOT NULL, ExpBonusPercent INT NOT NULL, MpBonusPercent INT NOT NULL)");
            stmt->execute(R"(CREATE TABLE IF NOT EXISTS GameLogs (ID INT AUTO_INCREMENT PRIMARY KEY,LogType VARCHAR(255) NOT NULL,Message TEXT NOT NULL,
                Severity VARCHAR(20) NOT NULL,CreatedAt TIMESTAMP DEFAULT CURRENT_TIMESTAMP))");
            stmt->execute(R"(CREATE TABLE IF NOT EXISTS PendingClanRequests (AccountID INT UNSIGNED NOT NULL, ClanID INT UNSIGNED NOT NULL, RequestTime DATETIME NOT NULL))");
            try { stmt->execute("ALTER TABLE Users ADD COLUMN IF NOT EXISTS VotekickDisabledUntil DATETIME NULL DEFAULT NULL"); } catch (...) {}
            try { stmt->execute("ALTER TABLE Clans ADD COLUMN IF NOT EXISTS LeaderAid INT NULL DEFAULT NULL"); } catch (...) {}
            try { stmt->execute("ALTER TABLE Clans MODIFY COLUMN ClanId INT NOT NULL AUTO_INCREMENT, ADD PRIMARY KEY (ClanId)");} catch (...) {}
            try { stmt->execute("ALTER TABLE Clans AUTO_INCREMENT = 10"); } catch (...) {}
            try { stmt->execute("ALTER TABLE Clans ADD CONSTRAINT uq_clan_name UNIQUE (ClanName)"); } catch (...) {}       
            try { stmt->execute("ALTER TABLE Users ADD CONSTRAINT uq_users_username UNIQUE (Username)"); } catch (...) {}
            try { stmt->execute("ALTER TABLE Users ADD CONSTRAINT uq_users_nickname UNIQUE (Nickname)"); } catch (...) {}
            try { stmt->execute("ALTER TABLE Users ADD COLUMN IF NOT EXISTS LastIpSalt VARCHAR(128) NULL DEFAULT NULL"); } catch (...) {}
            try { stmt->execute("ALTER TABLE Users ADD COLUMN IF NOT EXISTS HWIDSalt VARCHAR(128) NULL DEFAULT NULL"); } catch (...) {}
            try { stmt->execute("ALTER TABLE Users ADD COLUMN IF NOT EXISTS HWIDGraded VARCHAR(128) NULL DEFAULT NULL"); } catch (...) {}
            try { stmt->execute("ALTER TABLE Users ADD COLUMN IF NOT EXISTS HWIDGradedSalt VARCHAR(128) NULL DEFAULT NULL"); } catch (...) {}
            try { stmt->execute("ALTER TABLE Users ADD COLUMN IF NOT EXISTS EventEliminationWins INT NOT NULL DEFAULT 0"); } catch (...) {}
            try { stmt->execute("ALTER TABLE UserItems ADD COLUMN IF NOT EXISTS IsTradeable INT NOT NULL DEFAULT 1"); } catch (...) {}
            try { stmt->execute("CREATE INDEX IF NOT EXISTS idx_useritems_account_item ON UserItems(AccountID, ItemNumber)"); }catch (...) {}

            try { stmt->execute("ALTER TABLE Users ADD COLUMN IF NOT EXISTS CanUpdateNickname BOOLEAN NOT NULL DEFAULT 0"); } catch (...) {}
            try { stmt->execute("UPDATE Users SET CanUpdateNickname = 1 WHERE CanUpdateNickname = 0 AND AccountID IS NOT NULL"); } catch (...) {}
        }

        void PersistentDatabase::connectWithRetry()
        {
            const auto& dbSetup = Common::Utils::SetupParser::getInstance().getDatabaseSetup();

            constexpr int maxRetries = 5;

            for (int attempt = 1; attempt <= maxRetries; ++attempt)
            {
                try
                {
                    ::Utils::Logger::log("Connecting to MariaDB (attempt " + std::to_string(attempt) + ")", Utils::LogType::Info, "PersistentDatabase");

                    m_con = std::unique_ptr<sql::Connection>(sql::mariadb::get_driver_instance()->connect("tcp://" + dbSetup.ip + ":" + std::to_string(dbSetup.port),
                            dbSetup.username,dbSetup.password));                    
                    m_con->setSchema(dbSetup.databaseName);
                    m_con->setAutoCommit(true);

                    m_transactionalCon = std::unique_ptr<sql::Connection>(sql::mariadb::get_driver_instance()->connect( "tcp://" + dbSetup.ip + ":" + std::to_string(dbSetup.port),
                            dbSetup.username,dbSetup.password));     
                    m_transactionalCon->setSchema(dbSetup.databaseName);
                    m_transactionalCon->setAutoCommit(false);

                    ::Utils::Logger::log("Connected to MariaDB successfully", Utils::LogType::Info, "PersistentDatabase");

                    return;
                }
                catch (const sql::SQLException& e)
                {
                    ::Utils::Logger::log("Connection failed: " + std::string(e.what()), Utils::LogType::Error, "PersistentDatabase");

                    if (attempt == maxRetries)
                    {
                        ::Utils::Logger::log("Max connection retries reached!", Utils::LogType::Error, "PersistentDatabase");
                        throw;
                    }

                    std::this_thread::sleep_for(std::chrono::seconds(1 * attempt));
                }
            }
        }

        void PersistentDatabase::recreateConnection(std::unique_ptr<sql::Connection>& conn, bool autoCommit)
        {
            const auto& dbSetup = Common::Utils::SetupParser::getInstance().getDatabaseSetup();

            try {
                conn.reset(sql::mariadb::get_driver_instance()->connect(
                    "tcp://" + dbSetup.ip + ":" + std::to_string(dbSetup.port),
                    dbSetup.username,
                    dbSetup.password
                ));
                conn->setSchema(dbSetup.databaseName);
                conn->setAutoCommit(autoCommit);

                ::Utils::Logger::log("Connection recreated successfully (autoCommit=" +
                    std::string(autoCommit ? "true" : "false") + ")",
                    Utils::LogType::Info, "PersistentDatabase::recreateConnection");
            }
            catch (const sql::SQLException& e) {
                ::Utils::Logger::log("Failed to recreate connection: " + std::string(e.what()),
                    Utils::LogType::Error, "PersistentDatabase::recreateConnection");
                throw; 
            }
        }

        bool PersistentDatabase::isValidConnection(const std::unique_ptr<sql::Connection>& conn)
        {
            if (!conn) return false;

            try {
                if (conn->isClosed()) return false;

                std::unique_ptr<sql::Statement> stmt(conn->createStatement());
                std::unique_ptr<sql::ResultSet> res(stmt->executeQuery("SELECT 1"));

                return true;
            }
            catch (const sql::SQLException& e) {
                ::Utils::Logger::log("Connection validation failed: " + std::string(e.what()),Utils::LogType::Error, "PersistentDatabase::isValidConnection");
                return false;
            }
            catch (...) {
                return false;
            }
        }

        void PersistentDatabase::ensureConnections()
        {
            if (!isValidConnection(m_con)) 
            {
                recreateConnection(m_con, true);  
            }

            if (!isValidConnection(m_transactionalCon)) 
            {
                recreateConnection(m_transactionalCon, false); 
            }
        }

        void PersistentDatabase::updatePlayerCurrencyByType(std::uint32_t accountID, std::uint32_t newAmount, Main::Enums::ItemCurrencyType currencyType)
        {
            try
            {
                std::string sql;
                if (currencyType == Main::Enums::ITEM_MP) sql = "UPDATE Users SET MicroPoints = ? WHERE AccountID = ?";
                else if (currencyType == Main::Enums::ITEM_RT) sql = "UPDATE Users SET RockTotens = ? WHERE AccountID = ?";
                else if (currencyType == Main::Enums::ITEM_COUPON) sql = "UPDATE Users SET Coupons = ? WHERE AccountID = ?";
                else if (currencyType == Main::Enums::ITEM_COIN) sql = "UPDATE Users SET Coins = ? WHERE AccountID = ?";

                std::unique_ptr<sql::PreparedStatement> stmt(m_con->prepareStatement(sql));
                stmt->setUInt(1, newAmount);
                stmt->setUInt(2, accountID);

                stmt->executeUpdate();
            }
            catch (const sql::SQLException& e)
            {
                ::Utils::Logger::log("MariaDB exception: " + std::string(e.what()), Utils::LogType::Error, "PersistentDatabase::updatePlayerCurrencyByType");
                return;
            }
        }

        void PersistentDatabase::addPlayerAchievement(std::uint32_t accountID, std::uint32_t achievementIndex)
        {
            try
            {
                std::string sql = "INSERT INTO UserAchievements (AccountID, AchievementIndex) VALUES (?, ?)";

                std::unique_ptr<sql::PreparedStatement> stmt(m_con->prepareStatement(sql));
                stmt->setUInt(1, accountID);
                stmt->setUInt(2, achievementIndex);

                stmt->executeUpdate();
            }
            catch (const sql::SQLException& e)
            {
                ::Utils::Logger::log("MariaDB exception: " + std::string(e.what()), Utils::LogType::Error, "PersistentDatabase::addPlayerAchievement");
                return;
            }
        }

        std::optional<std::string> PersistentDatabase::getColumnByAid(const std::string& columnName, std::uint32_t accountID)
        {
            try
            {
                std::string sql = "SELECT " + columnName + " FROM Users WHERE AccountID = ? LIMIT 1";

                std::unique_ptr<sql::PreparedStatement> stmt(m_con->prepareStatement(sql));
                stmt->setUInt(1, accountID);

                std::unique_ptr<sql::ResultSet> res(stmt->executeQuery());

                if (res->next())
                {
                    return res->getString(columnName).c_str();
                }

                return std::nullopt;
            }
            catch (const sql::SQLException& e)
            {
                ::Utils::Logger::log("MariaDB exception: " + std::string(e.what()),Utils::LogType::Error,"PersistentDatabase::getColumnByAid");
                return std::nullopt;
            }
        }

        bool PersistentDatabase::updatePasswordByAid(std::uint32_t accountID, const std::string& newHashedPassword)
        {
            try
            {
                std::string sql = "UPDATE Users SET Password = ? WHERE AccountID = ?";

                std::unique_ptr<sql::PreparedStatement> stmt(m_con->prepareStatement(sql));
                stmt->setString(1, newHashedPassword);
                stmt->setUInt(2, accountID);

                const int affectedRows = stmt->executeUpdate();
                return affectedRows > 0;
            }
            catch (const sql::SQLException& e)
            {
                ::Utils::Logger::log("MariaDB exception: " + std::string(e.what()),Utils::LogType::Error,"PersistentDatabase::updatePasswordByAid");
                return false;;
            }
        }

        bool PersistentDatabase::updateSecretByAid(std::uint32_t accountID, const std::string& newEncryptedSecret)
        {
            try
            {
                std::string sql = "UPDATE Users SET Secret = ? WHERE AccountID = ?";

                std::unique_ptr<sql::PreparedStatement> stmt(m_con->prepareStatement(sql));
                stmt->setString(1, newEncryptedSecret);
                stmt->setUInt(2, accountID);

                const int affectedRows = stmt->executeUpdate();
                return affectedRows > 0;
            }
            catch (const sql::SQLException& e)
            {
                ::Utils::Logger::log("MariaDB exception: " + std::string(e.what()), Utils::LogType::Error, "PersistentDatabase::updateSecretByAid");
                return false;
            }
        }

        void PersistentDatabase::logMessage(std::uint32_t accountID, const std::string& message)
        {
            try
            {
                std::string truncatedMessage = message.substr(0, 300);
                std::string sql = "INSERT INTO ChatLogs (AccountID, Message) VALUES (?, ?)";
                std::unique_ptr<sql::PreparedStatement> stmt(m_con->prepareStatement(sql));
                stmt->setUInt(1, accountID);
                stmt->setString(2, truncatedMessage);

                stmt->executeUpdate();
            }
            catch (const sql::SQLException& e)
            {
                ::Utils::Logger::log("MariaDB exception: " + std::string(e.what()), Utils::LogType::Error, "PersistentDatabase::logMessage");
                return;
            }
        }

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

        std::vector<std::pair<std::uint32_t, std::uint32_t>> PersistentDatabase::getPlayerAchievements(std::uint32_t accountID)
        {
            std::vector<std::pair<std::uint32_t, std::uint32_t>> achievements;

            try
            {
                std::string sql = "SELECT AchievementIndex, AchievementType FROM UserAchievements WHERE AccountID = ?";

                std::unique_ptr<sql::PreparedStatement> stmt(m_con->prepareStatement(sql));
                stmt->setUInt(1, accountID);

                std::unique_ptr<sql::ResultSet> res(stmt->executeQuery());

                while (res->next())
                {
                    std::uint32_t achievementIndex = res->getUInt("AchievementIndex");
                    std::uint32_t achievementType = res->getUInt("AchievementType");
                    achievements.emplace_back(achievementIndex, achievementType);
                }
            }
            catch (const sql::SQLException& e)
            {
                ::Utils::Logger::log("MariaDB exception: " + std::string(e.what()), Utils::LogType::Error, "PersistentDatabase::getPlayerAchievements");
                return achievements;
            }

            return achievements;
        }

        std::unordered_map<std::uint32_t, std::uint32_t> PersistentDatabase::getPlayerMissions(std::uint32_t accountID)
        {
            std::unordered_map<std::uint32_t, std::uint32_t> missions;

            try
            {
                TransactionGuard guard(m_transactionalCon.get());

                std::string selectSql = "SELECT TotalMission1, TotalMission2, TotalMission3, TotalMission4, TotalMission5 "
                    "FROM EventMissions WHERE AccountID = ?";

                std::unique_ptr<sql::PreparedStatement> selectStmt(m_transactionalCon->prepareStatement(selectSql));
                selectStmt->setUInt(1, accountID);

                std::unique_ptr<sql::ResultSet> res(selectStmt->executeQuery());

                if (res->next())
                {
                    for (int i = 1; i <= Common::Constants::totalEventMissions; ++i)
                    {
                        std::uint32_t missionProgress = res->isNull("TotalMission" + std::to_string(i))
                            ? 0
                            : res->getUInt("TotalMission" + std::to_string(i));

                        if (missionProgress < Common::Constants::eventMissionTotal)
                        {
                            missions[i] = missionProgress;
                        }
                    }
                }
                else
                {
                    std::string insertSql = "INSERT INTO EventMissions (AccountID, TotalMission1, TotalMission2, TotalMission3, "
                        "TotalMission4, TotalMission5) VALUES (?, 0, 0, 0, 0, 0)";
                    std::unique_ptr<sql::PreparedStatement> insertStmt(m_transactionalCon->prepareStatement(insertSql));
                    insertStmt->setUInt(1, accountID);
                    insertStmt->executeUpdate();

                    for (int i = 1; i <= Common::Constants::totalEventMissions; ++i)
                    {
                        missions[i] = 0;
                    }
                }

                guard.commit();
            }
            catch (const sql::SQLException& e)
            {
                ::Utils::Logger::log("MariaDB exception: " + std::string(e.what()), Utils::LogType::Error, "PersistentDatabase::getPlayerMissions");
            }

            return missions;
        }

        std::optional<Main::Structures::EventMissionInfo> PersistentDatabase::getEventInfo(const std::string& tableName)
        {
            try
            {
                TransactionGuard guard(m_transactionalCon.get());

                const std::string query = "SELECT StartDate, EndDate FROM " + tableName + " LIMIT 1";
                std::unique_ptr<sql::PreparedStatement> stmt(m_transactionalCon->prepareStatement(query));
                std::unique_ptr<sql::ResultSet> res(stmt->executeQuery());

                if (res->next())
                {
                    Main::Structures::EventMissionInfo info;

                    std::string startDateStr = res->getString("StartDate").c_str();
                    std::string endDateStr = res->getString("EndDate").c_str();

                    info.startDate = Common::Utils::datetimeToEpoch(startDateStr);
                    info.endDate = Common::Utils::datetimeToEpoch(endDateStr);

                    guard.commit();
                    return info;
                }
                else
                {
                    std::string epochZeroStr = Common::Utils::epochToDatetime(0);
                    const std::string insertQuery = "INSERT INTO " + tableName + " (StartDate, EndDate) VALUES (?, ?)";
                    std::unique_ptr<sql::PreparedStatement> insertStmt(m_transactionalCon->prepareStatement(insertQuery));
                    insertStmt->setString(1, epochZeroStr);
                    insertStmt->setString(2, epochZeroStr);
                    insertStmt->executeUpdate();

                    guard.commit();
                    return Main::Structures::EventMissionInfo{ 0, 0 };
                }
            }
            catch (const sql::SQLException& e)
            {
                ::Utils::Logger::log("MariaDB exception: " + std::string(e.what()),Utils::LogType::Error,"PersistentDatabase::getEventInfo (" + tableName + ")");
                return std::nullopt;
            }
        }

        std::optional<Main::Structures::CapsuleListDatabase> PersistentDatabase::getCapsuleEvent()
        {
            try
            {
                TransactionGuard guard(m_transactionalCon.get());

                const std::string query = R"(SELECT StartDate, EndDate, NewMpPrice, NewRtPrice FROM CapsuleEvents LIMIT 1)";

                std::unique_ptr<sql::PreparedStatement> stmt(m_transactionalCon->prepareStatement(query));
                std::unique_ptr<sql::ResultSet> res(stmt->executeQuery());

                if (res->next())
                {
                    Main::Structures::CapsuleListDatabase capsule;

                    std::string startDateStr = res->getString("StartDate").c_str();
                    std::string endDateStr = res->getString("EndDate").c_str();

                    capsule.saleEventStartDate = Common::Utils::datetimeToEpoch(startDateStr);
                    capsule.saleEventEndDate = Common::Utils::datetimeToEpoch(endDateStr);
                    capsule.newMpPrice = res->getUInt("NewMpPrice");
                    capsule.newRtPrice = res->getUInt("NewRtPrice");

                    guard.commit();
                    return capsule;
                }
                else
                {
                    std::string epochZeroStr = Common::Utils::epochToDatetime(0);

                    const std::string insertQuery = R"(INSERT INTO CapsuleEvents (StartDate, EndDate, NewMpPrice, NewRtPrice) VALUES (?, ?, 0, 0))";

                    std::unique_ptr<sql::PreparedStatement> insertStmt(m_transactionalCon->prepareStatement(insertQuery));
                    insertStmt->setString(1, epochZeroStr);
                    insertStmt->setString(2, epochZeroStr);
                    insertStmt->executeUpdate();

                    guard.commit();
                    return Main::Structures::CapsuleListDatabase{};
                }
            }
            catch (const sql::SQLException& e)
            {
                ::Utils::Logger::log("MariaDB exception: " + std::string(e.what()), Utils::LogType::Error, "PersistentDatabase::getCapsuleEvent"); 
            }

            return std::nullopt;
        }

        bool PersistentDatabase::updateCapsuleEvent(const Main::Structures::CapsuleListDatabase& capsule)
        {
            try
            {
                TransactionGuard txn(m_transactionalCon.get());

                const std::string checkQuery = "SELECT COUNT(*) as Count FROM CapsuleEvents";
                std::unique_ptr<sql::PreparedStatement> checkStmt(m_transactionalCon->prepareStatement(checkQuery));
                std::unique_ptr<sql::ResultSet> checkRes(checkStmt->executeQuery());

                bool exists = false;
                if (checkRes->next())
                {
                    exists = checkRes->getUInt("Count") > 0;
                }

                std::string startDateStr = Common::Utils::epochToDatetime(capsule.saleEventStartDate);
                std::string endDateStr = Common::Utils::epochToDatetime(capsule.saleEventEndDate);

                if (exists)
                {
                    const std::string updateQuery = R"(UPDATE CapsuleEvents SET StartDate = ?, EndDate = ?, NewMpPrice = ?, NewRtPrice = ?)";

                    std::unique_ptr<sql::PreparedStatement> updateStmt(m_transactionalCon->prepareStatement(updateQuery));
                    updateStmt->setString(1, startDateStr);
                    updateStmt->setString(2, endDateStr);
                    updateStmt->setUInt(3, capsule.newMpPrice);
                    updateStmt->setUInt(4, capsule.newRtPrice);
                    updateStmt->executeUpdate();
                }
                else
                {
                    const std::string insertQuery = R"(INSERT INTO CapsuleEvents (StartDate, EndDate, NewMpPrice, NewRtPrice) VALUES (?, ?, ?, ?))";

                    std::unique_ptr<sql::PreparedStatement> insertStmt(m_transactionalCon->prepareStatement(insertQuery));
                    insertStmt->setString(1, startDateStr);
                    insertStmt->setString(2, endDateStr);
                    insertStmt->setUInt(3, capsule.newMpPrice);
                    insertStmt->setUInt(4, capsule.newRtPrice);
                    insertStmt->executeUpdate();
                }

                txn.commit();
                return true;
            }
            catch (const sql::SQLException& e)
            {
                ::Utils::Logger::log("MariaDB exception: " + std::string(e.what()),Utils::LogType::Error,"PersistentDatabase::updateCapsuleEvent");
                return false;
            }
        }

        bool PersistentDatabase::insertItemLogs(std::uint32_t accountId, const std::vector<Main::Structures::ItemLogInfo>& logs)
        {
            if (logs.empty()) return true;

            try
            {
                TransactionGuard txn(m_transactionalCon.get());

                const std::string query = R"(INSERT INTO ItemLogs (AccountID, Date, ItemNumber, ItemID, Action, ExpirationDate) VALUES (?, NOW(), ?, ?, ?, ?))";

                std::unique_ptr<sql::PreparedStatement> stmt(m_transactionalCon->prepareStatement(query));

                for (const auto& log : logs)
                {
                    stmt->setUInt64(1, accountId);
                    stmt->setUInt(2, log.itemNumber);
                    stmt->setUInt64(3, log.itemId);
                    stmt->setString(4, log.action);
                    stmt->setUInt(5, log.expirationDate);
                    stmt->executeUpdate();
                }

                txn.commit();
                return true;
            }
            catch (const sql::SQLException& e)
            {
                ::Utils::Logger::log("MariaDB exception: " + std::string(e.what()),Utils::LogType::Error,"PersistentDatabase::insertItemLogs");
                return false;
            }
        }

        bool PersistentDatabase::insertItemLog(std::uint32_t accountId, const Main::Structures::ItemLogInfo& log)
        {
            try
            {
                const std::string query = R"(INSERT INTO ItemLogs (AccountID, Date, ItemNumber, ItemID, Action, ExpirationDate) VALUES (?, NOW(), ?, ?, ?, ?))";

                std::unique_ptr<sql::PreparedStatement> stmt(m_con->prepareStatement(query));
                stmt->setUInt64(1, accountId);
                stmt->setUInt(2, log.itemNumber);
                stmt->setUInt64(3, log.itemId);
                stmt->setString(4, log.action);
                stmt->setUInt(5, log.expirationDate);
                stmt->executeUpdate();

                return true;
            }
            catch (const sql::SQLException& e)
            {
                ::Utils::Logger::log("MariaDB exception: " + std::string(e.what()), Utils::LogType::Error, "PersistentDatabase::insertItemLog");
                return false;
            }

            return false;
        }

        bool PersistentDatabase::logBoughtItems(std::uint32_t accountId, const std::vector<Main::Structures::BoughtItem>& boughtItems, bool isCouponItems)
        {
            std::vector<Main::Structures::ItemLogInfo> logInfos;
            for (const auto& currentBoughtItem : boughtItems)
            {
                logInfos.emplace_back(Main::Structures::ItemLogInfo{ currentBoughtItem.serialInfo.itemNumber, 
                    currentBoughtItem.itemId.itemId, currentBoughtItem.unknown, 
                    isCouponItems ? "The item was bought from the coupon shop" : "The item was bought from the normal shop" });
            }

            return insertItemLogs(accountId, logInfos);
        }

        bool PersistentDatabase::logBoxItems(std::uint32_t accountId, const std::vector<Main::Structures::BoxItem>& boxItem)
        {
            std::vector<Main::Structures::ItemLogInfo> logInfos;
            for (const auto& currentBoxItem : boxItem)
            {
                logInfos.emplace_back(Main::Structures::ItemLogInfo{ currentBoxItem.serialInfo.itemNumber,
                    currentBoxItem.itemId.itemId, currentBoxItem.expirationDate,
                    "The item was won through a box" });
            }

            return insertItemLogs(accountId, logInfos);
        }

        bool PersistentDatabase::logProlongedItems(std::uint32_t accountId, const std::vector<Main::Structures::BoughtItemToProlong>& boughtItems,
            const std::vector<std::uint64_t>& itemDurations)
        {
            // Need to use 32bits here due to the game using that .....
            const std::uint32_t timeNow = static_cast<std::uint32_t>(
                std::chrono::duration_cast<std::chrono::seconds>(std::chrono::system_clock::now().time_since_epoch()).count());

            std::vector<Main::Structures::ItemLogInfo> logInfos;
            for (std::uint32_t i = 0; const auto & currentBoughtItem : boughtItems)
            {
                std::uint32_t actualItemDuration = itemDurations[i] <= 3 ? itemDurations[i] : itemDurations[i] + timeNow;
                logInfos.emplace_back(Main::Structures::ItemLogInfo{currentBoughtItem.serialInfo.itemNumber, 0, actualItemDuration, 
                    "The item was prolonged after it expired" });
                ++i;
            }

            return insertItemLogs(accountId, logInfos);
        }

        bool PersistentDatabase::updateEventInfo(const std::string& tableName, const Main::Structures::EventMissionInfo& info)
        {
            try
            {
                TransactionGuard txn(m_transactionalCon.get());

                const std::string ensureRowQuery = "SELECT COUNT(*) AS RowCount FROM " + tableName;
                std::unique_ptr<sql::PreparedStatement> checkStmt(m_transactionalCon->prepareStatement(ensureRowQuery));
                std::unique_ptr<sql::ResultSet> checkRes(checkStmt->executeQuery());

                if (checkRes->next() && checkRes->getUInt("RowCount") == 0)
                {
                    std::string epochZeroStr = Common::Utils::epochToDatetime(0);
                    const std::string insertQuery = "INSERT INTO " + tableName + " (StartDate, EndDate) VALUES (?, ?)";
                    std::unique_ptr<sql::PreparedStatement> insertStmt(m_transactionalCon->prepareStatement(insertQuery));
                    insertStmt->setString(1, epochZeroStr);
                    insertStmt->setString(2, epochZeroStr);
                    insertStmt->executeUpdate();
                }

                std::string startDateStr = Common::Utils::epochToDatetime(info.startDate);
                std::string endDateStr = Common::Utils::epochToDatetime(info.endDate);

                const std::string updateQuery = "UPDATE " + tableName + " SET StartDate = ?, EndDate = ? LIMIT 1";
                std::unique_ptr<sql::PreparedStatement> updateStmt(m_transactionalCon->prepareStatement(updateQuery));
                updateStmt->setString(1, startDateStr);
                updateStmt->setString(2, endDateStr);

                bool success = updateStmt->executeUpdate() > 0;
                txn.commit();
                return success;
            }
            catch (const sql::SQLException& e)
            {
                ::Utils::Logger::log("MariaDB exception: " + std::string(e.what()),Utils::LogType::Error,"PersistentDatabase::updateEventInfo (" + tableName + ")");
            }

            return false;
        }

        std::optional<Main::Structures::ExpMpBonusInfo> PersistentDatabase::getExpMpBonusInfo()
        {
            try
            {
                TransactionGuard txn(m_transactionalCon.get());

                const std::string query = R"(SELECT StartDate, EndDate, ExpBonusPercent, MpBonusPercent FROM ExpMpBonusEvents LIMIT 1)";

                std::unique_ptr<sql::PreparedStatement> stmt(m_transactionalCon->prepareStatement(query));
                std::unique_ptr<sql::ResultSet> res(stmt->executeQuery());

                if (res->next())
                {
                    Main::Structures::ExpMpBonusInfo info;

                    std::string startDateStr = res->getString("StartDate").c_str();
                    std::string endDateStr = res->getString("EndDate").c_str();

                    info.startDate = Common::Utils::datetimeToEpoch(startDateStr);
                    info.endDate = Common::Utils::datetimeToEpoch(endDateStr);

                    info.expBonusPercent = res->getUInt("ExpBonusPercent");
                    info.mpBonusPercent = res->getUInt("MpBonusPercent");

                    txn.commit();
                    return info;
                }
                else
                {
                    std::string epochZeroStr = Common::Utils::epochToDatetime(0);

                    const std::string insertQuery = R"(INSERT INTO ExpMpBonusEvents (StartDate, EndDate, ExpBonusPercent, MpBonusPercent) VALUES (?, ?, 0, 0))";

                    std::unique_ptr<sql::PreparedStatement> insertStmt(m_transactionalCon->prepareStatement(insertQuery));
                    insertStmt->setString(1, epochZeroStr);
                    insertStmt->setString(2, epochZeroStr);
                    insertStmt->executeUpdate();

                    txn.commit();
                    return Main::Structures::ExpMpBonusInfo{ 0, 0, 0, 0 };
                }
            }
            catch (const sql::SQLException& e)
            {
                ::Utils::Logger::log("MariaDB exception: " + std::string(e.what()),Utils::LogType::Error,"PersistentDatabase::getExpMpBonusInfo");
                return std::nullopt;
            }
        }

        bool PersistentDatabase::updateExpMpBonusInfo(const Main::Structures::ExpMpBonusInfo& info)
        {
            try
            {
                TransactionGuard txn(m_transactionalCon.get());

                const std::string checkQuery = R"(SELECT COUNT(*) AS RowCount FROM ExpMpBonusEvents)";
                std::unique_ptr<sql::PreparedStatement> checkStmt(m_transactionalCon->prepareStatement(checkQuery));
                std::unique_ptr<sql::ResultSet> checkRes(checkStmt->executeQuery());

                if (checkRes->next() && checkRes->getUInt("RowCount") == 0)
                {
                    std::string epochZeroStr = Common::Utils::epochToDatetime(0);

                    const std::string insertQuery = R"(INSERT INTO ExpMpBonusEvents (StartDate, EndDate, ExpBonusPercent, MpBonusPercent) VALUES (?, ?, 0, 0))";

                    std::unique_ptr<sql::PreparedStatement> insertStmt(m_transactionalCon->prepareStatement(insertQuery));
                    insertStmt->setString(1, epochZeroStr);
                    insertStmt->setString(2, epochZeroStr);
                    insertStmt->executeUpdate();
                }

                std::string startDateStr = Common::Utils::epochToDatetime(info.startDate);
                std::string endDateStr = Common::Utils::epochToDatetime(info.endDate);

                const std::string updateQuery = R"(UPDATE ExpMpBonusEvents  SET StartDate = ?, EndDate = ?, ExpBonusPercent = ?, MpBonusPercent = ?  LIMIT 1)";

                std::unique_ptr<sql::PreparedStatement> updateStmt(m_transactionalCon->prepareStatement(updateQuery));
                updateStmt->setString(1, startDateStr);
                updateStmt->setString(2, endDateStr);
                updateStmt->setUInt(3, info.expBonusPercent);
                updateStmt->setUInt(4, info.mpBonusPercent);
                updateStmt->executeUpdate();

                const std::string updateModes = R"(UPDATE EventModes SET EndDate = ?)";
                std::unique_ptr<sql::PreparedStatement> updateStmtModes(m_transactionalCon->prepareStatement(updateModes));
                updateStmtModes->setString(1, endDateStr);
                updateStmtModes->executeUpdate();

                const std::string updateMaps = R"(UPDATE EventMaps SET EndDate = ?)";
                std::unique_ptr<sql::PreparedStatement> updateStmtMaps(m_transactionalCon->prepareStatement(updateMaps));
                updateStmtMaps->setString(1, endDateStr);
                updateStmtMaps->executeUpdate();

                txn.commit();
                return true;
            }
            catch (const sql::SQLException& e)
            {
                ::Utils::Logger::log("MariaDB exception: " + std::string(e.what()),Utils::LogType::Error,"PersistentDatabase::updateExpMpBonusInfo");
            }

            return false;
        }

        bool PersistentDatabase::updatePlayerMissionProgress(std::uint32_t accountID, std::uint32_t missionID, std::uint32_t newProgress)
        {
            if (missionID == 0 || missionID > Common::Constants::totalEventMissions)
            {
                ::Utils::Logger::log("Invalid mission ID: " + std::to_string(missionID),Utils::LogType::Warning, "PersistentDatabase::updatePlayerMissionProgress");
                return false;
            }

            try
            {
                TransactionGuard txn(m_transactionalCon.get());

                std::string checkSql = "SELECT COUNT(*) FROM EventMissions WHERE AccountID = ?";
                std::unique_ptr<sql::PreparedStatement> checkStmt(m_transactionalCon->prepareStatement(checkSql));
                checkStmt->setUInt(1, accountID);
                std::unique_ptr<sql::ResultSet> checkRes(checkStmt->executeQuery());

                bool exists = false;
                if (checkRes->next())
                {
                    exists = checkRes->getUInt(1) > 0;
                }

                if (exists)
                {
                    std::string column = "TotalMission" + std::to_string(missionID);
                    std::string updateSql = "UPDATE EventMissions SET " + column + " = ? WHERE AccountID = ?";
                    std::unique_ptr<sql::PreparedStatement> updateStmt(m_transactionalCon->prepareStatement(updateSql));
                    updateStmt->setUInt(1, newProgress);
                    updateStmt->setUInt(2, accountID);
                    updateStmt->executeUpdate();
                }
                else
                {
                    std::string insertSql = "INSERT INTO EventMissions (AccountID, TotalMission1, TotalMission2, TotalMission3, TotalMission4, TotalMission5) "
                        "VALUES (?, ?, ?, ?, ?, ?)";
                    std::unique_ptr<sql::PreparedStatement> insertStmt(m_transactionalCon->prepareStatement(insertSql));
                    insertStmt->setUInt(1, accountID);
                    for (int i = 1; i <= Common::Constants::totalEventMissions; ++i)
                    {
                        insertStmt->setUInt(i + 1, (i == static_cast<int>(missionID)) ? newProgress : 0);
                    }
                    insertStmt->executeUpdate();
                }

                txn.commit();
                return true;
            }
            catch (const sql::SQLException& e)
            {
                ::Utils::Logger::log("MariaDB exception: " + std::string(e.what()),Utils::LogType::Error, "PersistentDatabase::updatePlayerMissionProgress");
                return false;
            }
        }

        bool PersistentDatabase::savePlayerMissions(std::uint32_t accountID, const std::unordered_map<std::uint32_t, std::uint32_t>& activeMissions)
        {
            if (activeMissions.size() > Common::Constants::totalEventMissions)
            {
                ::Utils::Logger::log("Error: too many active missions, expected <= 5", Utils::LogType::Error, "PersistentDatabase::savePlayerMissions");
                return false;
            }

            try
            {
                TransactionGuard txn(m_transactionalCon.get());

                std::string checkSql = "SELECT COUNT(*) FROM EventMissions WHERE AccountID = ?";
                std::unique_ptr<sql::PreparedStatement> checkStmt(m_transactionalCon->prepareStatement(checkSql));
                checkStmt->setUInt(1, accountID);
                std::unique_ptr<sql::ResultSet> res(checkStmt->executeQuery());

                bool exists = false;
                if (res->next())
                {
                    exists = res->getUInt(1) > 0;
                }

                std::array<std::uint32_t, Common::Constants::totalEventMissions> totals{};
                if (exists)
                {
                    std::string fetchSql = "SELECT TotalMission1, TotalMission2, TotalMission3, TotalMission4, TotalMission5 FROM EventMissions WHERE AccountID = ?";
                    std::unique_ptr<sql::PreparedStatement> fetchStmt(m_transactionalCon->prepareStatement(fetchSql));
                    fetchStmt->setUInt(1, accountID);
                    std::unique_ptr<sql::ResultSet> fetchRes(fetchStmt->executeQuery());
                    if (fetchRes->next())
                    {
                        for (int i = 0; i < totals.size(); ++i)
                        {
                            totals[i] = fetchRes->getUInt(i + 1);
                        }
                    }
                }
                else
                {
                    totals.fill(0);
                }

                for (const auto& [missionID, total] : activeMissions)
                {
                    if (missionID >= 1 && missionID <= Common::Constants::totalEventMissions)
                    {
                        totals[missionID - 1] = total;
                    }
                }

                if (exists)
                {
                    std::string updateSql = "UPDATE EventMissions SET TotalMission1 = ?, TotalMission2 = ?, TotalMission3 = ?, "
                        "TotalMission4 = ?, TotalMission5 = ? WHERE AccountID = ?";
                    std::unique_ptr<sql::PreparedStatement> stmt(m_transactionalCon->prepareStatement(updateSql));

                    for (int i = 0; i < Common::Constants::totalEventMissions; ++i)
                    {
                        stmt->setUInt(i + 1, totals[i]);
                    }
                    stmt->setUInt(6, accountID);
                    stmt->executeUpdate();
                }
                else
                {
                    std::string insertSql = "INSERT INTO EventMissions (AccountID, TotalMission1, TotalMission2, TotalMission3, "
                        "TotalMission4, TotalMission5) VALUES (?, ?, ?, ?, ?, ?)";
                    std::unique_ptr<sql::PreparedStatement> stmt(m_transactionalCon->prepareStatement(insertSql));

                    stmt->setUInt(1, accountID);
                    for (int i = 0; i < Common::Constants::totalEventMissions; ++i)
                    {
                        stmt->setUInt(i + 2, totals[i]);
                    }
                    stmt->executeUpdate();
                }

                txn.commit();
                return true;
            }
            catch (const sql::SQLException& e)
            {
                ::Utils::Logger::log("MariaDB exception: " + std::string(e.what()), Utils::LogType::Error, "PersistentDatabase::savePlayerMissions");
                return false;
            }
        }

        void PersistentDatabase::updateLatestSelectedCharacter(std::uint32_t accountID, std::uint16_t characterId)
        {
            try
            {
                const std::string updateCharacterQuery = "UPDATE Users SET LastCharacterUsed = ? WHERE AccountID = ?";
                std::unique_ptr<sql::PreparedStatement> stmt(m_con->prepareStatement(updateCharacterQuery));
                stmt->setUInt(1, characterId);
                stmt->setUInt(2, accountID);
                stmt->executeUpdate();
            }
            catch (const sql::SQLException& e)
            {
                ::Utils::Logger::log("MariaDB exception: " + std::string(e.what()), Utils::LogType::Error, "PersistentDatabase::updateLatestSelectedCharacter");
                return;
            }
        }

        bool PersistentDatabase::updateUsernameByAid(std::uint32_t accountId,
            const std::string& newUsername, const std::string& oldUsername)
        {
            try
            {
                std::string queryStr = "UPDATE Users SET Username = ? WHERE AccountID = ?";

                std::unique_ptr<sql::PreparedStatement> stmt(m_con->prepareStatement(queryStr));
                stmt->setString(1, newUsername);
                stmt->setUInt(2, accountId);

                return stmt->executeUpdate() > 0;
            }
            catch (const sql::SQLException& e)
            {
                ::Utils::Logger::log("MariaDB exception: " + std::string(e.what()),
                    Utils::LogType::Error, "PersistentDatabase::updateUsernameByAid");
                return false;
            }
        }

        bool PersistentDatabase::checkUsernameExists(const std::string& username)
        {
            try
            {
                std::string queryStr = "SELECT COUNT(*) FROM Users WHERE LOWER(Username) = LOWER(?)";

                std::unique_ptr<sql::PreparedStatement> stmt(m_con->prepareStatement(queryStr));
                stmt->setString(1, username);

                std::unique_ptr<sql::ResultSet> res(stmt->executeQuery());

                if (res->next())
                {
                    return res->getInt(1) > 0;
                }

                return false;
            }
            catch (const sql::SQLException& e)
            {
                ::Utils::Logger::log("MariaDB exception: " + std::string(e.what()), Utils::LogType::Error, "PersistentDatabase::checkUsernameExists");

                return true;
            }
        }

        bool PersistentDatabase::logGameEvent(const std::string& logType, const std::string& message, const std::string& severity)
        {
            try
            {
                std::string insertQuery = "INSERT INTO GameLogs (LogType, Message, Severity) VALUES (?, ?, ?)";
                std::unique_ptr<sql::PreparedStatement> stmtInsert(m_con->prepareStatement(insertQuery));

                stmtInsert->setString(1, logType);
                stmtInsert->setString(2, message);
                stmtInsert->setString(3, severity);

                return stmtInsert->executeUpdate() > 0;
            }
            catch (const sql::SQLException& e)
            {
                ::Utils::Logger::log("MariaDB exception: " + std::string(e.what()), ::Utils::LogType::Error, "PersistentDatabase::logGameEvent");
                return false;
            }
        }

        std::optional<Main::Structures::AccountInfo> PersistentDatabase::getPlayerInfo(std::uint32_t playerID)
        {
            Main::Structures::AccountInfo playerInfoStructure{};
            try
            {
                std::string queryStr = "SELECT Users.*, Clans.Clanname as Clan_Clanname, Clans.ClanFrontIcon as Clan_FrontIcon, Clans.ClanBackIcon as Clan_BackIcon "
                    "FROM Users LEFT JOIN Clans ON Users.ClanID = Clans.ClanId WHERE AccountID = ?";

                std::unique_ptr<sql::PreparedStatement> stmt(m_con->prepareStatement(queryStr));
                stmt->setUInt(1, playerID);
                std::unique_ptr<sql::ResultSet> res(stmt->executeQuery());

                if (res->next())
                {
                    playerInfoStructure.accountID = playerID;
                    std::memcpy(playerInfoStructure.nickname, res->getString("Nickname"), Common::Constants::maxNicknameSize);
                    std::memcpy(playerInfoStructure.clanName, res->getString("Clan_Clanname"), Common::Constants::maxNicknameSize);
                    playerInfoStructure.accountKey = static_cast<std::uint32_t>(res->getUInt("AccountKey"));
                    playerInfoStructure.totalKills = static_cast<std::uint32_t>(res->getInt("Kills"));
                    playerInfoStructure.deaths = static_cast<std::uint32_t>(res->getInt("Deaths"));
                    playerInfoStructure.assists = static_cast<std::uint32_t>(res->getInt("Assists"));
                    playerInfoStructure.wins = static_cast<std::uint32_t>(res->getInt("Wins"));
                    playerInfoStructure.losses = static_cast<std::uint32_t>(res->getInt("Loses"));
                    playerInfoStructure.draws = static_cast<std::uint32_t>(res->getInt("Draws"));
                    playerInfoStructure.meleeKills = static_cast<std::uint32_t>(res->getInt("MeleeKills"));
                    playerInfoStructure.rifleKills = static_cast<std::uint32_t>(res->getInt("RifleKills"));
                    playerInfoStructure.shotgunKills = static_cast<std::uint32_t>(res->getInt("ShotgunKills"));
                    playerInfoStructure.sniperKills = static_cast<std::uint32_t>(res->getInt("SniperKills"));
                    playerInfoStructure.microgunKills = static_cast<std::uint32_t>(res->getInt("GatlingKills"));
                    playerInfoStructure.bazookaKills = static_cast<std::uint32_t>(res->getInt("BazookaKills"));
                    playerInfoStructure.grenadeKills = static_cast<std::uint32_t>(res->getInt("GrenadeKills"));
                    playerInfoStructure.killstreak = static_cast<std::uint64_t>(res->getInt("HighestKillstreak"));
                    playerInfoStructure.headshots = static_cast<std::uint64_t>(res->getInt("Headshots"));
                    playerInfoStructure.playtime = static_cast<std::uint32_t>(res->getInt("Playtime"));
                    playerInfoStructure.clanId = static_cast<std::uint32_t>(res->getInt("ClanID"));
                    playerInfoStructure.latestSelectedCharacter = static_cast<std::uint64_t>(res->getInt("LastCharacterUsed"));
                    playerInfoStructure.playerLevel = static_cast<std::uint64_t>(res->getInt("Level")) + 1;
                    playerInfoStructure.battery = static_cast<std::uint64_t>(res->getInt("Battery"));
                    playerInfoStructure.luckyPoints = static_cast<std::uint64_t>(res->getInt("LuckyPoints"));
                    playerInfoStructure.coins = static_cast<std::uint64_t>(res->getInt("Coins"));
                    playerInfoStructure.playerGrade = static_cast<std::uint64_t>(res->getInt("Grade"));
                    playerInfoStructure.experience = static_cast<std::uint32_t>(res->getInt("Experience"));
                    playerInfoStructure.microPoints = static_cast<std::uint64_t>(res->getInt64("MicroPoints"));
                    playerInfoStructure.rockTotens = static_cast<std::uint64_t>(res->getInt64("RockTotens"));
                    playerInfoStructure.inventorySpace = static_cast<std::uint32_t>(res->getInt("MaxInventory"));
                    playerInfoStructure.isTutorialDone = static_cast<std::uint32_t>(res->getInt("HasFinishedTutorial"));
                    playerInfoStructure.maxBattery = static_cast<std::uint32_t>(res->getInt("MaxBattery"));
                    playerInfoStructure.singleWaveAttempts = static_cast<std::uint32_t>(res->getInt("SingleWaveAttempts"));
                    playerInfoStructure.highestSinglewaveStage = static_cast<std::uint32_t>(res->getInt("SingleWaveAttempts"));
                    playerInfoStructure.highestSingleWaveScore = static_cast<std::uint32_t>(res->getInt("HighestSinglewaveScore"));
                    playerInfoStructure.vipExperience = static_cast<std::uint32_t>(res->getInt("VipExperience"));
                    playerInfoStructure.clanContribution = static_cast<std::uint64_t>(res->getInt64("ClanContribution"));
                    playerInfoStructure.clanLogoFrontId = static_cast<std::uint64_t>(res->getInt("Clan_FrontIcon"));
                    playerInfoStructure.clanLogoBackId = static_cast<std::uint64_t>(res->getInt("Clan_BackIcon"));
                    playerInfoStructure.clanWins = static_cast<std::uint64_t>(res->getInt("ClanWins"));
                    playerInfoStructure.clanLosses = static_cast<std::uint64_t>(res->getInt("ClanLoses"));
                    playerInfoStructure.clanDraws = static_cast<std::uint64_t>(res->getInt("ClanDraws"));
                    playerInfoStructure.clanKills = static_cast<std::uint32_t>(res->getInt("ClanKills"));
                    playerInfoStructure.clanDeaths = static_cast<std::uint32_t>(res->getInt("ClanDeaths"));
                    playerInfoStructure.clanAssists = static_cast<std::uint32_t>(res->getInt("ClanAssists"));
                    playerInfoStructure.infected = static_cast<std::uint32_t>(res->getInt("InfectedKills"));
                    playerInfoStructure.zombieKills = res->getInt("ZombieKills");

                    std::vector<Common::Enums::Characters> boughtCharacterTypes{};
                    boughtCharacterTypes.reserve(static_cast<std::size_t>(Common::Enums::Characters::Sophitia));
                    for (std::size_t currentCharacter = 0; currentCharacter <= static_cast<std::size_t>(Common::Enums::Characters::Sophitia); ++currentCharacter)
                    {
                        boughtCharacterTypes.push_back(static_cast<Common::Enums::Characters>(currentCharacter));
                    }
                    playerInfoStructure.setBoughtCharacters(boughtCharacterTypes);

                    // achievements
                    for (const auto& currentAchievement : getPlayerAchievements(playerID))
                    {
                        playerInfoStructure.achievements.setAchievementTier1(currentAchievement.first);
                    }
                }
            }
            catch (const sql::SQLException& e)
            {
                ::Utils::Logger::log("MariaDB exception: " + std::string(e.what()), Utils::LogType::Error, "PersistentDatabase::getPlayerInfo");
                return std::nullopt;
            }

            return playerInfoStructure;
        }

        std::optional<std::string> PersistentDatabase::getLastLogged(std::uint32_t accountId)
        {
            try
            {
                std::string queryStr = "SELECT LastLogged FROM Users WHERE AccountID = ?";

                std::unique_ptr<sql::PreparedStatement> stmt(m_con->prepareStatement(queryStr));
                stmt->setUInt(1, accountId);

                std::unique_ptr<sql::ResultSet> res(stmt->executeQuery());

                if (res->next())
                {
                    return res->getString("LastLogged").c_str();
                }
                else
                {
                    return std::nullopt;
                }
            }
            catch (const sql::SQLException& e)
            {
                ::Utils::Logger::log("MariaDB exception: " + std::string(e.what()),Utils::LogType::Error,"PersistentDatabase::getLastLoggedByAccountId");
                return std::nullopt;
            }
        }

        bool PersistentDatabase::resetAccountKey(std::uint32_t accountId)
        {
            try
            {
                std::string queryStr = "UPDATE Users SET AccountKey = 0 WHERE AccountID = ?";

                std::unique_ptr<sql::PreparedStatement> stmt(m_con->prepareStatement(queryStr));
                stmt->setUInt(1, accountId);

                return stmt->executeUpdate() > 0;
            }
            catch (const sql::SQLException& e)
            {
                ::Utils::Logger::log("MariaDB exception: " + std::string(e.what()), Utils::LogType::Error, "PersistentDatabase::resetAccountKey");
                return false;
            }
        }

        std::optional<std::pair<Main::Structures::AccountInfo, std::string>> PersistentDatabase::getPlayerInfoByNickname(const std::string& nickname)
        {
            Main::Structures::AccountInfo playerInfoStructure{};
            try
            {
                std::string queryStr = "SELECT AccountID, Grade, Level, MicroPoints, RockTotens, LastLogged "
                    "FROM Users WHERE Nickname = ?";

                std::unique_ptr<sql::PreparedStatement> stmt(m_con->prepareStatement(queryStr));
                stmt->setString(1, nickname);
                std::unique_ptr<sql::ResultSet> res(stmt->executeQuery());

                if (res->next())
                {
                    playerInfoStructure.accountID = static_cast<std::uint32_t>(res->getUInt("AccountID"));
                    playerInfoStructure.playerGrade = static_cast<std::uint32_t>(res->getInt("Grade"));
                    playerInfoStructure.playerLevel = static_cast<std::uint64_t>(res->getInt("Level")) + 1;
                    playerInfoStructure.microPoints = static_cast<std::uint64_t>(res->getInt64("MicroPoints"));
                    playerInfoStructure.rockTotens = static_cast<std::uint64_t>(res->getInt64("RockTotens"));

                    const char* lastLoggedCStr = res->getString("LastLogged").c_str();
                    std::string lastLogged = std::string(lastLoggedCStr);
                    return std::make_optional<std::pair<Main::Structures::AccountInfo, std::string>>(playerInfoStructure, lastLogged);
                }
                else
                {
                    return std::nullopt;
                }
            }
            catch (const sql::SQLException& e)
            {
                ::Utils::Logger::log("MariaDB exception: " + std::string(e.what()), Utils::LogType::Error, "PersistentDatabase::getPlayerInfoByNickname");
                return std::nullopt;;
            }
        }

        Main::Structures::MuteInfo PersistentDatabase::isMuted(std::uint32_t playerID)
        {
            try
            {
                std::unique_ptr<sql::PreparedStatement> stmt(
                    m_con->prepareStatement("SELECT * FROM Users WHERE AccountID = ?")
                );
                stmt->setUInt(1, playerID);
                std::unique_ptr<sql::ResultSet> res(stmt->executeQuery());

                if (res->next())
                {
                    const std::string mutedUntilStr = res->getString("MutedUntil").c_str();
                    const std::string muteReason = res->getString("MuteReason").c_str();
                    const std::string mutedBy = res->getString("MutedBy").c_str();

                    std::uint64_t mutedUntilEpoch = Common::Utils::datetimeToEpoch(mutedUntilStr);
                    std::uint64_t nowEpoch = static_cast<std::uint64_t>(std::chrono::duration_cast<std::chrono::seconds>(std::chrono::system_clock::now().time_since_epoch()).count());
                    bool isMuted = nowEpoch < mutedUntilEpoch;
                    return Main::Structures::MuteInfo{ isMuted, muteReason, mutedBy, mutedUntilStr };
                }
                else
                {
                    ::Utils::Logger::log("No muteinfo found for AccountID: " + std::to_string(playerID),Utils::LogType::Warning,"PersistentDatabase::isMuted");return {};
                }
            }
            catch (const sql::SQLException& e)
            {
                ::Utils::Logger::log("MariaDB exception: " + std::string(e.what()),Utils::LogType::Error,"PersistentDatabase::isMuted");
                return Main::Structures::MuteInfo{ false, "", "", ""};
            }
        }

        std::optional<Main::Structures::MuteInfo> PersistentDatabase::getMuteInfoByNickname(const std::string& nickname)
        {
            try
            {
                std::unique_ptr<sql::PreparedStatement> stmt(m_con->prepareStatement("SELECT * FROM Users WHERE Nickname = ?"));
                stmt->setString(1, nickname);
                std::unique_ptr<sql::ResultSet> res(stmt->executeQuery());

                if (res->next())
                {
                    const std::string mutedUntilStr = res->getString("MutedUntil").c_str();
                    const std::string muteReason = res->getString("MuteReason").c_str();
                    const std::string mutedBy = res->getString("MutedBy").c_str();

                    std::uint64_t mutedUntilEpoch = Common::Utils::datetimeToEpoch(mutedUntilStr);
                    std::uint64_t nowEpoch = static_cast<std::uint64_t>(std::chrono::duration_cast<std::chrono::seconds>(std::chrono::system_clock::now().time_since_epoch()).count());
                    bool isMuted = nowEpoch < mutedUntilEpoch;
                    return Main::Structures::MuteInfo{ isMuted, muteReason, mutedBy, mutedUntilStr };
                }
                else
                {
                    ::Utils::Logger::log("No muteinfo found for nickname: " + nickname,Utils::LogType::Warning,"PersistentDatabase::getMuteInfoByNickname");
                    return std::nullopt;
                }
            }
            catch (const sql::SQLException& e)
            {
                ::Utils::Logger::log("MariaDB exception: " + std::string(e.what()),Utils::LogType::Error,"PersistentDatabase::getMuteInfoByNickname");
                return std::nullopt;;
            }
        }

        std::optional<Main::Structures::BanInfo> PersistentDatabase::getBanInfoByNickname(const std::string& nickname)
        {
            try
            {
                std::unique_ptr<sql::PreparedStatement> stmt(m_con->prepareStatement("SELECT * FROM Users WHERE Nickname = ?"));
                stmt->setString(1, nickname);
                std::unique_ptr<sql::ResultSet> res(stmt->executeQuery());

                if (res->next())
                {
                    const std::string suspendedUntilStr = res->getString("SuspendedUntil").c_str();
                    const std::string suspensionReason = res->getString("SuspensionReason").c_str();

                    std::uint64_t suspendedUntilEpoch = Common::Utils::datetimeToEpoch(suspendedUntilStr);
                    std::uint64_t nowEpoch = static_cast<std::uint64_t>(std::chrono::duration_cast<std::chrono::seconds>(std::chrono::system_clock::now().time_since_epoch()).count());
                    bool isBanned = nowEpoch < suspendedUntilEpoch;
                    return Main::Structures::BanInfo{ isBanned, suspensionReason, suspendedUntilStr };
                }
                else
                {
                    ::Utils::Logger::log("No baninfo found for targetPlayer " + nickname, Utils::LogType::Warning, "PersistentDatabase::getBanInfoByNickname");
                    return std::nullopt;
                }
            }
            catch (const sql::SQLException& e)
            {
                ::Utils::Logger::log("MariaDB exception: " + std::string(e.what()), Utils::LogType::Error, "PersistentDatabase::getBanInfoByNickname");
                return std::nullopt;;
            }
        }

        std::optional<std::string> PersistentDatabase::getRoomCreationDisabledUntil(const std::string& nickname)
        {
            try
            {
                std::string selectQueryStr = "SELECT RoomCreationDisabledUntil FROM Users WHERE Nickname = ?";
                std::unique_ptr<sql::PreparedStatement> stmt(m_con->prepareStatement(selectQueryStr));
                stmt->setString(1, nickname);

                std::unique_ptr<sql::ResultSet> res(stmt->executeQuery());
                if (res->next())
                {
                    const std::string disabledUntilStr = res->getString("RoomCreationDisabledUntil").c_str();
                    std::uint64_t disabledUntilEpoch = Common::Utils::datetimeToEpoch(disabledUntilStr);
                    std::uint64_t nowEpoch = static_cast<std::uint64_t>(std::chrono::duration_cast<std::chrono::seconds>(std::chrono::system_clock::now().time_since_epoch()).count());

                    if (nowEpoch < disabledUntilEpoch)
                        return disabledUntilStr;

                    return std::nullopt;
                }

                return std::nullopt;
            }
            catch (const sql::SQLException& e)
            {
                ::Utils::Logger::log("MariaDB exception: " + std::string(e.what()), Utils::LogType::Error, "PersistentDatabase::getRoomCreationDisabledUntil");
                return std::nullopt;;
            }
        }

        bool PersistentDatabase::isRoomCreationDisabled(std::uint32_t playerID)
        {
            try
            {
                std::unique_ptr<sql::PreparedStatement> stmt(m_con->prepareStatement("SELECT RoomCreationDisabledUntil FROM Users WHERE AccountID = ?"));
                stmt->setUInt(1, playerID);
                std::unique_ptr<sql::ResultSet> res(stmt->executeQuery());

                if (res->next())
                {
                    const std::string disabledUntilStr = res->getString("RoomCreationDisabledUntil").c_str();
                    std::uint64_t disabledUntilEpoch = Common::Utils::datetimeToEpoch(disabledUntilStr);
                    std::uint64_t nowEpoch = static_cast<std::uint64_t>(
                        std::chrono::duration_cast<std::chrono::seconds>(
                            std::chrono::system_clock::now().time_since_epoch()
                        ).count()
                        );

                    return nowEpoch < disabledUntilEpoch;
                }

                ::Utils::Logger::log("No room creation ban info found for AccountID: " + std::to_string(playerID), Utils::LogType::Warning, "PersistentDatabase::isRoomCreationDisabled");
                return false;
            }
            catch (const sql::SQLException& e)
            {
                ::Utils::Logger::log("MariaDB exception: " + std::string(e.what()), Utils::LogType::Error, "PersistentDatabase::isRoomCreationDisabled");
                return false;
            }
        }
       
        std::optional<std::string> PersistentDatabase::getVotekickDisabledUntil(const std::string& nickname)
        {
            try
            {
                std::string selectQueryStr = "SELECT VotekickDisabledUntil FROM Users WHERE Nickname = ?";
                std::unique_ptr<sql::PreparedStatement> stmt(m_con->prepareStatement(selectQueryStr));
                stmt->setString(1, nickname);

                std::unique_ptr<sql::ResultSet> res(stmt->executeQuery());
                if (res->next())
                {
                    const std::string disabledUntilStr = res->getString("VotekickDisabledUntil").c_str();
                    std::uint64_t disabledUntilEpoch = Common::Utils::datetimeToEpoch(disabledUntilStr);
                    std::uint64_t nowEpoch = static_cast<std::uint64_t>(
                        std::chrono::duration_cast<std::chrono::seconds>(
                            std::chrono::system_clock::now().time_since_epoch()
                        ).count()
                        );

                    if (nowEpoch < disabledUntilEpoch)
                        return disabledUntilStr;

                    return std::nullopt;
                }
                return std::nullopt;
            }
            catch (const sql::SQLException& e)
            {
                ::Utils::Logger::log("MariaDB exception: " + std::string(e.what()), Utils::LogType::Error, "PersistentDatabase::getVotekickDisabledUntil");
                return std::nullopt;;
            }
        }

        bool PersistentDatabase::isVotekickDisabled(std::uint32_t playerID)
        {
            try
            {
                std::unique_ptr<sql::PreparedStatement> stmt(m_con->prepareStatement("SELECT VotekickDisabledUntil FROM Users WHERE AccountID = ?"));
                stmt->setUInt(1, playerID);

                std::unique_ptr<sql::ResultSet> res(stmt->executeQuery());
                if (res->next())
                {
                    const std::string disabledUntilStr = res->getString("VotekickDisabledUntil").c_str();
                    std::uint64_t disabledUntilEpoch = Common::Utils::datetimeToEpoch(disabledUntilStr);
                    std::uint64_t nowEpoch = static_cast<std::uint64_t>(
                        std::chrono::duration_cast<std::chrono::seconds>(
                            std::chrono::system_clock::now().time_since_epoch()
                        ).count()
                        );

                    return nowEpoch < disabledUntilEpoch;
                }
                return false;
            }
            catch (const sql::SQLException& e)
            {
                ::Utils::Logger::log("MariaDB exception: " + std::string(e.what()), Utils::LogType::Error, "PersistentDatabase::isVotekickDisabled");
                return false;
            }
        }

        bool PersistentDatabase::unbanPlayer(const std::string& nickname)
        {
            try
            {
                std::string queryStr = "UPDATE Users SET SuspendedUntil = 0, SuspensionReason = '' WHERE Nickname = ?";
                std::unique_ptr<sql::PreparedStatement> stmt(m_con->prepareStatement(queryStr));
                stmt->setString(1, nickname);

                if (stmt->executeUpdate() == 0)
                {
                    ::Utils::Logger::log("No rows changed for player: " + nickname, Utils::LogType::Warning, "PersistentDatabase::unbanPlayer");
                    return false;
                }
                return true;
            }
            catch (const sql::SQLException& e)
            {
                ::Utils::Logger::log("MariaDB exception: " + std::string(e.what()), Utils::LogType::Error, "PersistentDatabase::unbanPlayer");
                return false;
            }
        }

        bool PersistentDatabase::addPlayer(const std::string& username, const std::string& password, const std::string& nickname, const std::string& email,
            const std::string& secret2fa)
        {
            try
            {
                std::string queryStr = "INSERT INTO Users (Username, Password, Nickname, Email, Secret) VALUES (?, ?, ?, ?, ?)";
                std::unique_ptr<sql::PreparedStatement> stmt(m_con->prepareStatement(queryStr));
                stmt->setString(1, username);
                stmt->setString(2, password);
                stmt->setString(3, nickname);
                stmt->setString(4, email);
                stmt->setString(5, Common::Utils::SetupParser::getInstance().getAuthSetup().enhancedSecurity ? secret2fa : "");

                if (stmt->executeUpdate() == 0)
                {
                    ::Utils::Logger::log("Error in query execution: " + queryStr, Utils::LogType::Warning, "PersistentDatabase::addPlayer");
                    return false;
                }
                return true;
            }
            catch (const sql::SQLException& e)
            {
                ::Utils::Logger::log("MariaDB exception: " + std::string(e.what()), Utils::LogType::Error, "PersistentDatabase::addPlayer");
                return false;
            }
        }

        bool PersistentDatabase::playerExistsByNickname(const std::string& nickname)
        {
            try
            {
                std::unique_ptr<sql::PreparedStatement> stmt(m_con->prepareStatement("SELECT COUNT(*) FROM Users WHERE Nickname = ?"));
                stmt->setString(1, nickname);
                std::unique_ptr<sql::ResultSet> res(stmt->executeQuery());

                if (res->next())
                {
                    return res->getUInt(1) > 0;
                }
                return false;
            }
            catch (const sql::SQLException& e)
            {
                ::Utils::Logger::log("MariaDB exception: " + std::string(e.what()),Utils::LogType::Error,"PersistentDatabase::playerExistsByNickname");
                return false;
            }
        }

        bool PersistentDatabase::playerExistsByUsername(const std::string& username)
        {
            try
            {
                std::unique_ptr<sql::PreparedStatement> stmt(m_con->prepareStatement("SELECT COUNT(*) FROM Users WHERE Username = ?"));
                stmt->setString(1, username);
                std::unique_ptr<sql::ResultSet> res(stmt->executeQuery());

                if (res->next())
                {
                    return res->getUInt(1) > 0;
                }
                return false;
            }
            catch (const sql::SQLException& e)
            {
                ::Utils::Logger::log("MariaDB exception: " + std::string(e.what()), Utils::LogType::Error, "PersistentDatabase::playerExistsByUsername");
                return false;
            }
        }

        auto PersistentDatabase::getPlayerItems(std::uint32_t playerID)
            -> std::pair<std::vector<Main::Structures::Item>, std::unordered_map<std::uint16_t, std::vector<Main::Structures::EquippedItem>>>
        {
            std::vector<Main::Structures::Item> nonEquippedItems;
            std::unordered_map<std::uint16_t, std::vector<Main::Structures::EquippedItem>> equippedItemsPerCharacter;
            std::vector<std::pair<std::uint64_t, std::uint64_t>> itemNumbersToUpdate;
            std::uint64_t itemNum = 0;

            try
            {
                TransactionGuard tx(m_transactionalCon.get());

                const std::string queryStr = "SELECT * FROM UserItems WHERE AccountID = ?";
                std::unique_ptr<sql::PreparedStatement> stmt(m_transactionalCon->prepareStatement(queryStr));
                stmt->setUInt(1, playerID);
                std::unique_ptr<sql::ResultSet> resultSet(stmt->executeQuery());

                nonEquippedItems.reserve(1000);
                equippedItemsPerCharacter.reserve(500);

                while (resultSet->next())
                {
                    Main::Structures::Item item{ static_cast<std::uint32_t>(resultSet->getInt("ItemID")) };
                    auto rowId = static_cast<std::uint64_t>(resultSet->getUInt64("rowid"));
                    item.serialInfo.itemOrigin = static_cast<std::uint64_t>(resultSet->getInt64("ItemOrigin"));
                    item.serialInfo.m_serverId = static_cast<std::uint64_t>(resultSet->getInt64("acquisitionServerId"));
                    item.serialInfo.itemCreationDate = static_cast<time32_t>(resultSet->getInt64("creationDate"));

                    item.serialInfo.itemNumber = ++itemNum;
                    itemNumbersToUpdate.emplace_back(std::pair{ rowId, static_cast<unsigned long>(item.serialInfo.itemNumber) });

                    const std::uint64_t itemDuration_s = static_cast<std::uint64_t>(resultSet->getInt64("ItemDuration"));
                    item.expirationDate = (itemDuration_s <= 2)
                        ? static_cast<time32_t>(itemDuration_s)
                        : static_cast<time32_t>(item.serialInfo.itemCreationDate + itemDuration_s);

                    item.durability = static_cast<std::uint16_t>(resultSet->getInt("durability"));
                    item.energy = static_cast<std::uint16_t>(resultSet->getInt("energy"));
                    item.isSealed = static_cast<std::uint32_t>(resultSet->getInt("isSealed"));
                    item.sealLevel = static_cast<std::uint32_t>(resultSet->getInt("sealLevel"));
                    item.experienceEnhancement = static_cast<std::uint32_t>(resultSet->getInt("expEnhancement"));
                    item.mpEnhancement = static_cast<std::uint32_t>(resultSet->getInt("mpEnhancement"));
                    item.unknown = static_cast<std::uint32_t>(resultSet->getInt("IsCoupon"));
                    item.itemId.stock = static_cast<std::uint32_t>(resultSet->getInt("Stocks"));

                    if (resultSet->getInt("IsEquipped") == 1)
                    {
                        Main::Structures::EquippedItem equippedItem{ item };
                        const auto characterId = static_cast<std::uint16_t>(resultSet->getInt("CharacterID"));

                        if ((equippedItem.type >= 0 && equippedItem.type <= 17) || equippedItem.type == 19 || equippedItem.type == 20 /* diorama and scaffold */)
                        {
                            auto& equippedList = equippedItemsPerCharacter[characterId];
                            auto duplicateIt = std::find_if(equippedList.begin(), equippedList.end(),
                                [&](const Main::Structures::EquippedItem& existing) {
                                    return existing.type == equippedItem.type;
                                });

                            if (duplicateIt == equippedList.end())
                            {
                                equippedList.push_back(equippedItem);
                            }
                            else
                            {
                                Utils::Logger::log("Logic warning: Player with AID " + std::to_string(playerID) +
                                    " has multiple equipped items of type " + std::to_string(equippedItem.type) +
                                    " for CharacterID " + std::to_string(characterId) + ". Auto-fixing by unequipping one.",
                                    Utils::LogType::Warning, "PersistentDatabase::getPlayerItems");

                                nonEquippedItems.push_back(std::move(item));

                                const std::string updateQuery = "UPDATE UserItems SET IsEquipped = 0 WHERE rowid = ?";
                                std::unique_ptr<sql::PreparedStatement> updateStmt(m_transactionalCon->prepareStatement(updateQuery));
                                updateStmt->setUInt64(1, rowId);
                                updateStmt->executeUpdate();
                            }
                        }
                        else
                        {
                            Utils::Logger::log("Logic error: Player with AID " + std::to_string(playerID) +
                                " got an equipped item that is not allowed (itemId: " + std::to_string((uint32_t)equippedItem.id) + ", type: " + std::to_string((uint32_t)equippedItem.type)
                                + ")", Utils::LogType::Error, "PersistentDatabase::getPlayerItems");

                            nonEquippedItems.push_back(std::move(item));

                            const std::string updateQuery = "UPDATE UserItems SET IsEquipped = 0 WHERE rowid = ?";
                            std::unique_ptr<sql::PreparedStatement> updateStmt(m_transactionalCon->prepareStatement(updateQuery));
                            updateStmt->setUInt64(1, rowId);
                            updateStmt->executeUpdate();
                        }
                    }
                    else
                    {
                        nonEquippedItems.push_back(std::move(item));
                    }
                }

                if (!itemNumbersToUpdate.empty())
                {
                    std::string updateItemNumbersQuery = "UPDATE UserItems SET ItemNumber = ? WHERE rowid = ?";
                    std::unique_ptr<sql::PreparedStatement> updateStmt(m_transactionalCon->prepareStatement(updateItemNumbersQuery));

                    for (const auto& [rowId, itemNumber] : itemNumbersToUpdate)
                    {
                        updateStmt->setInt64(1, itemNumber);
                        updateStmt->setUInt64(2, rowId);
                        updateStmt->executeUpdate();
                        updateStmt->clearParameters();
                    }
                }

                tx.commit();
            }
            catch (const sql::SQLException& e)
            {
                ::Utils::Logger::log("MariaDB exception: " + std::string(e.what()), Utils::LogType::Error, "PersistentDatabase::getPlayerItems");
            }

            return std::pair{ std::move(nonEquippedItems), std::move(equippedItemsPerCharacter) };
        }

        bool PersistentDatabase::replaceItem(std::uint32_t accountID, std::uint64_t itemNumber, std::uint32_t newItemId)
        {
            try
            {
                const std::string queryStr = "UPDATE UserItems SET ItemID = ?, energy = 0 WHERE AccountID = ? AND ItemNumber = ?";

                std::unique_ptr<sql::PreparedStatement> stmt(m_con->prepareStatement(queryStr));
                stmt->setUInt(1, newItemId);
                stmt->setUInt(2, accountID);
                stmt->setUInt64(3, itemNumber); 

                if (stmt->executeUpdate() == 0)
                {
                    ::Utils::Logger::log("replaceItem: No rows affected. Possible reasons: item not found or already has the same ItemID. "
                        "AccountID: " + std::to_string(accountID) + ", ItemNumber: " + std::to_string(itemNumber) + ", NewItemID: " + std::to_string(newItemId),
                        Utils::LogType::Warning, "PersistentDatabase::replaceItem"); 
                    return false;
                }
                return true;
            }
            catch (const sql::SQLException& e)
            {
                ::Utils::Logger::log("MariaDB exception: " + std::string(e.what()) + " | AccountID: " + std::to_string(accountID) +
                    ", ItemNumber: " + std::to_string(itemNumber) + ", NewItemID: " + std::to_string(newItemId), 
                    Utils::LogType::Error, "PersistentDatabase::replaceItem");  
                return false;
            }
        }

        bool PersistentDatabase::replaceItemResetEnergy(std::uint32_t accountID, std::uint64_t itemNumber, std::uint32_t newItemId)
        {
            try
            {
                const std::string queryStr = "UPDATE UserItems SET ItemID = ?, energy = 0 WHERE AccountID = ? AND ItemNumber = ?";

                std::unique_ptr<sql::PreparedStatement> stmt(m_con->prepareStatement(queryStr));
                stmt->setUInt(1, newItemId);
                stmt->setUInt(2, accountID);
                stmt->setUInt64(3, itemNumber);

                if (stmt->executeUpdate() == 0)
                {
                    ::Utils::Logger::log("replaceItemResetEnergy: No rows affected. Possible reasons: item not found or already has the same ItemID. "
                        "AccountID: " + std::to_string(accountID) +
                        ", ItemNumber: " + std::to_string(itemNumber) +
                        ", NewItemID: " + std::to_string(newItemId),
                        Utils::LogType::Warning, "PersistentDatabase::replaceItemResetEnergy");
                    return false;
                }
                return true;
            }
            catch (const sql::SQLException& e)
            {
                ::Utils::Logger::log("MariaDB exception: " + std::string(e.what()) +
                    " | AccountID: " + std::to_string(accountID) +
                    ", ItemNumber: " + std::to_string(itemNumber) +
                    ", NewItemID: " + std::to_string(newItemId),
                    Utils::LogType::Error, "PersistentDatabase::replaceItemResetEnergy");
                return false;
            }
        }

        bool PersistentDatabase::addPlayerItems(std::uint32_t accountID, const std::vector<Item>& items, std::uint32_t latestCharacterSelected)
        {
            try
            {
                TransactionGuard tx(m_transactionalCon.get());

                const std::string queryStr =
                    "INSERT INTO UserItems (AccountID, IsEquipped, CharacterID, ItemID, ItemDuration, ItemNumber, ItemOrigin, acquisitionServerId, creationDate,"
                    " durability, energy, isSealed, sealLevel, expEnhancement, mpEnhancement, IsCoupon, Stocks) "
                    "VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?)";

                std::unique_ptr<sql::PreparedStatement> stmt(m_transactionalCon->prepareStatement(queryStr));
                for (const auto& item : items)
                {
                    stmt->setUInt(1, accountID);
                    stmt->setUInt(2, latestCharacterSelected == -1 ? 0 : 1);
                    stmt->setUInt(3, latestCharacterSelected == -1 ? 0 : latestCharacterSelected);
                    stmt->setUInt(4, item.itemId.itemId);
                    stmt->setInt64(5, static_cast<std::int64_t>(item.expirationDate <= 3 ? item.expirationDate : item.expirationDate - item.serialInfo.itemCreationDate));
                    stmt->setInt64(6, static_cast<std::int64_t>(item.serialInfo.itemNumber));
                    stmt->setInt64(7, static_cast<std::int64_t>(item.serialInfo.itemOrigin));
                    stmt->setInt64(8, static_cast<std::int64_t>(item.serialInfo.m_serverId));
                    stmt->setInt64(9, static_cast<std::int64_t>(item.serialInfo.itemCreationDate));
                    stmt->setUInt(10, static_cast<std::uint32_t>(item.durability));
                    stmt->setUInt(11, static_cast<std::uint32_t>(item.energy));
                    stmt->setUInt(12, 0); // isSealed
                    stmt->setUInt(13, 0); // sealLevel
                    stmt->setUInt(14, 0); // expEnhancement
                    stmt->setUInt(15, 0); // mpEnhancement
                    stmt->setUInt(16, 0); // unknown
                    stmt->setUInt(17, item.itemId.stock);
                    stmt->executeUpdate();
                }
                tx.commit();
                return true;
            }
            catch (const sql::SQLException& e)
            {
                ::Utils::Logger::log("MariaDB exception: " + std::string(e.what()), Utils::LogType::Error, "PersistentDatabase::addPlayerItems");
                return false;
            }
        }

        bool PersistentDatabase::updateItemStock(std::uint32_t accountID, std::uint64_t itemNumber, std::uint32_t newStock)
        {
            try
            {
                const std::string queryStr = "UPDATE UserItems SET Stocks = ? WHERE AccountID = ? AND ItemNumber = ?";
                std::unique_ptr<sql::PreparedStatement> stmt(m_con->prepareStatement(queryStr));
                stmt->setUInt(1, newStock);     
                stmt->setUInt(2, accountID);      
                stmt->setInt64(3, static_cast<std::int64_t>(itemNumber)); 
                stmt->executeUpdate();
                return true;
            }
            catch (const sql::SQLException& e)
            {
                ::Utils::Logger::log("MariaDB exception: " + std::string(e.what()), Utils::LogType::Error, "PersistentDatabase::updateItemStock");
                return false;
            }
        }

        void PersistentDatabase::prolongItems(std::uint32_t accountID, const std::vector<Main::Structures::BoughtItemToProlong>& toProlongItems,
            const std::vector<std::uint64_t>& itemDurations, std::uint32_t timeNow)
        {
            try
            {
                TransactionGuard tg(m_transactionalCon.get());

                const std::string prolongItemQuery = "UPDATE UserItems SET ItemDuration = ?, ItemOrigin = ?, acquisitionServerId = ? "
                    "WHERE AccountID = ? AND ItemNumber = ?";

                std::unique_ptr<sql::PreparedStatement> stmt(m_transactionalCon->prepareStatement(prolongItemQuery));

                for (std::size_t i = 0; i < toProlongItems.size(); ++i)
                {
                    const auto& itemToProlong = toProlongItems[i];
                    stmt->setInt64(1, static_cast<std::int64_t>(itemDurations[i] <= 3 ? itemDurations[i] : itemDurations[i] + timeNow));
                    stmt->setInt64(2, static_cast<std::int64_t>(itemToProlong.serialInfo.itemOrigin));
                    stmt->setInt64(3, static_cast<std::int64_t>(itemToProlong.serialInfo.m_serverId));
                    stmt->setInt64(4, static_cast<std::int64_t>(accountID));
                    stmt->setInt(5, static_cast<std::int32_t>(itemToProlong.serialInfo.itemNumber));

                    if (stmt->executeUpdate() == 0)
                    {
                        ::Utils::Logger::log("Failed to update item with ItemNumber: " + std::to_string(itemToProlong.serialInfo.itemNumber) +
                            " for AccountID: " + std::to_string(accountID), Utils::LogType::Warning, "PersistentDatabase::prolongItems");

                        return; 
                    }
                }

                tg.commit(); 
            }
            catch (const sql::SQLException& e)
            {
                ::Utils::Logger::log("MariaDB exception: " + std::string(e.what()), Utils::LogType::Error, "PersistentDatabase::prolongItems");
                return;
            }
        }

        bool PersistentDatabase::removePlayerItem(std::uint32_t accountId, std::uint64_t itemNumber, const std::string& caller)
        {
            try
            {
                const std::string deleteItem = "DELETE FROM UserItems WHERE AccountID = ? AND ItemNumber = ?";
                std::unique_ptr<sql::PreparedStatement> stmt(m_con->prepareStatement(deleteItem));

                stmt->setUInt(1, accountId);
                stmt->setUInt64(2, itemNumber);

                if (stmt->executeUpdate() == 0)
                {
                    ::Utils::Logger::log("[CALLER: " + caller + "] Warning: No rows affected by query for AccountID: " + std::to_string(accountId) +
                        " and ItemNumber: " + std::to_string(itemNumber), Utils::LogType::Warning, "PersistentDatabase::removePlayerItem");
                    return false;
                }

                return true;
            }
            catch (const sql::SQLException& e)
            {
                ::Utils::Logger::log("[Main::Database::removePlayerItem] MariaDB exception: " + std::string(e.what()) +
                    " | AccountID: " + std::to_string(accountId) +
                    ", ItemNumber: " + std::to_string(itemNumber),
                    Utils::LogType::Error, "PersistentDatabase::removePlayerItem");
                return false;
            }
        }

        bool PersistentDatabase::removeAllPlayerItems(const std::string& nickname, std::uint32_t executorGrade)
        {
            try
            {
                const std::string checkQuery = "SELECT AccountID, Grade FROM Users WHERE Nickname = ?";
                std::unique_ptr<sql::PreparedStatement> checkStmt(m_con->prepareStatement(checkQuery));
                checkStmt->setString(1, nickname);

                std::unique_ptr<sql::ResultSet> res(checkStmt->executeQuery());
                if (!res->next())
                {
                    return false;
                }
                if (res->getInt("Grade") > static_cast<int>(executorGrade))
                {
                    return false;
                }

                const std::uint32_t accountId = res->getUInt("AccountID");
                const std::string deleteItems = "DELETE FROM UserItems WHERE AccountID = ?";
                std::unique_ptr<sql::PreparedStatement> stmt(m_con->prepareStatement(deleteItems));
                stmt->setUInt(1, accountId);
                stmt->executeUpdate();
                return true;
            }
            catch (const sql::SQLException& e)
            {
                ::Utils::Logger::log("[Main::Database::removeAllPlayerItems] MariaDB exception: " + std::string(e.what()) +
                    " | Nickname: " + nickname,
                    Utils::LogType::Error, "PersistentDatabase::removeAllPlayerItems");
                return false;
            }
        }

        bool PersistentDatabase::setPlayerLevelByName(const std::string& nickname, std::uint16_t level, std::uint32_t experience, std::uint32_t executorGrade)
        {
            try
            {
                const std::string checkQuery = "SELECT AccountID, Grade FROM Users WHERE Nickname = ?";
                std::unique_ptr<sql::PreparedStatement> checkStmt(m_con->prepareStatement(checkQuery));
                checkStmt->setString(1, nickname);

                std::unique_ptr<sql::ResultSet> res(checkStmt->executeQuery());
                if (!res->next())
                {
                    return false;
                }
                if (res->getInt("Grade") > static_cast<int>(executorGrade))
                {
                    return false;
                }

                const std::uint32_t accountId = res->getUInt("AccountID");
                const std::string updateQuery = "UPDATE Users SET Level = ?, Experience = ? WHERE AccountID = ?";
                std::unique_ptr<sql::PreparedStatement> stmt(m_con->prepareStatement(updateQuery));
                stmt->setUInt(1, level);
                stmt->setUInt(2, experience);
                stmt->setUInt(3, accountId);
                stmt->executeUpdate();
                return true;
            }
            catch (const sql::SQLException& e)
            {
                ::Utils::Logger::log("[Main::Database::setPlayerLevelByName] MariaDB exception: " + std::string(e.what()) +
                    " | Nickname: " + nickname,
                    Utils::LogType::Error, "PersistentDatabase::setPlayerLevelByName");
                return false;
            }
        }

        bool PersistentDatabase::setCurrencyByName(const std::string& nickname, std::uint32_t rockTotens, std::uint32_t microPoints, std::uint16_t coins, std::uint32_t executorGrade)
        {
            try
            {
                const std::string checkQuery = "SELECT AccountID, Grade FROM Users WHERE Nickname = ?";
                std::unique_ptr<sql::PreparedStatement> checkStmt(m_con->prepareStatement(checkQuery));
                checkStmt->setString(1, nickname);

                std::unique_ptr<sql::ResultSet> res(checkStmt->executeQuery());
                if (!res->next())
                {
                    return false;
                }
                if (res->getInt("Grade") > static_cast<int>(executorGrade))
                {
                    return false;
                }

                const std::uint32_t accountId = res->getUInt("AccountID");
                const std::string updateQuery = "UPDATE Users SET RockTotens = ?, MicroPoints = ?, Coins = ? WHERE AccountID = ?";
                std::unique_ptr<sql::PreparedStatement> stmt(m_con->prepareStatement(updateQuery));
                stmt->setUInt(1, rockTotens);
                stmt->setUInt(2, microPoints);
                stmt->setUInt(3, coins);
                stmt->setUInt(4, accountId);
                stmt->executeUpdate();
                return true;
            }
            catch (const sql::SQLException& e)
            {
                ::Utils::Logger::log("[Main::Database::setCurrencyByName] MariaDB exception: " + std::string(e.what()) +
                    " | Nickname: " + nickname,
                    Utils::LogType::Error, "PersistentDatabase::setCurrencyByName");
                return false;
            }
        }

        void PersistentDatabase::updatePlayerLevel(std::uint32_t accountID, std::uint16_t level)
        {
            try
            {
                std::string updateLevelQuery = "UPDATE Users SET Level = ? WHERE AccountID = ?";
                std::unique_ptr<sql::PreparedStatement> stmt(m_con->prepareStatement(updateLevelQuery));

                stmt->setUInt(1, level);
                stmt->setUInt(2, accountID);

                if (stmt->executeUpdate() == 0)
                {
                    ::Utils::Logger::log("No rows changed with query", Utils::LogType::Warning, "PersistentDatabase::updatePlayerLevel");
                    return;
                }
            }
            catch (const sql::SQLException& e)
            {
                ::Utils::Logger::log(std::string("MariaDB exception: ") + e.what(), Utils::LogType::Error, "PersistentDatabase::updatePlayerLevel");
                return;
            }
        }

        void PersistentDatabase::updatePlayerExperience(std::uint32_t accountID, std::uint32_t exp)
        {
            try
            {
                std::string updateLevelQuery = "UPDATE Users SET Experience = ? WHERE AccountID = ?";
                std::unique_ptr<sql::PreparedStatement> stmt(m_con->prepareStatement(updateLevelQuery));

                stmt->setUInt(1, exp);
                stmt->setUInt(2, accountID);

                if (stmt->executeUpdate() == 0)
                {
                    ::Utils::Logger::log("No rows changed with query", Utils::LogType::Warning, "PersistentDatabase::updatePlayerExperience");
                    return;
                }
            }
            catch (const sql::SQLException& e)
            {
                ::Utils::Logger::log(std::string("MariaDB exception: ") + e.what(), Utils::LogType::Error, "PersistentDatabase::updatePlayerExperience");
                return;
            }
        }

        std::expected<bool, std::string> PersistentDatabase::updatePlayerName(std::uint32_t accountID, const char* name, bool isStaff)
        {
            try
            {
                std::unique_ptr<sql::PreparedStatement> checkDuplicateStmt(m_con->prepareStatement(
                    "SELECT AccountID FROM Users WHERE Nickname = ? AND AccountID != ?"));
                checkDuplicateStmt->setString(1, name);
                checkDuplicateStmt->setUInt(2, accountID);

                std::unique_ptr<sql::ResultSet> duplicateResult(checkDuplicateStmt->executeQuery());

                if (duplicateResult->next())
                {
                    return std::unexpected("There's already a player with this nickname");
                }

                if (!isStaff)
                {
                    std::unique_ptr<sql::PreparedStatement> checkStmt(m_con->prepareStatement(
                        "SELECT CanUpdateNickname FROM Users WHERE AccountID = ?"));
                    checkStmt->setUInt(1, accountID);
                    std::unique_ptr<sql::ResultSet> result(checkStmt->executeQuery());

                    if (!result->next())
                    {
                        return std::unexpected("User not found");
                    }

                    if (!result->getBoolean("CanUpdateNickname"))
                    {
                        return std::unexpected("You cannot change your nickname right now");
                    }
                }

                std::string updateNameQuery = "UPDATE Users SET Nickname = ? WHERE AccountID = ?";
                std::unique_ptr<sql::PreparedStatement> stmt(m_con->prepareStatement(updateNameQuery));

                stmt->setString(1, name);
                stmt->setUInt(2, accountID);

                if (stmt->executeUpdate() == 0)
                {
                    ::Utils::Logger::log("No rows changed with query", Utils::LogType::Warning, "PersistentDatabase::updatePlayerName");
                    return std::unexpected("Failed to update nickname");
                }

                if (!isStaff)
                {
                    std::unique_ptr<sql::PreparedStatement> resetStmt(m_con->prepareStatement(
                        "UPDATE Users SET CanUpdateNickname = 0 WHERE AccountID = ?"));
                    resetStmt->setUInt(1, accountID);
                    resetStmt->executeUpdate();
                }
            }
            catch (const sql::SQLException& e)
            {
                ::Utils::Logger::log(std::string("MariaDB exception: ") + e.what(), Utils::LogType::Error, "PersistentDatabase::updatePlayerName");
                return std::unexpected("Database error occurred");
            }

            return true; 
        }

        bool PersistentDatabase::updateSuspension(const std::string& nickname, const std::string& until, const std::string& reason, std::uint32_t executorGrade)
        {
            try
            {
                TransactionGuard tg(m_transactionalCon.get());

                std::string checkGradeQuery = "SELECT Grade FROM Users WHERE Nickname = ?";
                std::unique_ptr<sql::PreparedStatement> checkStmt(m_transactionalCon->prepareStatement(checkGradeQuery));
                checkStmt->setString(1, nickname);

                std::unique_ptr<sql::ResultSet> res(checkStmt->executeQuery());
                if (!res->next())
                {
                    return false;
                }

                if (res->getInt(1) >= static_cast<int>(executorGrade))
                {
                    return false;
                }

                std::string updateQuery = "UPDATE Users SET SuspendedUntil = ?, SuspensionReason = ? WHERE Nickname = ?";
                std::unique_ptr<sql::PreparedStatement> updateStmt(m_transactionalCon->prepareStatement(updateQuery));
                updateStmt->setString(1, until);
                updateStmt->setString(2, reason);
                updateStmt->setString(3, nickname);

                if (updateStmt->executeUpdate() == 0)
                {
                    ::Utils::Logger::log("Error executing query: " + updateQuery, Utils::LogType::Warning, "PersistentDatabase::updateSuspension");
                    return false;
                }

                tg.commit();
            }
            catch (const sql::SQLException& e)
            {
                ::Utils::Logger::log(std::string("MariaDB exception: ") + e.what(), Utils::LogType::Error, "PersistentDatabase::updateSuspension");
                return false;
            }
            return true;
        }

        void PersistentDatabase::updateLatestRewardDay(const std::string& columnName, std::uint32_t accountId, const std::string& rewardDay)
        {
            try
            {
                std::string queryStr = "UPDATE Users SET " + columnName + " = ? WHERE AccountID = ?";
                std::unique_ptr<sql::PreparedStatement> stmt(m_con->prepareStatement(queryStr));

                stmt->setString(1, rewardDay);
                stmt->setUInt(2, accountId);

                if (stmt->executeUpdate() == 0)
                {
                    ::Utils::Logger::log("Error executing query: " + queryStr, Utils::LogType::Warning, "PersistentDatabase::updateLatestRewardDay");
                    return;
                }
            }
            catch (const sql::SQLException& e)
            {
                ::Utils::Logger::log(std::string("MariaDB exception: ") + e.what(), Utils::LogType::Error, "PersistentDatabase::updateLatestRewardDay");
                return;
            }
        }

        std::string PersistentDatabase::getLatestRewardDayFor(const std::string& columnName, std::uint32_t accountId)
        {
            try
            {
                const std::string queryStr = "SELECT " + columnName + " FROM Users WHERE AccountID = ?";
                std::unique_ptr<sql::PreparedStatement> stmt(m_con->prepareStatement(queryStr));

                stmt->setUInt(1, accountId);
                std::unique_ptr<sql::ResultSet> res(stmt->executeQuery());

                if (res->next())
                {
                    return res->getString(1).c_str();
                }
                else
                {
                    return "";
                }
            }
            catch (const sql::SQLException& e)
            {
                ::Utils::Logger::log(std::string("MariaDB exception: ") + e.what(), Utils::LogType::Error, "PersistentDatabase::getLatestRewardDayFor");
                return "";
            }
        }

        bool PersistentDatabase::mustRewardsBeUpdated(const std::string& tableName, std::uint32_t daysToCheck)
        {
            try
            {
                std::string queryStr = "SELECT Date FROM " + tableName + " ORDER BY Date DESC LIMIT 1";
                std::unique_ptr<sql::PreparedStatement> stmt(m_con->prepareStatement(queryStr));
                std::unique_ptr<sql::ResultSet> res(stmt->executeQuery());

                if (res->next())
                {
                    std::string dateStr = res->getString("Date").c_str();

                    std::istringstream ss(dateStr);
                    int year, month, day;
                    char char1, char2;
                    ss >> year >> char1 >> month >> char2 >> day;

                    if (ss.fail() || char1 != '-' || char2 != '-' || month < 1 || month > 12 || day < 1 || day > 31)
                    {
                        return false;
                    }

                    std::chrono::year_month_day parsedDate = std::chrono::year_month_day(std::chrono::year(year), std::chrono::month(month), std::chrono::day(day));
                    auto todayTimestamp = std::chrono::floor<std::chrono::days>(std::chrono::system_clock::now());
                    return std::chrono::duration_cast<std::chrono::days>(todayTimestamp - std::chrono::sys_days(parsedDate)).count() >= daysToCheck;
                }
                else
                {
                    return true;
                }
            }
            catch (const sql::SQLException& e)
            {
                ::Utils::Logger::log("MariaDB exception: " + std::string(e.what()), Utils::LogType::Error, "PersistentDatabase::mustRewardsBeUpdated");
                return false;
            }
        }

        void PersistentDatabase::updateRewards(const std::string& tableName, const std::vector<std::uint32_t>& items)
        {
            using std::chrono::system_clock;
            const auto today = std::format("{:%F}", std::chrono::floor<std::chrono::days>(system_clock::now()));

            try
            {
                TransactionGuard tg(m_transactionalCon.get());

                std::string countQueryStr = "SELECT COUNT(*) FROM " + tableName;
                std::unique_ptr<sql::PreparedStatement> countStmt(m_transactionalCon->prepareStatement(countQueryStr));
                std::unique_ptr<sql::ResultSet> countRes(countStmt->executeQuery());

                if (countRes->next() && countRes->getInt(1) > 0)
                {
                    std::string deleteQueryStr = "DELETE FROM " + tableName;
                    std::unique_ptr<sql::PreparedStatement> deleteStmt(m_transactionalCon->prepareStatement(deleteQueryStr));
                    if (deleteStmt->executeUpdate() == 0)
                    {
                        return;
                    }
                }

                std::string insertQueryStr = "INSERT INTO " + tableName + " (ItemID, Date) VALUES (?, ?)";
                std::unique_ptr<sql::PreparedStatement> insertStmt(m_transactionalCon->prepareStatement(insertQueryStr));

                for (const auto& item : items)
                {
                    insertStmt->setUInt(1, item);
                    insertStmt->setString(2, today);
                    if (insertStmt->executeUpdate() == 0)
                    {
                        return;
                    }
                }

                tg.commit();
            }
            catch (const sql::SQLException& e)
            {
                ::Utils::Logger::log("MariaDB exception: " + std::string(e.what()), Utils::LogType::Error, "PersistentDatabase::updateRewards");
                return;
            }
        }

        void PersistentDatabase::updateBattery(std::uint32_t accountId, std::uint32_t newBattery)
        {
            try
            {
                std::string updateQueryStr = "UPDATE Users SET Battery = ? WHERE AccountID = ?";
                std::unique_ptr<sql::PreparedStatement> stmt(m_con->prepareStatement(updateQueryStr));

                stmt->setUInt(1, newBattery);
                stmt->setUInt(2, accountId);

                if (stmt->executeUpdate() == 0)
                {
                    ::Utils::Logger::log("Update failed.", Utils::LogType::Warning, "PersistentDatabase::updateBattery");
                    return;
                }
            }
            catch (const sql::SQLException& e)
            {
                ::Utils::Logger::log("MariaDB exception: " + std::string(e.what()), Utils::LogType::Error, "PersistentDatabase::updateBattery");
                return;
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

        void PersistentDatabase::reduceDurability(std::uint32_t accountId, const std::vector<std::pair<std::uint32_t, std::uint64_t>>& equippedItems)
        {
            try
            {
                TransactionGuard tg(m_transactionalCon.get());

                std::string updateQueryStr = "UPDATE UserItems SET Durability = ? WHERE AccountID = ? AND ItemNumber = ? AND ItemDuration = 0";
                std::unique_ptr<sql::PreparedStatement> stmt(m_transactionalCon->prepareStatement(updateQueryStr));

                for (const auto& currentEquippedItem : equippedItems)
                {
                    const auto currentItemBaseDurability = Main::CdbUtils::getItemDurability(currentEquippedItem.first);

                    if (!currentItemBaseDurability || *currentItemBaseDurability == 0)
                        continue;

                    const std::uint32_t reduction = (*currentItemBaseDurability / 100) * 1;

                    const std::uint32_t newDurability = (*currentItemBaseDurability > reduction)
                        ? (*currentItemBaseDurability - reduction)
                        : 0;

                    stmt->setUInt(1, newDurability);
                    stmt->setUInt(2, accountId);
                    stmt->setUInt(3, currentEquippedItem.second);
                    stmt->executeUpdate();
                }

                tg.commit();
            }
            catch (const sql::SQLException& e)
            {
                ::Utils::Logger::log(std::string("SQL Error: ") + e.what(), Utils::LogType::Error, "PersistentDatabase::reduceDurability");
                return;
            }
        }

        void PersistentDatabase::updateItemDurability(std::uint32_t accountId, std::uint32_t itemNumber, std::uint32_t newDurability)
        {
            try
            {
                const std::string updateQueryStr =
                    "UPDATE UserItems SET Durability = ? WHERE AccountID = ? AND ItemNumber = ?";

                std::unique_ptr<sql::PreparedStatement> stmt(m_con->prepareStatement(updateQueryStr));

                stmt->setUInt(1, newDurability);
                stmt->setUInt(2, accountId);
                stmt->setUInt(3, itemNumber);
                stmt->executeUpdate();
            }
            catch (const sql::SQLException& e)
            {
                ::Utils::Logger::log(std::string("SQL Error: ") + e.what(), Utils::LogType::Error, "PersistentDatabase::updateItemDurability");
                return;
            }
        }

        void PersistentDatabase::updatePlayerStats(std::uint32_t accountId, const Main::Structures::AccountInfo& updatedAccountInfo)
        {
            try
            {
                std::string updateQueryStr = "UPDATE Users SET MeleeKills = ?, RifleKills = ?, ShotgunKills = ?, SniperKills = ?, GatlingKills = ?, "
                    "BazookaKills = ?, GrenadeKills = ?, HighestKillstreak = ?, Kills = ?, Deaths = ?, Headshots = ?, Assists = ?, "
                    "Experience = ?, MicroPoints = ?, Wins = ?, Loses = ?, Draws = ?, Level = ?, ZombieKills = ?, InfectedKills = ?, "
                    "ClanKills = ?, ClanDeaths = ?, ClanAssists = ?, ClanWins = ?, ClanLoses = ?, ClanDraws = ?, ClanContribution = ?, "
                    "HighestSinglewaveScore = ?, HighestSinglewaveStage = ?, HasFinishedTutorial = ?, Playtime = ? WHERE AccountID = ? ";

                std::unique_ptr<sql::PreparedStatement> stmt(m_con->prepareStatement(updateQueryStr));

                stmt->setUInt(1, updatedAccountInfo.meleeKills);
                stmt->setUInt(2, updatedAccountInfo.rifleKills);
                stmt->setUInt(3, updatedAccountInfo.shotgunKills);
                stmt->setUInt(4, updatedAccountInfo.sniperKills);
                stmt->setUInt(5, updatedAccountInfo.microgunKills);
                stmt->setUInt(6, updatedAccountInfo.bazookaKills);
                stmt->setUInt(7, updatedAccountInfo.grenadeKills);
                stmt->setUInt(8, static_cast<std::uint32_t>(updatedAccountInfo.killstreak));
                stmt->setUInt(9, updatedAccountInfo.totalKills);
                stmt->setUInt(10, updatedAccountInfo.deaths);
                stmt->setUInt(11, static_cast<std::uint32_t>(updatedAccountInfo.headshots));
                stmt->setUInt(12, updatedAccountInfo.assists);
                stmt->setUInt(13, updatedAccountInfo.experience);
                stmt->setUInt(14, static_cast<std::uint32_t>(updatedAccountInfo.microPoints));
                stmt->setUInt(15, updatedAccountInfo.wins);
                stmt->setUInt(16, updatedAccountInfo.losses);
                stmt->setUInt(17, updatedAccountInfo.draws);
                stmt->setUInt(18, static_cast<std::uint32_t>(updatedAccountInfo.playerLevel - 1));
                stmt->setUInt(19, updatedAccountInfo.zombieKills); 
                stmt->setUInt(20, updatedAccountInfo.infected);    
                stmt->setUInt(21, updatedAccountInfo.clanKills);
                stmt->setUInt(22, updatedAccountInfo.clanDeaths);
                stmt->setUInt(23, updatedAccountInfo.clanAssists);
                stmt->setUInt(24, updatedAccountInfo.clanWins);
                stmt->setUInt(25, updatedAccountInfo.clanLosses);
                stmt->setUInt(26, updatedAccountInfo.clanDraws);
                stmt->setUInt(27, updatedAccountInfo.clanContribution);
                stmt->setUInt(28, updatedAccountInfo.highestSingleWaveScore);
                stmt->setUInt(29, updatedAccountInfo.highestSinglewaveStage);
                stmt->setUInt(30, updatedAccountInfo.isTutorialDone);
                stmt->setUInt(31, updatedAccountInfo.playtime); 
                stmt->setUInt(32, accountId); 


                if (stmt->executeUpdate() == 0)
                {
                    ::Utils::Logger::log("Update failed.", Utils::LogType::Warning, "PersistentDatabase::updatePlayerStats");
                }
            }
            catch (const sql::SQLException& e)
            {
                ::Utils::Logger::log("MariaDB exception: " + std::string(e.what()), Utils::LogType::Error, "PersistentDatabase::updatePlayerStats");
                return;
            }
        }

        bool PersistentDatabase::updateMute(const std::string& nickname, const std::string& until,const std::string& reason, const std::string& mutedBy,
            std::uint32_t executorGrade)
        {
            try
            {
                TransactionGuard tg(m_transactionalCon.get());

                std::string checkGradeQuery = "SELECT Grade FROM Users WHERE Nickname = ?";
                std::unique_ptr<sql::PreparedStatement> checkStmt(m_transactionalCon->prepareStatement(checkGradeQuery));
                checkStmt->setString(1, nickname);

                std::unique_ptr<sql::ResultSet> resultSet(checkStmt->executeQuery());  

                if (!resultSet->next())
                {
                    return false; 
                }

                const int targetPlayerGrade = resultSet->getInt("Grade");
                if (targetPlayerGrade >= executorGrade)
                {
                    return false;  
                }

                std::string updateQuery = "UPDATE Users SET MutedUntil = ?, MuteReason = ?, MutedBy = ? WHERE Nickname = ?";
                std::unique_ptr<sql::PreparedStatement> updateStmt(m_transactionalCon->prepareStatement(updateQuery));
                updateStmt->setString(1, until);
                updateStmt->setString(2, reason);
                updateStmt->setString(3, mutedBy);
                updateStmt->setString(4, nickname);

                if (updateStmt->executeUpdate() == 0)
                {
                    ::Utils::Logger::log("No rows updated in updateMute", Utils::LogType::Warning, "PersistentDatabase::updateMute");
                    return false;
                }

                tg.commit();
                return true;
            }
            catch (const sql::SQLException& e)
            {
                ::Utils::Logger::log("MariaDB exception: " + std::string(e.what()),
                    Utils::LogType::Error, "PersistentDatabase::updateMute");
                return false;
            }
        }

        bool PersistentDatabase::updateVotekickDisabledUntil(const std::string& nickname, const std::string& until)
        {
            try
            {
                std::string updateQuery = "UPDATE Users SET VotekickDisabledUntil = ? WHERE Nickname = ?";
                std::unique_ptr<sql::PreparedStatement> updateStmt(m_con->prepareStatement(updateQuery));
                updateStmt->setString(1, until);
                updateStmt->setString(2, nickname);

                if (updateStmt->executeUpdate() == 0)
                {
                    ::Utils::Logger::log("Update failed: No rows affected.", Utils::LogType::Warning, "PersistentDatabase::updateVotekickDisabledUntil");
                    return false;
                }

                return true;
            }
            catch (const sql::SQLException& e)
            {
                ::Utils::Logger::log("MariaDB exception: " + std::string(e.what()), Utils::LogType::Error, "PersistentDatabase::updateVotekickDisabledUntil");
                return false;
            }
        }

        bool PersistentDatabase::resetVotekickDisabledUntil(const std::string& nickname)
        {
            try
            {
                TransactionGuard tg(m_transactionalCon.get());

                std::string selectQueryStr = "SELECT AccountID FROM Users WHERE Nickname = ?";
                std::unique_ptr<sql::PreparedStatement> selectStmt(m_transactionalCon->prepareStatement(selectQueryStr));
                selectStmt->setString(1, nickname);

                std::unique_ptr<sql::ResultSet> resultSet(selectStmt->executeQuery());
                if (!resultSet->next())
                {
                    return false;
                }

                const std::uint32_t accountID = resultSet->getUInt("AccountID");
                std::string updateQueryStr = "UPDATE Users SET VotekickDisabledUntil = 0 WHERE AccountID = ?";
                std::unique_ptr<sql::PreparedStatement> updateStmt(m_transactionalCon->prepareStatement(updateQueryStr));
                updateStmt->setUInt(1, accountID);

                if (updateStmt->executeUpdate() == 0)
                {
                    return false;
                }

                tg.commit();
                return true;
            }
            catch (const sql::SQLException& e)
            {
                ::Utils::Logger::log("MariaDB exception: " + std::string(e.what()), Utils::LogType::Error, "PersistentDatabase::resetVotekickDisabledUntil");
                return false;
            }
        }

        bool PersistentDatabase::updateRoomCreationDisabledUntil(const std::string& nickname, const std::string& until)
        {
            try
            {
                std::string updateQuery = "UPDATE Users SET RoomCreationDisabledUntil = ? WHERE Nickname = ?";
                std::unique_ptr<sql::PreparedStatement> updateStmt(m_con->prepareStatement(updateQuery));
                updateStmt->setString(1, until);
                updateStmt->setString(2, nickname);

                if (updateStmt->executeUpdate() == 0)
                {
                    ::Utils::Logger::log("Update failed: No rows affected.", Utils::LogType::Warning, "PersistentDatabase::updateRoomCreationDisabledUntil");
                    return false;
                }

                return true;
            }
            catch (const sql::SQLException& e)
            {
                ::Utils::Logger::log("MariaDB exception: " + std::string(e.what()), Utils::LogType::Error, "PersistentDatabase::updateRoomCreationDisabledUntil");
                return false;;
            }
        }

        bool PersistentDatabase::resetRoomCreationDisabledUntil(const std::string& nickname)
        {
            try
            {
                TransactionGuard tg(m_transactionalCon.get());

                std::string selectQueryStr = "SELECT AccountID FROM Users WHERE Nickname = ?";
                std::unique_ptr<sql::PreparedStatement> selectStmt(m_transactionalCon->prepareStatement(selectQueryStr));
                selectStmt->setString(1, nickname);

                std::unique_ptr<sql::ResultSet> resultSet(selectStmt->executeQuery());
                if (!resultSet->next())
                {
                    return false;
                }

                const std::uint32_t accountID = resultSet->getUInt("AccountID");
                std::string updateQueryStr = "UPDATE Users SET RoomCreationDisabledUntil = 0 WHERE AccountID = ?";
                std::unique_ptr<sql::PreparedStatement> updateStmt(m_transactionalCon->prepareStatement(updateQueryStr));
                updateStmt->setUInt(1, accountID);

                if (updateStmt->executeUpdate() == 0)
                {
                    return false;
                }

                tg.commit();
                return true;
            }
            catch (const sql::SQLException& e)
            {
                ::Utils::Logger::log("MariaDB exception: " + std::string(e.what()), Utils::LogType::Error, "PersistentDatabase::resetRoomCreationDisabledUntil");
                return false;;
            }
        }

        bool PersistentDatabase::unmuteAccount(const std::string& nickname)
        {
            try
            {
                TransactionGuard tg(m_transactionalCon.get());

                std::string selectQueryStr = "SELECT AccountID FROM Users WHERE Nickname = ?";
                std::unique_ptr<sql::PreparedStatement> selectStmt(m_transactionalCon->prepareStatement(selectQueryStr));
                selectStmt->setString(1, nickname);

                std::unique_ptr<sql::ResultSet> resultSet(selectStmt->executeQuery());
                if (!resultSet->next())
                {
                    return false;
                }

                const std::uint32_t accountID = resultSet->getUInt("AccountID");
                std::string updateQueryStr = "UPDATE Users SET MutedUntil = '1970-01-01 00:00:00' WHERE AccountID = ?";
                std::unique_ptr<sql::PreparedStatement> updateStmt(m_transactionalCon->prepareStatement(updateQueryStr));
                updateStmt->setUInt(1, accountID);

                if (updateStmt->executeUpdate() == 0)
                {
                    return false;
                }

                tg.commit();
                return true;
            }
            catch (const sql::SQLException& e)
            {
                ::Utils::Logger::log("MariaDB exception: " + std::string(e.what()), Utils::LogType::Error, "PersistentDatabase::unmuteAccount");
                return false;;
            }
        }

        void PersistentDatabase::switchItemEquip(std::uint32_t accountID, std::uint64_t itemNumber, std::uint32_t characterId)
        {
            try
            {
                std::string queryStr = "UPDATE UserItems SET IsEquipped = CASE WHEN IsEquipped = 0 THEN 1 ELSE 0 END, "
                    " CharacterID = ? WHERE AccountID = ? AND ItemNumber = ?";
                std::unique_ptr<sql::PreparedStatement> stmt(m_con->prepareStatement(queryStr));

                stmt->setUInt(1, characterId);
                stmt->setUInt(2, accountID);
                stmt->setUInt64(3, itemNumber);

                if (stmt->executeUpdate() == 0)
                {
                    ::Utils::Logger::log("Update failed.", Utils::LogType::Warning, "PersistentDatabase::switchItemEquip");
                    return;
                }
            }
            catch (const sql::SQLException& e)
            {
                ::Utils::Logger::log("MariaDB exception: " + std::string(e.what()), Utils::LogType::Error, "PersistentDatabase::switchItemEquip");
                return;
            }
        }

        void PersistentDatabase::unequipItem(std::uint32_t accountID, std::uint64_t unequipItemNumber)
        {
            try
            {
                std::string queryStr = "UPDATE UserItems SET IsEquipped = 0, CharacterID = 0 WHERE AccountID = ? AND ItemNumber = ?";
                std::unique_ptr<sql::PreparedStatement> stmt(m_con->prepareStatement(queryStr));

                stmt->setUInt(1, accountID);
                stmt->setUInt64(2, unequipItemNumber);

                if (stmt->executeUpdate() == 0)
                {
                    ::Utils::Logger::log("Update failed", Utils::LogType::Warning, "PersistentDatabase::unequipItem");
                    return;
                }
            }
            catch (const sql::SQLException& e)
            {
                ::Utils::Logger::log("MariaDB exception: " + std::string(e.what()), Utils::LogType::Error, "PersistentDatabase::unequipItem");
                return;
            }
        }

        void PersistentDatabase::equipItem(std::uint32_t accountID, std::uint64_t equipItemNumber, std::uint16_t characterId)
        {
            try
            {
                std::string queryStr = "UPDATE UserItems SET IsEquipped = 1, CharacterID = ? WHERE AccountID = ? AND ItemNumber = ?";
                std::unique_ptr<sql::PreparedStatement> stmt(m_con->prepareStatement(queryStr));

                stmt->setUInt(1, characterId);
                stmt->setUInt(2, accountID);
                stmt->setUInt64(3, equipItemNumber);

                if (stmt->executeUpdate() == 0)
                {
                    ::Utils::Logger::log("Update failed", Utils::LogType::Warning, "PersistentDatabase::equipItem");
                    return;
                }
            }
            catch (const sql::SQLException& e)
            {
                ::Utils::Logger::log("MariaDB exception: " + std::string(e.what()), Utils::LogType::Error, "PersistentDatabase::equipItem");
                return;
            }
        }

        void PersistentDatabase::swapItems(std::uint32_t accountID, std::uint64_t toUnequipItemNumber, std::uint64_t toEquipItemNumber, std::uint16_t characterId)
        {
            try
            {
                TransactionGuard tg(m_transactionalCon.get());

                std::string queryStr1 = "UPDATE UserItems SET IsEquipped = 1, CharacterID = ? WHERE AccountID = ? AND ItemNumber = ?";
                std::unique_ptr<sql::PreparedStatement> stmt1(m_transactionalCon->prepareStatement(queryStr1));
                stmt1->setUInt(1, characterId);
                stmt1->setUInt(2, accountID);
                stmt1->setUInt64(3, toEquipItemNumber);

                std::string queryStr2 = "UPDATE UserItems SET IsEquipped = 0, CharacterID = 0 WHERE AccountID = ? AND ItemNumber = ?";
                std::unique_ptr<sql::PreparedStatement> stmt2(m_transactionalCon->prepareStatement(queryStr2));
                stmt2->setUInt(1, accountID);
                stmt2->setUInt64(2, toUnequipItemNumber);

                if (stmt1->executeUpdate() == 0)
                {
                    return;
                }

                if (stmt2->executeUpdate() == 0)
                {
                    return;
                }

                tg.commit();
            }
            catch (const sql::SQLException& e)
            {
                ::Utils::Logger::log("MariaDB exception: " + std::string(e.what()), Utils::LogType::Error, "PersistentDatabase::swapItems");
                return;
            }
        }

        void PersistentDatabase::addFriend(std::uint32_t accountID, std::uint32_t targetAccountId)
        {
            try
            {
                TransactionGuard tg(m_transactionalCon.get());

                const std::string insertQuery = "INSERT IGNORE INTO Friendlist (AccountID, TargetAccountID) VALUES (?, ?)";
                std::unique_ptr<sql::PreparedStatement> stmt(m_transactionalCon->prepareStatement(insertQuery));

                stmt->setUInt(1, accountID);
                stmt->setUInt(2, targetAccountId);
                stmt->executeUpdate(); 

                stmt->setUInt(1, targetAccountId);
                stmt->setUInt(2, accountID);
                stmt->executeUpdate(); 

                tg.commit();
            }
            catch (const sql::SQLException& e)
            {
                ::Utils::Logger::log("MariaDB exception: " + std::string(e.what()), Utils::LogType::Error, "PersistentDatabase::addFriend");
            }
        }

        bool PersistentDatabase::resetKillDeath(std::uint32_t accountID)
        {
            try
            {
                const std::string updateQuery = "UPDATE Users SET Kills = 0, Deaths = 0 WHERE AccountID = ?";
                std::unique_ptr<sql::PreparedStatement> stmt(m_con->prepareStatement(updateQuery));
                stmt->setUInt(1, accountID);

                bool success = stmt->executeUpdate() > 0;

                if (!success)
                {
                    ::Utils::Logger::log("Failed to reset kills/deaths for account: " + std::to_string(accountID),
                        Utils::LogType::Warning, "PersistentDatabase::resetKillDeath");
                }

                return success;
            }
            catch (const sql::SQLException& e)
            {
                ::Utils::Logger::log("MariaDB exception: " + std::string(e.what()),
                    Utils::LogType::Error, "PersistentDatabase::resetKillDeath");
                return false;;
            }
        }

        bool PersistentDatabase::resetRecord(std::uint32_t accountID)
        {
            try
            {
                const std::string resetQuery = "UPDATE Users SET Wins = 0, Loses = 0, Draws = 0 WHERE AccountID = ?";
                std::unique_ptr<sql::PreparedStatement> stmt(m_con->prepareStatement(resetQuery));
                stmt->setUInt(1, accountID);

                bool success = stmt->executeUpdate() > 0;

                if (!success)
                {
                    ::Utils::Logger::log("Failed to reset record for account: " + std::to_string(accountID),Utils::LogType::Warning, "PersistentDatabase::resetRecord");
                }

                return success;
            }
            catch (const sql::SQLException& e)
            {
                ::Utils::Logger::log("MariaDB exception: " + std::string(e.what()),Utils::LogType::Error, "PersistentDatabase::resetRecord");
                return false;;
            }
        }

        void PersistentDatabase::addPlayerItem(const Item& item, std::uint32_t accountID, std::uint32_t latestCharacterSelected)
        {
            addPlayerItems(accountID, std::vector<Item>{ item }, latestCharacterSelected);
        }

        bool PersistentDatabase::batteryRecharge(std::uint32_t accountID, std::uint32_t quantity)
        {
            try
            {
                TransactionGuard tg(m_transactionalCon.get());

                const std::string getBattery = "SELECT Battery FROM Users WHERE AccountID = ?";
                const std::string updateBattery = "UPDATE Users SET Battery = ? WHERE AccountID = ?";

                std::unique_ptr<sql::PreparedStatement> stmtGetBattery(m_transactionalCon->prepareStatement(getBattery));
                std::unique_ptr<sql::PreparedStatement> stmtUpdateBattery(m_transactionalCon->prepareStatement(updateBattery));

                stmtGetBattery->setUInt(1, accountID);
                std::unique_ptr<sql::ResultSet> resGetBattery(stmtGetBattery->executeQuery());

                if (!resGetBattery->next())
                {
                    return false;
                }

                uint32_t currentBattery = resGetBattery->getUInt("Battery");
                stmtUpdateBattery->setUInt(1, currentBattery + quantity);
                stmtUpdateBattery->setUInt(2, accountID);

                if (stmtUpdateBattery->executeUpdate() == 0)
                {
                    return false;
                }

                tg.commit();
                return true;
            }
            catch (const sql::SQLException& e)
            {
                ::Utils::Logger::log("MariaDB exception: " + std::string(e.what()), Utils::LogType::Error, "PersistentDatabase::batteryRecharge");
                return false;;
            }
        }

        bool PersistentDatabase::batteryExpansion(std::uint32_t accountID)
        {
            try
            {
                TransactionGuard tg(m_transactionalCon.get());

                const std::string getMaxBattery = "SELECT MaxBattery FROM Users WHERE AccountID = ?";
                const std::string updateMaxBattery = "UPDATE Users SET MaxBattery = ? WHERE AccountID = ?";

                std::unique_ptr<sql::PreparedStatement> stmtGetMaxBattery(m_transactionalCon->prepareStatement(getMaxBattery));
                std::unique_ptr<sql::PreparedStatement> stmtUpdateMaxBattery(m_transactionalCon->prepareStatement(updateMaxBattery));

                stmtGetMaxBattery->setUInt(1, accountID);
                std::unique_ptr<sql::ResultSet> resGetMaxBattery(stmtGetMaxBattery->executeQuery());

                if (!resGetMaxBattery->next())
                {
                    return false;
                }

                uint32_t currentMaxBattery = resGetMaxBattery->getUInt("MaxBattery");
                if (currentMaxBattery > 4000)
                {
                    return false;
                }

                uint32_t newMaxBattery = currentMaxBattery + 1000;
                stmtUpdateMaxBattery->setUInt(1, newMaxBattery);
                stmtUpdateMaxBattery->setUInt(2, accountID);

                if (stmtUpdateMaxBattery->executeUpdate() == 0)
                {
                    return false;
                }

                tg.commit();
                return true;
            }
            catch (const sql::SQLException& e)
            {
                ::Utils::Logger::log("MariaDB exception: " + std::string(e.what()), Utils::LogType::Error, "PersistentDatabase::batteryExpansion");
                return false;;
            }
        }

        bool PersistentDatabase::inventoryExpansion(std::uint32_t accountID, std::uint32_t spaceToAdd)
        {
            try
            {
                TransactionGuard tg(m_transactionalCon.get());

                const std::string getMaxInventory = "SELECT MaxInventory FROM Users WHERE AccountID = ?";
                const std::string updateMaxInventory = "UPDATE Users SET MaxInventory = ? WHERE AccountID = ?";

                std::unique_ptr<sql::PreparedStatement> stmtGetMaxInventory(m_transactionalCon->prepareStatement(getMaxInventory));
                std::unique_ptr<sql::PreparedStatement> stmtUpdateMaxInventory(m_transactionalCon->prepareStatement(updateMaxInventory));

                stmtGetMaxInventory->setUInt(1, accountID);
                std::unique_ptr<sql::ResultSet> resGetMaxInventory(stmtGetMaxInventory->executeQuery());

                if (!resGetMaxInventory->next())
                {
                    return false;
                }

                uint32_t currentMaxInventory = resGetMaxInventory->getUInt("MaxInventory");

                if (currentMaxInventory + spaceToAdd > 1000)
                {
                    return false;
                }

                uint32_t newMaxInventory = currentMaxInventory + spaceToAdd;
                stmtUpdateMaxInventory->setUInt(1, newMaxInventory);
                stmtUpdateMaxInventory->setUInt(2, accountID);

                if (stmtUpdateMaxInventory->executeUpdate() == 0)
                {
                    return false;
                }

                tg.commit();
                return true;
            }
            catch (const sql::SQLException& e)
            {
                ::Utils::Logger::log("MariaDB exception: " + std::string(e.what()), Utils::LogType::Error, "PersistentDatabase::inventoryExpansion");
                return false;;
            }
        }

        void PersistentDatabase::removeFriend(std::uint32_t accountID, std::uint32_t targetAccountId)
        {
            try
            {
                const std::string deleteFriend = "DELETE FROM Friendlist WHERE (AccountID = ? AND TargetAccountID = ?) OR (AccountID = ? AND TargetAccountID = ?)";
                std::unique_ptr<sql::PreparedStatement> stmtDeleteFriend(m_con->prepareStatement(deleteFriend));

                stmtDeleteFriend->setUInt(1, accountID);
                stmtDeleteFriend->setUInt(2, targetAccountId);
                stmtDeleteFriend->setUInt(3, targetAccountId);
                stmtDeleteFriend->setUInt(4, accountID);

                const int deleteResult = stmtDeleteFriend->executeUpdate();
                if (deleteResult == 0)
                {
                    ::Utils::Logger::log("Error executing delete: no rows affected", Utils::LogType::Warning, "PersistentDatabase::removeFriend");
                    return;
                }
            }
            catch (const sql::SQLException& e)
            {
                ::Utils::Logger::log("MariaDB exception: " + std::string(e.what()), Utils::LogType::Error, "PersistentDatabase::removeFriend");
                return;
            }
        }

        std::vector<Main::Structures::Friend> PersistentDatabase::loadFriends(std::uint32_t accountID)
        {
            std::vector<Main::Structures::Friend> friendList;
            std::size_t count = 0;
            try
            {
                static const char* selectFriendsQuery = "SELECT Friendlist.*, Users.Nickname AS TargetNickname "
                    "FROM Friendlist INNER JOIN Users ON Friendlist.TargetAccountID = Users.AccountID "
                    "WHERE Friendlist.AccountID = ?";

                std::unique_ptr<sql::PreparedStatement> stmtSelectFriends(m_con->prepareStatement(selectFriendsQuery));
                stmtSelectFriends->setUInt(1, accountID);
                std::unique_ptr<sql::ResultSet> res(stmtSelectFriends->executeQuery());

                Main::Structures::Friend ffriend;
                while (res->next())
                {
                    if (count >= Common::Constants::maxFriends)
                        break;

                    std::memcpy(ffriend.targetNickname, res->getString("TargetNickname").c_str(), Common::Constants::maxNicknameSize);
                    ffriend.targetAccountId = res->getInt("TargetAccountID");

                    friendList.push_back(ffriend);
                    ++count;
                }
            }
            catch (const sql::SQLException& e)
            {
                ::Utils::Logger::log("MariaDB exception: " + std::string(e.what()), Utils::LogType::Error, "PersistentDatabase::loadFriends");
            }

            return friendList;
        }

        std::vector<Main::Structures::BlockedPlayer> PersistentDatabase::loadBlockedPlayers(std::uint32_t accountID)
        {
            std::vector<Main::Structures::BlockedPlayer> blockedList;
            std::size_t count = 0;
            try
            {
                static const char* selectBlockedPlayersQuery = "SELECT BlockedPlayers.*, Users.Nickname AS TargetNickname "
                    "FROM BlockedPlayers INNER JOIN Users ON BlockedPlayers.TargetAccountID = Users.AccountID "
                    "WHERE BlockedPlayers.AccountID = ?";

                std::unique_ptr<sql::PreparedStatement> stmtSelectBlockedPlayers(m_con->prepareStatement(selectBlockedPlayersQuery));
                stmtSelectBlockedPlayers->setUInt(1, accountID);

                std::unique_ptr<sql::ResultSet> res(stmtSelectBlockedPlayers->executeQuery());

                Main::Structures::BlockedPlayer blockedPlayer;
                while (res->next())
                {
                    if (count >= Common::Constants::maxFriends)
                        break;

                    std::memcpy(blockedPlayer.targetNickname, res->getString("TargetNickname").c_str(), Common::Constants::maxNicknameSize);
                    blockedPlayer.targetAccountId = res->getInt("TargetAccountID");

                    blockedList.push_back(blockedPlayer);
                    ++count;
                }
            }
            catch (const sql::SQLException& e)
            {
                ::Utils::Logger::log("MariaDB exception: " + std::string(e.what()), Utils::LogType::Error, "PersistentDatabase::loadBlockedPlayers");
            }

            return blockedList;
        }

        void PersistentDatabase::blockPlayer(std::uint32_t accountID, std::uint32_t targetAccountId)
        {
            try
            {
                std::unique_ptr<sql::PreparedStatement> stmtBlockPlayer(m_con->prepareStatement("INSERT INTO BlockedPlayers (AccountID, TargetAccountID) VALUES (?, ?)"));
                stmtBlockPlayer->setUInt(1, accountID);
                stmtBlockPlayer->setUInt(2, targetAccountId);

                if (stmtBlockPlayer->executeUpdate() == 0)
                {
                    ::Utils::Logger::log("Error executing query", Utils::LogType::Warning, "PersistentDatabase::blockPlayer");
                    return;
                }
            }
            catch (const sql::SQLException& e)
            {
                ::Utils::Logger::log("MariaDB exception: " + std::string(e.what()), Utils::LogType::Error, "PersistentDatabase::blockPlayer");
                return;
            }
        }

        std::optional<std::uint32_t> PersistentDatabase::blockPlayerByNickname(std::uint32_t accountID, const std::string& targetNickname)
        {
            try
            {
                TransactionGuard tg(m_transactionalCon.get());

                std::unique_ptr<sql::PreparedStatement> stmtFind(m_transactionalCon->prepareStatement(
                    "SELECT AccountID, Grade FROM Users WHERE Nickname = ?"));
                stmtFind->setString(1, targetNickname);
                std::unique_ptr<sql::ResultSet> resFind(stmtFind->executeQuery());

                if (!resFind->next())
                    return std::nullopt;

                const std::uint32_t targetAccountId = resFind->getUInt("AccountID");
                const std::uint32_t targetGrade = resFind->getUInt("Grade");

                if (targetGrade >= 3)
                    return std::nullopt;

                std::unique_ptr<sql::PreparedStatement> stmtBlockPlayer(m_transactionalCon->prepareStatement(
                    "INSERT INTO BlockedPlayers (AccountID, TargetAccountID) VALUES (?, ?)"));
                stmtBlockPlayer->setUInt(1, accountID);
                stmtBlockPlayer->setUInt(2, targetAccountId);

                if (stmtBlockPlayer->executeUpdate() == 0)
                    return std::nullopt;

                tg.commit();
                return targetAccountId;
            }
            catch (const sql::SQLException& e)
            {
                ::Utils::Logger::log("MariaDB exception: " + std::string(e.what()), Utils::LogType::Error, "PersistentDatabase::blockPlayerByNickname");
                return std::nullopt;
            }
        }

        void PersistentDatabase::unblockPlayer(std::uint32_t accountID, std::uint32_t targetAccountId)
        {
            try
            {
                std::unique_ptr<sql::PreparedStatement> stmtUnblockPlayer(m_con->prepareStatement("DELETE FROM BlockedPlayers WHERE AccountID = ? AND TargetAccountID = ?"));
                stmtUnblockPlayer->setUInt(1, accountID);
                stmtUnblockPlayer->setUInt(2, targetAccountId);

                if (stmtUnblockPlayer->executeUpdate() == 0)
                {
                    ::Utils::Logger::log("Error executing query", Utils::LogType::Warning, "PersistentDatabase::unblockPlayer");
                    return;
                }
            }
            catch (const sql::SQLException& e)
            {
                ::Utils::Logger::log("MariaDB exception: " + std::string(e.what()), Utils::LogType::Error, "PersistentDatabase::unblockPlayer");
                return;
            }
        }

        Main::Enums::AddFriendServerExtra PersistentDatabase::addPendingFriendRequest(std::uint32_t aid, const char* targetName)
        {
            try
            {
                TransactionGuard tx(m_transactionalCon.get());

                std::uint32_t targetAid = 0;

                std::unique_ptr<sql::PreparedStatement> stmtFindPlayer(
                    m_transactionalCon->prepareStatement("SELECT AccountID FROM Users WHERE Nickname = ? FOR UPDATE"));
                stmtFindPlayer->setString(1, targetName);

                std::unique_ptr<sql::ResultSet> resFindPlayer(stmtFindPlayer->executeQuery());

                if (!resFindPlayer->next())
                    return Main::Enums::AddFriendServerExtra::TARGET_NOT_FOUND;

                targetAid = static_cast<std::uint32_t>(
                    resFindPlayer->getUInt("AccountID"));

                if (targetAid == aid)
                    return Main::Enums::AddFriendServerExtra::TARGET_NOT_FOUND;

                std::unique_ptr<sql::PreparedStatement> stmtFriendCount(
                    m_transactionalCon->prepareStatement("SELECT COUNT(*) AS FriendCount FROM Friendlist WHERE AccountID = ? FOR UPDATE"));
                stmtFriendCount->setUInt(1, targetAid);

                std::unique_ptr<sql::ResultSet> resFriendCount(stmtFriendCount->executeQuery());

                if (!resFriendCount->next())
                    return Main::Enums::AddFriendServerExtra::DB_ERROR;

                if (resFriendCount->getUInt("FriendCount") >= Common::Constants::maxFriends)
                    return Main::Enums::AddFriendServerExtra::TARGET_OR_SENDER_FRIEND_LIST_FULL;

                std::unique_ptr<sql::PreparedStatement> stmtBlocked(
                    m_transactionalCon->prepareStatement("SELECT 1 FROM BlockedPlayers WHERE AccountID = ? AND TargetAccountID = ? FOR UPDATE"));
                stmtBlocked->setUInt(1, targetAid);
                stmtBlocked->setUInt(2, aid);

                std::unique_ptr<sql::ResultSet> resBlocked(stmtBlocked->executeQuery());

                if (resBlocked->next())
                    return Main::Enums::AddFriendServerExtra::RECEIVER_BLOCKED_SENDER;

                std::unique_ptr<sql::PreparedStatement> stmtInsert(
                    m_transactionalCon->prepareStatement("INSERT INTO PendingFriendRequests (AccountID, TargetAccountID) VALUES (?, ?)"));
                stmtInsert->setUInt(1, targetAid);
                stmtInsert->setUInt(2, aid);

                if (stmtInsert->executeUpdate() == 0)
                    return Main::Enums::AddFriendServerExtra::DB_ERROR;

                tx.commit();
                return Main::Enums::AddFriendServerExtra::REQUEST_SENT;
            }
            catch (const sql::SQLException& e)
            {
                ::Utils::Logger::log("MariaDB exception: " + std::string(e.what()),Utils::LogType::Error,"PersistentDatabase::addPendingFriendRequest");
                return Main::Enums::AddFriendServerExtra::DB_ERROR;
            }
        }

        std::vector<Main::Structures::Friend> PersistentDatabase::loadPendingFriendRequests(std::uint32_t accountID)
        {
            std::vector<Main::Structures::Friend> pendingFriendRequests;

            try
            {
                TransactionGuard tx(m_transactionalCon.get());

                std::unique_ptr<sql::PreparedStatement> stmt(
                    m_transactionalCon->prepareStatement("SELECT PendingFriendRequests.*, Users.Nickname AS TargetNickname "
                        "FROM PendingFriendRequests INNER JOIN Users ON PendingFriendRequests.TargetAccountID = Users.AccountID "
                        "WHERE PendingFriendRequests.AccountID = ? FOR UPDATE"));

                stmt->setUInt(1, accountID);
                std::unique_ptr<sql::ResultSet> res(stmt->executeQuery());
                Main::Structures::Friend ffriend{};

                while (res->next())
                {
                    std::memcpy(ffriend.targetNickname,res->getString("TargetNickname").c_str(),16);

                    ffriend.targetAccountId = res->getUInt("TargetAccountID");

                    pendingFriendRequests.push_back(ffriend);
                }

                if (!pendingFriendRequests.empty())
                {
                    std::unique_ptr<sql::PreparedStatement> stmtRemove(m_transactionalCon->prepareStatement("DELETE FROM PendingFriendRequests WHERE AccountID = ?"));

                    stmtRemove->setUInt(1, accountID);
                    stmtRemove->executeUpdate();
                }

                tx.commit();
            }
            catch (const sql::SQLException& e)
            {
                ::Utils::Logger::log("MariaDB exception: " + std::string(e.what()),Utils::LogType::Error,"PersistentDatabase::loadPendingFriendRequests");
            }

            return pendingFriendRequests;
        }

        // We're limited by uint32_t here since that's what the game expects ....
        std::vector<Main::Structures::SingleModeEvent> PersistentDatabase::getEventsModeList()
        {
            std::vector<Main::Structures::SingleModeEvent> events;

            try
            {
                std::unique_ptr<sql::PreparedStatement> stmt(m_con->prepareStatement("SELECT * FROM EventsModes"));
                std::unique_ptr<sql::ResultSet> res(stmt->executeQuery());

                Main::Structures::SingleModeEvent singleEvent;

                while (res->next())
                {
                    singleEvent.gameMode = static_cast<Common::Enums::GameModes>(res->getInt("GameMode"));

                    const std::string startDateStr = res->getString("StartDate").c_str();
                    const std::string endDateStr = res->getString("EndDate").c_str();

                    std::tm startTm = {};
                    std::tm endTm = {};

                    std::istringstream(startDateStr) >> std::get_time(&startTm, "%Y-%m-%d %H:%M:%S");
                    std::istringstream(endDateStr) >> std::get_time(&endTm, "%Y-%m-%d %H:%M:%S");

                    auto startTimePoint = std::chrono::system_clock::from_time_t(std::mktime(&startTm));
                    auto endTimePoint = std::chrono::system_clock::from_time_t(std::mktime(&endTm));

                    singleEvent.startDate = static_cast<time32_t>(std::chrono::system_clock::to_time_t(startTimePoint));
                    singleEvent.endDate = static_cast<time32_t>(std::chrono::system_clock::to_time_t(endTimePoint));

                    events.push_back(singleEvent);
                }

                return events;
            }
            catch (const sql::SQLException& e)
            {
                ::Utils::Logger::log("MariaDB exception: " + std::string(e.what()), Utils::LogType::Error, "PersistentDatabase::getEventsModeList");
                return events;
            }
        }

        std::vector<Main::Structures::SingleMapEvent> PersistentDatabase::getEventsMapList()
        {
            std::vector<Main::Structures::SingleMapEvent> events;

            try
            {
                std::unique_ptr<sql::PreparedStatement> stmt(m_con->prepareStatement("SELECT * FROM EventsMaps"));
                std::unique_ptr<sql::ResultSet> res(stmt->executeQuery());

                Main::Structures::SingleMapEvent singleEvent;

                while (res->next())
                {
                    singleEvent.gameMap = static_cast<Common::Enums::GameMaps>(res->getInt("GameMap"));

                    const std::string startDateStr = res->getString("StartDate").c_str();
                    const std::string endDateStr = res->getString("EndDate").c_str();

                    std::tm startTm = {};
                    std::tm endTm = {};

                    std::istringstream(startDateStr) >> std::get_time(&startTm, "%Y-%m-%d %H:%M:%S");
                    std::istringstream(endDateStr) >> std::get_time(&endTm, "%Y-%m-%d %H:%M:%S");

                    auto startTimePoint = std::chrono::system_clock::from_time_t(std::mktime(&startTm));
                    auto endTimePoint = std::chrono::system_clock::from_time_t(std::mktime(&endTm));

                    singleEvent.startDate = static_cast<time32_t>(std::chrono::system_clock::to_time_t(startTimePoint));
                    singleEvent.endDate = static_cast<time32_t>(std::chrono::system_clock::to_time_t(endTimePoint));

                    events.push_back(singleEvent);
                }

                return events;
            }
            catch (const sql::SQLException& e)
            {
                ::Utils::Logger::log("MariaDB exception: " + std::string(e.what()), Utils::LogType::Error, "PersistentDatabase::getEventsMapList");
                return events;
            }
        }

        void PersistentDatabase::storeMailbox(const Main::Structures::Mailbox& mailbox, std::uint32_t accountId, bool isSent)
        {
            try
            {
                const std::string storeMailboxQuery = "INSERT INTO Mailbox (accountId, timestamp, nickname, message, sent) VALUES (?, ?, ?, ?, ?)";
                std::unique_ptr<sql::PreparedStatement> stmt(m_con->prepareStatement(storeMailboxQuery));

                stmt->setUInt(1, accountId);
                stmt->setUInt64(2, mailbox.timestamp);
                stmt->setString(3, mailbox.nickname);
                stmt->setString(4, mailbox.message);
                stmt->setBoolean(5, isSent);

                if (stmt->executeUpdate() != 1)
                {
                    ::Utils::Logger::log("Error executing query: " + storeMailboxQuery, Utils::LogType::Warning, "PersistentDatabase::storeMailbox");
                    return;
                }
            }
            catch (const sql::SQLException& e)
            {
                ::Utils::Logger::log("SQLException: " + std::string(e.what()), Utils::LogType::Error, "PersistentDatabase::storeMailbox");
                return;
            }
        }

        bool PersistentDatabase::storeGiftbox(const Main::Structures::Giftbox& giftbox, std::uint32_t accountId)
        {
            try
            {
                const std::string storeGiftboxQuery = "INSERT INTO Giftbox (accountId, itemId, timestamp, sender, message) VALUES (?, ?, ?, ?, ?)";
                std::unique_ptr<sql::PreparedStatement> stmt(m_con->prepareStatement(storeGiftboxQuery));

                stmt->setUInt(1, accountId);
                stmt->setUInt(2, giftbox.id);
                stmt->setUInt64(3, giftbox.timestamp);
                stmt->setString(4, giftbox.nickname);
                stmt->setString(5, giftbox.message);

                if (stmt->executeUpdate() != 1)
                {
                    ::Utils::Logger::log("Error executing query: " + storeGiftboxQuery, Utils::LogType::Warning, "PersistentDatabase::storeGiftbox");
                    return false;
                }
                return true;
            }
            catch (const sql::SQLException& e)
            {
                ::Utils::Logger::log("SQLException: " + std::string(e.what()), Utils::LogType::Error, "PersistentDatabase::storeGiftbox");
                return false;;
            }
        }

        bool PersistentDatabase::storeGiftbox(const std::string& nickname, const std::string& giftDescription, std::uint32_t itemId)
        {
            try
            {
                TransactionGuard tx(m_transactionalCon.get());

                const std::string retrieveAccountIdQuery = "SELECT AccountID FROM Users WHERE Nickname = ? FOR UPDATE";
                std::unique_ptr<sql::PreparedStatement> stmt(m_transactionalCon->prepareStatement(retrieveAccountIdQuery));
                stmt->setString(1, nickname);

                std::unique_ptr<sql::ResultSet> res(stmt->executeQuery());

                if (!res->next())
                {
                    ::Utils::Logger::log("No user found with nickname: " + nickname, Utils::LogType::Warning, "PersistentDatabase::storeGiftbox");
                    return false;
                }
                const std::uint32_t accountId = res->getUInt("AccountID");

                const std::string countGiftboxesQuery = "SELECT COUNT(*) FROM Giftbox WHERE accountId = ? FOR UPDATE";
                std::unique_ptr<sql::PreparedStatement> countStmt(m_transactionalCon->prepareStatement(countGiftboxesQuery));
                countStmt->setUInt(1, accountId);

                std::unique_ptr<sql::ResultSet> countRes(countStmt->executeQuery());

                if (countRes->next())
                {
                    const int giftboxCount = countRes->getInt(1);
                    if (giftboxCount >= Common::Constants::maxMailbox)
                    {
                        return false;
                    }
                }

                Main::Structures::Giftbox giftbox{ accountId, static_cast<time32_t>(std::time(0)), itemId, itemId, itemId };
                std::memcpy(giftbox.nickname, Common::Constants::teamString.c_str(), Common::Constants::teamString.size());
                std::memcpy(giftbox.message, giftDescription.c_str(), giftDescription.size());

                const std::string storeGiftboxQuery = "INSERT INTO Giftbox (accountId, itemId, timestamp, sender, message) VALUES (?, ?, ?, ?, ?)";
                std::unique_ptr<sql::PreparedStatement> insertStmt(m_transactionalCon->prepareStatement(storeGiftboxQuery));

                insertStmt->setUInt(1, accountId);
                insertStmt->setUInt(2, giftbox.id);
                insertStmt->setUInt64(3, giftbox.timestamp);
                insertStmt->setString(4, giftbox.nickname);
                insertStmt->setString(5, giftbox.message);

                if (insertStmt->executeUpdate() != 1)
                {
                    ::Utils::Logger::log("Error executing query: " + storeGiftboxQuery, Utils::LogType::Warning, "PersistentDatabase::storeGiftbox");
                    return false;
                }

                tx.commit();
                return true;
            }
            catch (const sql::SQLException& e)
            {
                ::Utils::Logger::log("SQLException: " + std::string(e.what()), Utils::LogType::Error, "PersistentDatabase::storeGiftbox");
                return false;;
            }
        }
       
        Main::Enums::MailboxExtra PersistentDatabase::storeOfflineMailbox(const Main::Structures::Mailbox& mailbox, const char* senderNickname)
        {
            try
            {
                TransactionGuard tx(m_transactionalCon.get());

                std::uint32_t accountId = 0;
                const std::string retrieveAccountIdQuery = "SELECT AccountID FROM Users WHERE Nickname = ? FOR UPDATE";
                std::unique_ptr<sql::PreparedStatement> stmt(m_transactionalCon->prepareStatement(retrieveAccountIdQuery));
                stmt->setString(1, mailbox.nickname);

                std::unique_ptr<sql::ResultSet> res(stmt->executeQuery());
                if (res->next())
                    accountId = res->getUInt("AccountID");
                else
                    return Main::Enums::MailboxExtra::RECEIVER_NOT_FOUND;

                const std::string countMailboxesQuery = "SELECT COUNT(*) FROM Mailbox WHERE accountId = ? AND sent = 0 FOR UPDATE";
                std::unique_ptr<sql::PreparedStatement> countStmt(m_transactionalCon->prepareStatement(countMailboxesQuery));
                countStmt->setUInt(1, accountId);

                std::unique_ptr<sql::ResultSet> countRes(countStmt->executeQuery());
                if (countRes->next() && countRes->getInt(1) >= Common::Constants::maxMailbox)
                    return Main::Enums::MailboxExtra::RECEIVER_NO_SPACE_LEFT;

                std::uint32_t senderAccountId = 0;
                std::unique_ptr<sql::PreparedStatement> senderQuery(m_transactionalCon->prepareStatement(retrieveAccountIdQuery));
                senderQuery->setString(1, senderNickname);

                std::unique_ptr<sql::ResultSet> senderRes(senderQuery->executeQuery());
                if (senderRes->next())
                    senderAccountId = senderRes->getUInt("AccountID");
                else
                    return Main::Enums::MailboxExtra::MAILBOX_DB_ERROR;

                const std::string blockedPlayersQuery = "SELECT 1 FROM BlockedPlayers WHERE AccountID = ? AND TargetAccountID = ? FOR UPDATE";
                std::unique_ptr<sql::PreparedStatement> blockCheckQuery(m_transactionalCon->prepareStatement(blockedPlayersQuery));
                blockCheckQuery->setUInt(1, accountId);
                blockCheckQuery->setUInt(2, senderAccountId);

                std::unique_ptr<sql::ResultSet> blockCheckRes(blockCheckQuery->executeQuery());
                if (blockCheckRes->next())
                    return Main::Enums::MailboxExtra::MAILBOX_RECEIVER_BLOCKED_SENDER;

                const std::string storeMailboxQuery =
                    "INSERT INTO Mailbox (accountId, timestamp, uniqueId, nickname, message, sent, isNew) VALUES (?, ?, ?, ?, ?, ?, ?)";
                std::unique_ptr<sql::PreparedStatement> query(m_transactionalCon->prepareStatement(storeMailboxQuery));

                query->setUInt(1, accountId);
                query->setUInt64(2, mailbox.timestamp);
                query->setUInt64(3, 0);
                query->setString(4, senderNickname);
                query->setString(5, mailbox.message);
                query->setBoolean(6, false);
                query->setBoolean(7, true);

                if (query->executeUpdate() != 1)
                {
                    ::Utils::Logger::log("Error executing query: " + storeMailboxQuery, Utils::LogType::Error, "PersistentDatabase::storeOfflineMailbox");
                    return Main::Enums::MailboxExtra::MAILBOX_DB_ERROR;
                }

                tx.commit();
                return Main::Enums::MailboxExtra::MAILBOX_SENT;
            }
            catch (const sql::SQLException& e)
            {
                ::Utils::Logger::log("SQLException: " + std::string(e.what()), Utils::LogType::Error, "PersistentDatabase::storeOfflineMailbox");
                return Main::Enums::MailboxExtra::MAILBOX_DB_ERROR;;
            }
        }

        std::vector<Main::Structures::Mailbox> PersistentDatabase::getNewMailboxes(std::uint32_t accountID)
        {
            std::vector<Main::Structures::Mailbox> mailboxes;

            try
            {
                const std::string selectMailboxQuery = "SELECT * FROM Mailbox WHERE accountId = ? AND isNew = 1";
                std::unique_ptr<sql::PreparedStatement> stmt(m_con->prepareStatement(selectMailboxQuery));
                stmt->setUInt(1, accountID);

                std::unique_ptr<sql::ResultSet> res(stmt->executeQuery());

                Main::Structures::Mailbox mailbox;
                while (res->next())
                {
                    mailbox.accountId = res->getUInt("accountId");
                    mailbox.timestamp = res->getUInt("timestamp");
                    std::memcpy(mailbox.nickname, res->getString("nickname").c_str(), 16);
                    std::memcpy(mailbox.message, res->getString("message").c_str(), 256);
                    mailboxes.push_back(mailbox);
                }
            }
            catch (const sql::SQLException& e)
            {
                ::Utils::Logger::log("SQLException: " + std::string(e.what()), Utils::LogType::Error, "PersistentDatabase::getNewMailboxes");
            }

            return mailboxes;
        }

        void PersistentDatabase::updateReadMailbox(std::uint32_t accountID, std::uint32_t timestamp)
        {
            try
            {
                const std::string updateReadMailboxQuery = "UPDATE Mailbox SET isNew = 0 WHERE accountId = ? AND timestamp = ?";
                std::unique_ptr<sql::PreparedStatement> stmt(m_con->prepareStatement(updateReadMailboxQuery));
                stmt->setUInt(1, accountID);
                stmt->setUInt(2, timestamp);

                if (stmt->executeUpdate() != 1)
                {
                    ::Utils::Logger::log("Error executing query: " + updateReadMailboxQuery, Utils::LogType::Warning, "PersistentDatabase::updateReadMailbox");
                }
            }
            catch (const sql::SQLException& e)
            {
                ::Utils::Logger::log("SQLException: " + std::string(e.what()), Utils::LogType::Error, "PersistentDatabase::updateReadMailbox");
                return;
            }
        }

        void PersistentDatabase::deleteMailbox(std::uint32_t timestamp, std::uint32_t accountId, bool isSent)
        {
            try
            {
                const std::string deleteMailboxQuery = "DELETE FROM Mailbox WHERE timestamp = ? AND accountId = ? AND sent = ?";
                std::unique_ptr<sql::PreparedStatement> stmt(m_con->prepareStatement(deleteMailboxQuery));

                stmt->setUInt(1, timestamp);
                stmt->setUInt(2, accountId);
                stmt->setBoolean(3, isSent);

                if (stmt->executeUpdate() != 1)
                {
                    ::Utils::Logger::log("Error executing query: " + deleteMailboxQuery, Utils::LogType::Warning, "PersistentDatabase::deleteMailbox");
                }
            }
            catch (const sql::SQLException& e)
            {
                ::Utils::Logger::log("SQLException: " + std::string(e.what()), Utils::LogType::Error, "PersistentDatabase::deleteMailbox");
                return;
            }
        }

        void PersistentDatabase::deleteReceivedGiftbox(std::uint32_t accountId, std::uint32_t timestamp)
        {
            try
            {
                const std::string deleteGiftboxQuery = "DELETE FROM Giftbox WHERE timestamp = ? AND accountId = ?";
                std::unique_ptr<sql::PreparedStatement> stmt(m_con->prepareStatement(deleteGiftboxQuery));

                stmt->setUInt(1, timestamp);
                stmt->setUInt(2, accountId);

                if (stmt->executeUpdate() != 1)
                {
                    ::Utils::Logger::log("Error executing query: " + deleteGiftboxQuery, Utils::LogType::Warning, "PersistentDatabase::deleteReceivedGiftbox");
                }
            }
            catch (const sql::SQLException& e)
            {
                ::Utils::Logger::log("SQLException: " + std::string(e.what()), Utils::LogType::Error, "PersistentDatabase::deleteReceivedGiftbox");
                return;
            }
        }

        std::pair<std::vector<Main::Structures::Mailbox>, std::vector<Main::Structures::Mailbox>> PersistentDatabase::loadMailboxes(std::uint32_t accountID)
        {
            std::vector<Main::Structures::Mailbox> sentMailboxes;
            std::vector<Main::Structures::Mailbox> receivedMailboxes;

            try
            {
                const std::string selectMailboxQuery = "SELECT * FROM Mailbox WHERE accountId = ? AND (sent = ? OR sent = ?)";
                std::unique_ptr<sql::PreparedStatement> stmt(m_con->prepareStatement(selectMailboxQuery));

                stmt->setUInt(1, accountID);
                stmt->setBoolean(2, true);
                stmt->setBoolean(3, false);

                std::unique_ptr<sql::ResultSet> res(stmt->executeQuery());

                Main::Structures::Mailbox mailbox;
                while (res->next())
                {
                    mailbox.accountId = res->getInt("accountId");
                    mailbox.timestamp = res->getInt("timestamp");
                    mailbox.hasBeenRead = !res->getBoolean("isNew");

                    std::string nicknameStr = res->getString("nickname").c_str();
                    size_t copyLen = nicknameStr.size();
                    if (copyLen >= sizeof(mailbox.nickname)) copyLen = sizeof(mailbox.nickname) - 1;
                    std::memset(mailbox.nickname, 0, sizeof(mailbox.nickname));
                    std::memcpy(mailbox.nickname, nicknameStr.c_str(), copyLen);
                    mailbox.nickname[copyLen] = '\0';

                    std::string messageStr = res->getString("message").c_str();
                    copyLen = messageStr.size();
                    if (copyLen >= sizeof(mailbox.message)) copyLen = sizeof(mailbox.message) - 1;
                    std::memset(mailbox.message, 0, sizeof(mailbox.message));
                    std::memcpy(mailbox.message, messageStr.c_str(), copyLen);
                    mailbox.message[copyLen] = '\0';

                    if (res->getBoolean("sent"))
                        sentMailboxes.push_back(mailbox);
                    else
                        receivedMailboxes.push_back(mailbox);
                }

            }
            catch (const sql::SQLException& e)
            {
                ::Utils::Logger::log("SQLException: " + std::string(e.what()), Utils::LogType::Error, "PersistentDatabase::loadMailboxes");
            }

            return { sentMailboxes, receivedMailboxes };
        }

        std::vector<Main::Structures::Giftbox> PersistentDatabase::loadReceivedGiftboxes(std::uint32_t accountID)
        {
            std::vector<Main::Structures::Giftbox> giftboxes;
            try
            {
                const std::string selectGiftboxQuery = "SELECT * FROM Giftbox WHERE accountId = ?";
                std::unique_ptr<sql::PreparedStatement> stmt(m_con->prepareStatement(selectGiftboxQuery));

                stmt->setUInt(1, accountID);

                std::unique_ptr<sql::ResultSet> res(stmt->executeQuery());

                Main::Structures::Giftbox giftbox;
                while (res && res->next())
                {
                    giftbox.accountId = res->getInt("accountId");
                    giftbox.timestamp = res->getInt("timestamp");
                    giftbox.id = giftbox.id1 = giftbox.id2 = res->getInt("itemId");

                    const std::string& senderStr = res->getString("sender").c_str();
                    const std::string& messageStr = res->getString("message").c_str();

                    std::memset(giftbox.nickname, 0, sizeof(giftbox.nickname));
                    std::memset(giftbox.message, 0, sizeof(giftbox.message));
                    size_t copyLen = senderStr.size();
                    if (copyLen >= sizeof(giftbox.nickname)) copyLen = sizeof(giftbox.nickname) - 1;
                    std::memcpy(giftbox.nickname, senderStr.c_str(), copyLen);
                    giftbox.nickname[copyLen] = '\0'; 
                    copyLen = messageStr.size();
                    if (copyLen >= sizeof(giftbox.message)) copyLen = sizeof(giftbox.message) - 1;
                    std::memcpy(giftbox.message, messageStr.c_str(), copyLen);
                    giftbox.message[copyLen] = '\0';

                    giftboxes.push_back(giftbox);
                }
            }
            catch (const sql::SQLException& e)
            {
                ::Utils::Logger::log("SQLException: " + std::string(e.what()), Utils::LogType::Error, "PersistentDatabase::loadReceivedGiftboxes");
            }
            return giftboxes;
        }

        void PersistentDatabase::insertEnergyToItem(std::uint32_t accountID, std::uint64_t itemNumber, std::uint32_t newItemEnergy, std::uint32_t newTotalEnergy)
        {
            try
            {
                TransactionGuard tx(m_transactionalCon.get());

                const std::string setEnergyForItem = "UPDATE UserItems SET energy = ? WHERE AccountID = ? AND ItemNumber = ?";
                std::unique_ptr<sql::PreparedStatement> stmt1(m_transactionalCon->prepareStatement(setEnergyForItem));
                stmt1->setUInt(1, newItemEnergy);
                stmt1->setUInt(2, accountID);
                stmt1->setUInt64(3, itemNumber);

                const std::string removeEnergyFromTotalEnergy = "UPDATE Users SET Battery = ? WHERE AccountID = ?";
                std::unique_ptr<sql::PreparedStatement> stmt2(m_transactionalCon->prepareStatement(removeEnergyFromTotalEnergy));
                stmt2->setUInt(1, newTotalEnergy);
                stmt2->setUInt(2, accountID);

                if (stmt1->executeUpdate() != 1 || stmt2->executeUpdate() != 1)
                {
                    ::Utils::Logger::log("Error executing queries for energy-insertion", Utils::LogType::Warning, "PersistentDatabase::insertEnergyToItem");
                    return;
                }

                Main::Structures::ItemLogInfo logInfo{ itemNumber, 0, 0, "The user added energy to this item. New item energy is " + std::to_string(newItemEnergy) };
                insertItemLog(accountID, logInfo);

                tx.commit();
            }
            catch (const sql::SQLException& e)
            {
                ::Utils::Logger::log("SQLException: " + std::string(e.what()), Utils::LogType::Error, "PersistentDatabase::insertEnergyToItem");
                return;
            }
        }

        void PersistentDatabase::updatePlayerLuckyPoints(std::uint32_t accountID, std::uint32_t luckyPoints)
        {
            try
            {
                std::string updateLevelQuery = "UPDATE Users SET LuckyPoints = ? WHERE AccountID = ?";
                std::unique_ptr<sql::PreparedStatement> stmt(m_con->prepareStatement(updateLevelQuery));

                stmt->setUInt(1, luckyPoints);
                stmt->setUInt(2, accountID);

                if (stmt->executeUpdate() != 1)
                {
                    ::Utils::Logger::log("Error executing query: " + updateLevelQuery, Utils::LogType::Warning, "PersistentDatabase::updatePlayerLuckyPoints");
                    return;
                }
            }
            catch (const sql::SQLException& e)
            {
                ::Utils::Logger::log("SQLException: " + std::string(e.what()), Utils::LogType::Error, "PersistentDatabase::updatePlayerLuckyPoints");
                return;
            }
        }

        bool PersistentDatabase::setCommandEventExpirationHours(std::uint32_t hoursFromNow)
        {
            try
            {
                TransactionGuard tx(m_transactionalCon.get());

                auto now = std::chrono::system_clock::now();
                auto expirationTime = now + std::chrono::hours(hoursFromNow);
                std::time_t expirationT = std::chrono::system_clock::to_time_t(expirationTime);

                std::ostringstream oss;
                oss << std::put_time(std::localtime(&expirationT), "%Y-%m-%d %H:%M:%S");
                const std::string expirationDate = oss.str();

                const std::string updateQuery = "UPDATE EventCommands SET ExpirationDate = ?";
                std::unique_ptr<sql::PreparedStatement> updateStmt(m_transactionalCon->prepareStatement(updateQuery));
                updateStmt->setString(1, expirationDate);
                int affected = updateStmt->executeUpdate();

                if (affected == 0)
                {
                    const std::string insertQuery = "INSERT INTO EventCommands (ExpirationDate) VALUES (?)";
                    std::unique_ptr<sql::PreparedStatement> insertStmt(m_transactionalCon->prepareStatement(insertQuery));
                    insertStmt->setString(1, expirationDate);
                    insertStmt->executeUpdate();
                }

                tx.commit();
                return true;
            }
            catch (const sql::SQLException& e)
            {
                ::Utils::Logger::log("MariaDB exception: " + std::string(e.what()), Utils::LogType::Error, "PersistentDatabase::setCommandEventExpirationHours");
                return false;
            }
        }

        bool PersistentDatabase::isCommandEventExpired()
        {
            try
            {
                const std::string selectQuery = "SELECT ExpirationDate < NOW() AS Expired FROM EventCommands LIMIT 1";
                std::unique_ptr<sql::PreparedStatement> stmt(m_con->prepareStatement(selectQuery));
                std::unique_ptr<sql::ResultSet> res(stmt->executeQuery());

                if (res->next())
                    return res->getBoolean("Expired");
            }
            catch (const sql::SQLException& e)
            {
                ::Utils::Logger::log("MariaDB exception: " + std::string(e.what()), Utils::LogType::Error, "PersistentDatabase::isEventExpired");
            }

            return true; 
        }

        std::optional<std::pair<std::string, std::string>> PersistentDatabase::getGradedHwid(std::uint32_t accountId)
        {
            try
            {
                std::string query = "SELECT HWIDGraded, HWIDGradedSalt FROM Users WHERE AccountID = ?";
                std::unique_ptr<sql::PreparedStatement> stmt(m_con->prepareStatement(query));
                stmt->setUInt(1, accountId);

                std::unique_ptr<sql::ResultSet> res(stmt->executeQuery());
                if (res->next())
                {
                    std::string hwid = res->getString("HWIDGraded").c_str();
                    std::string salt = res->getString("HWIDGradedSalt").c_str();
                    return std::make_pair(std::move(hwid), std::move(salt));
                }

                return std::nullopt;
            }
            catch (const sql::SQLException& e)
            {
                ::Utils::Logger::log("MariaDB exception in getGradedHwid: " + std::string(e.what()),::Utils::LogType::Error);
                return std::nullopt;
            }
        }

        void PersistentDatabase::updateEvent(std::uint32_t accountId)
        {
            try
            {
                std::string query = "UPDATE Users SET EventEliminationWins = EventEliminationWins + 1 WHERE AccountID = ?";
                std::unique_ptr<sql::PreparedStatement> stmt(m_con->prepareStatement(query));
                stmt->setUInt(1, accountId);
                stmt->executeUpdate();
            }
            catch (const sql::SQLException& e)
            {
                ::Utils::Logger::log("MariaDB exception in updateEvent: " + std::string(e.what()), ::Utils::LogType::Error);
            }
        }

        [[nodiscard]] bool PersistentDatabase::isItemTradeable(std::uint32_t accountId, std::uint32_t itemNumber)
        {
            try
            {
                std::unique_ptr<sql::PreparedStatement> stmt(m_con->prepareStatement(
                    "SELECT IsTradeable FROM UserItems WHERE AccountID = ? AND ItemNumber = ?"));

                stmt->setUInt(1, accountId);
                stmt->setUInt(2, itemNumber);

                std::unique_ptr<sql::ResultSet> result(stmt->executeQuery());

                if (result->next())
                {
                    return result->getInt("IsTradeable") == 1;
                }

                return false; 
            }
            catch (const sql::SQLException& e)
            {
                ::Utils::Logger::log("MariaDB exception in isItemTradeable: " + std::string(e.what()),
                    Utils::LogType::Error, "PersistentDatabase::isItemTradeable");
                return false;
            }
        }

    } // end namespace Main
} // end namespace Persistence
