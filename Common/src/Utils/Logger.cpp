#include <iostream>
#include <fstream>
#include <iomanip>
#include <chrono>
#include <ctime>
#include <string>
#include <sstream>
#include <format>
#include <memory>
#include <queue>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <atomic>
#include "../../include/Utils/Logger.h"

#ifdef _WIN32
#include <Windows.h>
#endif

namespace Utils
{
    namespace
    {
        class AsyncConsole
        {
            std::queue<std::string> m_queue;
            std::mutex m_mutex;
            std::condition_variable m_cv;
            std::atomic<bool> m_stop{ false };
            std::thread m_thread;

            void run()
            {
                std::unique_lock<std::mutex> lock(m_mutex);
                while (true)
                {
                    m_cv.wait(lock, [this] { return m_stop.load() || !m_queue.empty(); });

                    while (!m_queue.empty())
                    {
                        std::string line = std::move(m_queue.front());
                        m_queue.pop();
                        lock.unlock();
                        std::cout << line;
                        lock.lock();
                    }

                    if (m_stop.load() && m_queue.empty())
                        break;
                }
                std::cout.flush();
            }

        public:
            AsyncConsole() : m_thread([this] { run(); }) {}

            ~AsyncConsole()
            {
                {
                    std::lock_guard<std::mutex> lock(m_mutex);
                    m_stop = true;
                }
                m_cv.notify_all();
                if (m_thread.joinable())
                    m_thread.join();
            }

            void enqueue(std::string line)
            {
                {
                    std::lock_guard<std::mutex> lock(m_mutex);
                    m_queue.push(std::move(line));
                }
                m_cv.notify_one();
            }
        };

        AsyncConsole& console()
        {
            static AsyncConsole instance;
            return instance;
        }
    }

    void Logger::enableAnsiEscapeCodes()
    {
#ifdef _WIN32
        HANDLE hOut = GetStdHandle(STD_OUTPUT_HANDLE);
        if (hOut == INVALID_HANDLE_VALUE) return;

        DWORD dwMode = 0;
        if (!GetConsoleMode(hOut, &dwMode)) return;
        SetConsoleMode(hOut, dwMode | ENABLE_VIRTUAL_TERMINAL_PROCESSING);
#endif
    }

    void Logger::printToConsole(const std::string& message, LogType type)
    {
        std::call_once(initFlag, enableAnsiEscapeCodes);

        if (!m_loggingEnabled)
            return;

        std::string line;
        switch (type)
        {
        case LogType::Info:
            line = LogColors::Info + "[Info] " + message + LogColors::Reset + "\n";
            break;
        case LogType::Error:
            line = LogColors::Error + "[Error] " + message + LogColors::Reset + "\n";
            break;
        case LogType::Normal:
            line = LogColors::Normal + message + LogColors::Reset + "\n";
            break;
        case LogType::Warning:
            line = LogColors::Warning + "[Warning] " + message + LogColors::Reset + "\n";
            break;
        }

        console().enqueue(std::move(line));
    }

    void Logger::log(const std::string& message, LogType type, const std::string& functionName)
    {
        std::string logMessage = message;
        if (!functionName.empty())
        {
            logMessage = "[" + functionName + "] " + logMessage;
        }
        printToConsole(logMessage, type);
        newline();
    }

    std::string Logger::getCurrentDateTime()
    {
        auto now = std::chrono::system_clock::now();
        std::time_t now_c = std::chrono::system_clock::to_time_t(now);

        std::tm tm_time{};
#ifdef _WIN32
        localtime_s(&tm_time, &now_c);   // Windows
#else
        localtime_r(&now_c, &tm_time);   // Linux/Unix
#endif

        std::stringstream ss;
        ss << std::put_time(&tm_time, "%Y-%m-%d %X");
        return ss.str();
    }

    std::string Logger::logTypeToString(LogType type)
    {
        switch (type) {
        case LogType::Info: return "Info";
        case LogType::Error: return "Error";
        case LogType::Normal: return "Normal";
        case LogType::Warning: return "Warning";
        }
        return "Unknown";
    }
}
