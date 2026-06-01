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

        sql::Connection* ConnectionHandle::get() const
        {
            static thread_local std::unique_ptr<sql::Connection> autoConn;
            static thread_local std::unique_ptr<sql::Connection> transConn;
            std::unique_ptr<sql::Connection>& slot = m_autoCommit ? autoConn : transConn;

            if (slot)
            {
                try
                {
                    if (!slot->isClosed())
                        return slot.get();
                }
                catch (...) {}
                slot.reset();
            }

            const auto& dbSetup = Common::Utils::SetupParser::getInstance().getDatabaseSetup();
            slot.reset(sql::mariadb::get_driver_instance()->connect(
                "tcp://" + dbSetup.ip + ":" + std::to_string(dbSetup.port), dbSetup.username, dbSetup.password));
            slot->setSchema(dbSetup.databaseName);
            slot->setAutoCommit(m_autoCommit);
            return slot.get();
        }

        void PersistentDatabase::connectWithRetry()
        {
            constexpr int maxRetries = 5;

            for (int attempt = 1; attempt <= maxRetries; ++attempt)
            {
                try
                {
                    ::Utils::Logger::log("Connecting to MariaDB (attempt " + std::to_string(attempt) + ")", Utils::LogType::Info, "PersistentDatabase");

                    m_con.get();
                    m_transactionalCon.get();

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

    } // end namespace Main
} // end namespace Persistence
