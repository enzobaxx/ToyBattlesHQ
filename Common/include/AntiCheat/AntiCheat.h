#ifndef AC_AC_BASE_H
#define AC_AC_BASE_H

#include "AcEventQueue.h"
#include "Interfaces.h"
#include "Interfaces.h"
#include <iostream>
#include "Checks/PacketFloodingCheck.h"
#include "Checks/PacketReplicaCheck.h"

#include <mariadb/conncpp.hpp>
#include <mariadb/conncpp/Connection.hpp>
#include "../Utils/SetupParser.h"

namespace Ac
{
    class AntiCheatManager
    {
    private:
        std::unique_ptr<sql::Connection> m_con;
        ACEventQueue m_eventQueue;
        std::vector<std::unique_ptr<IACCheckerBase>> m_checkers;
        std::thread m_thread;
        std::atomic<bool> m_isRunning{ false };

        AntiCheatManager(const AntiCheatManager&) = delete;
        AntiCheatManager& operator=(const AntiCheatManager&) = delete;

        void worker()
        {
            while (m_isRunning)
            {
                auto event = m_eventQueue.popEvent();
                if (!event) continue;

                for (auto& checker : m_checkers)
                {
                    if (auto flag = checker->processEventBase(*event))
                    {
                        handleFlag(*flag);
                    }
                }
            }
        }

        void handleFlag(const ACFlag& flag)
        {
            std::ostringstream descriptionStream;
            descriptionStream << "CheatType: " << flag.cheatType
                << ", AccountID: " << flag.sessionId
                << ", Details: " << flag.details;
            const std::string description = descriptionStream.str();

            for (int attempt = 0; attempt < 2; ++attempt)
            {
                if (!m_con)
                    connectToDb();

                if (!m_con)
                {
                    ::Utils::Logger::log("Cannot log cheat flag: no database connection available",
                        Utils::LogType::Error, "AntiCheat::handleFlag");
                    return;
                }

                try
                {
                    const std::string insertQuery = "INSERT INTO CheatFlags (Description) VALUES (?)";
                    std::unique_ptr<sql::PreparedStatement> insertStmt(m_con->prepareStatement(insertQuery));
                    insertStmt->setString(1, description);
                    insertStmt->executeUpdate();
                    return;
                }
                catch (const sql::SQLException& e)
                {
                    ::Utils::Logger::log(std::string("MariaDB exception while logging cheat flag (attempt ")
                        + std::to_string(attempt + 1) + "): " + e.what(),
                        Utils::LogType::Error, "AntiCheat::handleFlag");
                    m_con.reset();
                }
            }
        }

        void connectToDb()
        {
            const auto& dbSetup = Common::Utils::SetupParser::getInstance().getDatabaseSetup();
            try
            {
                m_con.reset(sql::mariadb::get_driver_instance()->connect("tcp://" + dbSetup.ip + ":" + std::to_string(dbSetup.port),
                    dbSetup.username, dbSetup.password));
                if (!m_con)
                    return;
                m_con->setSchema(dbSetup.databaseName);
                m_con->setAutoCommit(true);

                const std::string createTableQuery =
                    "CREATE TABLE IF NOT EXISTS CheatFlags ("
                    "ID INT AUTO_INCREMENT PRIMARY KEY, "
                    "Description TEXT NOT NULL"
                    ")";
                std::unique_ptr<sql::Statement> createStmt(m_con->createStatement());
                createStmt->execute(createTableQuery);
            }
            catch (const sql::SQLException& e)
            {
                ::Utils::Logger::log("Error connecting to MariaDB: " + std::string(e.what()),
                    Utils::LogType::Error, "AntiCheat::connectToDb");
                m_con.reset();
            }
        }

    public:
        AntiCheatManager(bool startWorker = true)
        {
            //registerChecker<PacketFloodChecker>();
            registerChecker<PacketReplicationChecker>();

            if (startWorker)
            {
                m_isRunning = true;
                m_thread = std::thread(&AntiCheatManager::worker, this);
            }
        }


        ~AntiCheatManager()
        {
            m_isRunning = false;
            m_eventQueue.shutdown();
            if (m_thread.joinable()) m_thread.join();
        }

        template<typename CheckerT>
        void registerChecker() 
        {
            m_checkers.push_back(std::make_unique<CheckerT>());
        }

        void submitEvent(std::unique_ptr<ACEvent> event) 
        {
            m_eventQueue.pushEvent(std::move(event));
        }
    };
}


#endif