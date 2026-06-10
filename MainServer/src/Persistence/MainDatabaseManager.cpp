#include <memory>
#include <string>

#include "Structures/AccountInfo/MainAccountInfo.h"
#include "Structures/Item/MainItem.h"
#include "Structures/Item/MainBoughtItem.h"
#include "Structures/Item/MainEquippedItem.h"
#include "Structures/Events/MainEventsList.h"
#include "Structures/PlayerLists/Friend.h"
#include "Structures/PlayerLists/BlockedPlayer.h"
#include "Structures/Mailbox/Mailbox.h"
#include "Structures/AccountInfo/MuteInfo.h"
#include "Persistence/TransactionGuard.h"
#include "Persistence/MainDatabaseManager.h"
#include "MainEnums.h"
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
                            dbSetup.username, dbSetup.password));
                    m_con->setSchema(dbSetup.databaseName);
                    m_con->setAutoCommit(true);

                    m_transactionalCon = std::unique_ptr<sql::Connection>(sql::mariadb::get_driver_instance()->connect("tcp://" + dbSetup.ip + ":" + std::to_string(dbSetup.port),
                            dbSetup.username, dbSetup.password));
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

            conn.reset(sql::mariadb::get_driver_instance()->connect(
                "tcp://" + dbSetup.ip + ":" + std::to_string(dbSetup.port), dbSetup.username, dbSetup.password));
            conn->setSchema(dbSetup.databaseName);
            conn->setAutoCommit(autoCommit);
        }

        bool PersistentDatabase::isValidConnection(const std::unique_ptr<sql::Connection>& conn)
        {
            if (!conn) return false;
            try
            {
                if (conn->isClosed()) return false;
                std::unique_ptr<sql::Statement> stmt(conn->createStatement());
                std::unique_ptr<sql::ResultSet> res(stmt->executeQuery("SELECT 1"));
                return true;
            }
            catch (...)
            {
                return false;
            }
        }

        void PersistentDatabase::ensureConnections()
        {
            if (!isValidConnection(m_con))
                recreateConnection(m_con, true);

            if (!isValidConnection(m_transactionalCon))
                recreateConnection(m_transactionalCon, false);

            try { m_transactionalCon->rollback(); } catch (...) {}
        }

    } // end namespace Main
} // end namespace Persistence
