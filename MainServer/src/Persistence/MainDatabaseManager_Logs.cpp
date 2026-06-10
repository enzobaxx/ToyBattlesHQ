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

    } // end namespace Main
} // end namespace Persistence
