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

    } // end namespace Main
} // end namespace Persistence
