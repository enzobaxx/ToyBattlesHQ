#ifndef CAST_SESSION_HEADER
#define CAST_SESSION_HEADER

#include <functional>
#include <chrono>
#include <array>

#include <iostream>
#include <Utils/Parser.h>

#include "asio.hpp"

#include "../../../MainServer/include/Structures/AccountInfo/MainAccountInfo.h"
#include "Network/Session.h"
#include "../Structures/PlayerPositionFromClient.h"
#include "../include/Enums/PlayerEnums.h"

namespace Cast
{
    namespace Network
    {
        class Session final : public Common::Network::Session
        {
        protected:
            std::uint32_t m_roomNumber{};

        public:
            std::chrono::steady_clock::time_point m_lastPing = std::chrono::steady_clock::now();
            Common::Enums::Team m_team{};
            bool isDead{};
            bool m_isInMatch{ 0 };
            std::string m_nickname;
            bool m_isInvisible{};

        public:
            explicit Session(asio::ip::tcp::socket&& socket, std::function<void(std::size_t)> fnct)
                : Common::Network::Session{ std::move(socket), fnct }
            {
                m_socket.set_option(asio::ip::tcp::no_delay(true));
            }

            void setRoomNumber(std::uint32_t roomNum);
            std::uint32_t getRoomNumber() const;
            bool isInMatch() const;
            void setIsInMatch(bool val);
            void onPacket(std::vector<std::uint8_t>& data) final override;
            void setTeam(Common::Enums::Team team) { m_team = team; }
            Common::Enums::Team getTeam() const noexcept { return m_team; }
        };
    }
}

#endif