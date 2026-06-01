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

    } // end namespace Main
} // end namespace Persistence
