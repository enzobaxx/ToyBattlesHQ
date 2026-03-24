#ifndef AUTO_MAIN_SESSION_H
#define AUTO_MAIN_SESSION_H

#include <asio.hpp>
#include <unordered_map>
#include "Network/Session.h"

// Used for Main<=>Auth IPC communication
// (MainServer acts as a client here, retrieves certain info, and MainServer responds to its requests)
namespace Auth
{
    namespace Network
    {
        struct MainSession : public Common::Network::Session
        {
            explicit MainSession(asio::ip::tcp::socket socket)
                : Common::Network::Session{ std::move(socket), nullptr }
            {
            }

            void onPacket(std::vector<std::uint8_t>& data) override
            {
                Common::Network::UnecryptedPacket incomingPacket;
                if (!incomingPacket.processIncomingPacket(data.data(), static_cast<std::uint16_t>(data.size())))
                {
                    closeSocket();
                    return;
                }

                const std::uint16_t callbackNum = incomingPacket.getOrder();
                if (!Common::Network::Session::callbacks<Common::Network::PacketType::UNECRYPTED, MainSession>.contains(callbackNum))
                {
                    std::cout << "[IPC Auth<=>Main] No callback for order: " << callbackNum << "\n";
                    return;
                }

                Common::Network::Session::callbacks<Common::Network::PacketType::UNECRYPTED, MainSession>[callbackNum](incomingPacket, std::static_pointer_cast<MainSession>(shared_from_this()));

            }
        };
    }
}

#endif