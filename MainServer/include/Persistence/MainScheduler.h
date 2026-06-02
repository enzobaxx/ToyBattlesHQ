#ifndef MAIN_SCHEDULER_HEADER
#define MAIN_SCHEDULER_HEADER

#include <unordered_map>
#include <map>
#include <string>
#include <thread>
#include <chrono>
#include <mutex>
#include <atomic>
#include <condition_variable>
#include <functional>

#include "../Structures/AccountInfo/MainAccountInfo.h"
#include "../Structures/Item/MainItem.h"
#include "../Structures/Item/MainEquippedItem.h"
#include "../Persistence/MainDatabaseManager.h"
#include <source_location>

// Used to accept overloaded member functions inside Database.h when calling MainScheduler::immediatePersist(...)
#define FWD(...) static_cast<decltype(__VA_ARGS__)&&>(__VA_ARGS__)

#define ARROW(...) \
    noexcept(noexcept((__VA_ARGS__))) -> decltype(auto) requires(requires { (__VA_ARGS__); }) \
    { return (__VA_ARGS__); }

#define LIFT_MEMBER(...) \
    [](auto&& detail_obj, auto&&... detail_args) ARROW(FWD(detail_obj).__VA_ARGS__(FWD(detail_args)...))


namespace Main
{
    namespace Persistence
    {
        class MainScheduler
        {
        private:
            using AccountInfo = Main::Structures::AccountInfo;
            using Item = Main::Structures::Item;
            using EquippedItem = Main::Structures::EquippedItem;
            using BoughtItem = Main::Structures::BoughtItem;

            std::size_t m_wakeupFrequency{};
            std::thread m_schedulerThread{};
            std::atomic<bool> m_stopRequested{ false };
            std::condition_variable m_wakeupCv;
            std::mutex m_wakeupMutex;

            Main::Persistence::PersistentDatabase& m_database;
            std::unordered_map<std::uint32_t, std::map<std::size_t, std::function<void()>>> m_databaseCallbacks{};
            std::unordered_map<std::uint32_t, std::map<std::size_t, std::function<void()>>> m_databaseCallbacksIncremental{};
            std::size_t m_incrementalDifferentiationKey = 0;
            std::mutex m_callbacksMutex;


        public:
            explicit MainScheduler(std::size_t wakeupFrequency, Main::Persistence::PersistentDatabase& database);

            ~MainScheduler();

            template<typename Function, typename... Args>
            void addCallback(const std::source_location& loc, std::uint32_t accountId, std::size_t differentiationKey, Function databaseMemberFunction, Args&&... args)
            {
                std::unique_lock<std::mutex> lock(m_callbacksMutex);
                m_databaseCallbacks[accountId][differentiationKey] = std::bind(databaseMemberFunction, &m_database, std::forward<Args>(args)...);
            }

            template <typename Function, typename... Args>
            void addRepetitiveCallback(const std::source_location& loc, std::uint32_t accountId, Function databaseMemberFunction, Args&&... args)
            {
                std::unique_lock<std::mutex> lock(m_callbacksMutex);
                m_databaseCallbacksIncremental[accountId][++m_incrementalDifferentiationKey] =
                    [this, databaseMemberFunction, ...args = std::forward<Args>(args)]() mutable {
                    std::invoke(databaseMemberFunction, m_database, std::forward<decltype(args)>(args)...);
                    };
            }


            template<typename F, typename... Args>
            decltype(auto) immediatePersist(const std::source_location& loc, F databaseMemberFunction, Args&&... args)
            {
                return m_database.withGuard([&]() -> decltype(auto) {
                    return std::invoke(databaseMemberFunction, m_database, std::forward<Args>(args)...);
                });
            }


        private:
            void schedulerLoop();
            void persist();
            static void runCallback(const std::function<void()>& callback);

        public:
            void persistFor(std::uint32_t accountId);
        };
    }
}
#endif