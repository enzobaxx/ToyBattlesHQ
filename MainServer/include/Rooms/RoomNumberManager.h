#ifndef ROOM_NUMBER_MANAGER_HEADER
#define ROOM_NUMBER_MANAGER_HEADER

#include <queue>
#include <cstdint>
#include <optional>
#include <type_traits>
#include "MainEnums.h"

namespace Main
{
    namespace Classes
    {
        template <Main::Enums::RoomType RoomType>
        class RoomNumberGenerator
        {
        private:
            std::uint16_t m_maxRoomNumber;
            std::uint16_t m_nextNumber;
            std::queue<std::uint16_t> m_availableNumbers;

            RoomNumberGenerator(std::uint16_t maxRoomNumber)
                : m_maxRoomNumber(maxRoomNumber), m_nextNumber(RoomType == Main::Enums::RoomType::Room ? 1 : 151) {}

        public:
            RoomNumberGenerator(const RoomNumberGenerator&) = delete;
            RoomNumberGenerator& operator=(const RoomNumberGenerator&) = delete;

            static RoomNumberGenerator& getInstance(std::uint16_t maxRoomNumber = 600)
            {
                static RoomNumberGenerator instance(maxRoomNumber);
                return instance;
            }

            std::optional<std::uint16_t> generate()
            {
                if (!m_availableNumbers.empty())
                {
                    const std::uint16_t number = m_availableNumbers.front();
                    m_availableNumbers.pop();
                    return number;
                }

                if (m_nextNumber <= m_maxRoomNumber)
                {
                    return m_nextNumber++;
                }

                return std::nullopt; 
            }

            bool release(std::uint16_t number)
            {
                if (number < 1 || number > m_maxRoomNumber)
                {
                    return false;
                }
                m_availableNumbers.push(number);
                return true;
            }
        };

    } 
} 

#endif 
