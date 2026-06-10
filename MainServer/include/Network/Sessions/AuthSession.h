#ifndef MAIN_AUTH_SESSION_H
#define MAIN_AUTH_SESSION_H

#include <asio.hpp>
#include "Network/MainSessionManager.h"
#include <unordered_map>
#include "Utils/Logger.h"

// Used for Main<=>Auth IPC communication
// (AuthServer acts as a client here, retrieves certain info, and MainServer responds to its requests)
namespace Main 
{
    namespace Network 
    {
        struct AuthSession : public Common::Network::Session
        {
            explicit AuthSession(asio::ip::tcp::socket socket)
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
                if (!Common::Network::Session::callbacks<Common::Network::PacketType::UNECRYPTED, AuthSession>.contains(callbackNum))
                {
                    Utils::Logger::log("[IPC Auth<=>Main] No callback for order: " + std::to_string(callbackNum), Utils::LogType::Error, "AuthSession::onPacket");
                    return;
                }
                Common::Network::Session::callbacks<Common::Network::PacketType::UNECRYPTED, AuthSession>[callbackNum](incomingPacket, 
                    std::static_pointer_cast<AuthSession>(shared_from_this()));
            }
        };
    }
}

#endif