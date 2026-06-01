
#include <unordered_map>
#include <map>
#include <string>
#include <thread>
#include <chrono>
#include <mutex>

#include "../../include/Structures/AccountInfo/MainAccountInfo.h"
#include "../../include/Structures/Item/MainItem.h"
#include "../../include/Structures/Item/MainEquippedItem.h"
#include "../../include/Persistence/MainDatabaseManager.h"
#include "../../include/Persistence/MainScheduler.h"

namespace Main
{
    namespace Persistence
    {
        MainScheduler::MainScheduler(std::size_t wakeupFrequency, Main::Persistence::PersistentDatabase& database)
            : m_wakeupFrequency{ wakeupFrequency }
            , m_database{ database }
        {
            m_schedulerThread = std::thread(&MainScheduler::schedulerLoop, this);
        }

        MainScheduler::~MainScheduler()
        {
            {
                std::lock_guard<std::mutex> lock(m_wakeupMutex);
                m_stopRequested = true;
            }
            m_wakeupCv.notify_all();
            if (m_schedulerThread.joinable())
            {
                m_schedulerThread.join();
            }
        }

        void MainScheduler::schedulerLoop()
        {
            while (!m_stopRequested)
            {
                {
                    std::unique_lock<std::mutex> lock(m_wakeupMutex);
                    m_wakeupCv.wait_for(lock, std::chrono::seconds(m_wakeupFrequency), [this] { return m_stopRequested.load(); });
                }

                if (m_stopRequested) break;

                try
                {
                    persist();
                }
                catch (const std::exception& e)
                {
                    ::Utils::Logger::log("Database failure during persist, scheduler continues: " + std::string(e.what()),
                        Utils::LogType::Error, "MainScheduler::schedulerLoop");
                }
            }
        }

        void MainScheduler::persist()
        {
            std::unordered_map<std::uint32_t, std::map<std::size_t, std::function<void()>>> incremental;
            std::unordered_map<std::uint32_t, std::map<std::size_t, std::function<void()>>> normal;

            {
                std::unique_lock<std::mutex> lock(m_callbacksMutex);

                incremental = std::move(m_databaseCallbacksIncremental);
                normal = std::move(m_databaseCallbacks);

                m_incrementalDifferentiationKey = 0;
                m_databaseCallbacks.clear();
                m_databaseCallbacksIncremental.clear();
            }

            for (const auto& [accountId, callbacks] : incremental)
                for (const auto& [updateType, callback] : callbacks)
                    runCallback(callback);

            for (const auto& [accountId, callbacks] : normal)
                for (const auto& [updateType, callback] : callbacks)
                    runCallback(callback);
        }

        void MainScheduler::runCallback(const std::function<void()>& callback)
        {
            try
            {
                callback();
            }
            catch (const std::exception& e)
            {
                ::Utils::Logger::log("A persistence callback threw, skipping it: " + std::string(e.what()),
                    Utils::LogType::Error, "MainScheduler::runCallback");
            }
        }

        void MainScheduler::persistFor(std::uint32_t accountId)
        {
            std::map<std::size_t, std::function<void()>> incremental;
            std::map<std::size_t, std::function<void()>> normal;

            {
                std::unique_lock<std::mutex> lock(m_callbacksMutex);

                incremental = std::move(m_databaseCallbacksIncremental[accountId]);
                normal = std::move(m_databaseCallbacks[accountId]);

                m_databaseCallbacks.erase(accountId);
                m_databaseCallbacksIncremental.erase(accountId);
            }

            for (const auto& [updateType, callback] : incremental)
                runCallback(callback);

            for (const auto& [updateType, callback] : normal)
                runCallback(callback);
        }
    };
}
