#ifndef CAST_IPC_UTILS_H
#define CAST_IPC_UTILS_H

#include "Network/Session.h"
#include "Network/Sessions/CastSession.h"
#include <Utils/SetupParser.h>
#include <Utils/Logger.h>
#include <Utils/Constants.h>
#include <asio.hpp>
#include <thread>
#include <optional>
#include <cstring>
#include <vector>

namespace Cast
{
    namespace Handlers
    {
        inline void sendPlayerStateUpdate(std::uint32_t accountID, bool isDead, std::shared_ptr<Common::Network::Session> ipcSession)
        {
            if (!Common::Utils::SetupParser::getInstance().getSelfCastServerInfo().IPC_enableDeadBroadcast || !ipcSession) return;

            Common::Network::UnecryptedPacket packet;
            packet.setTcpHeader(0);
            packet.setCommand(Common::Constants::C2M_updatePlayerState, 0, 0, isDead ? Common::Enums::STATE_DYING : Common::Enums::STATE_NORMAL);
            packet.setData(reinterpret_cast<const std::uint8_t*>(&accountID), sizeof(accountID));
            ipcSession->asyncWrite(packet);
        }

        inline void sendCloseSocketReq(std::shared_ptr<Cast::Network::Session> session, std::shared_ptr<Common::Network::Session> ipcSession)
        {
            if (!ipcSession || !ipcSession->is_open())
            {
                session->closeSocket();
                return;
            }

            Common::Network::UnecryptedPacket packet;
            packet.setTcpHeader(0);
            packet.setCommand(Common::Constants::C2M_CloseSocketReq, 0, 0, 0);
            auto seid = session->getId();
            packet.setData(reinterpret_cast<const std::uint8_t*>(&seid), sizeof(seid));
            ipcSession->asyncWrite(packet);
        }

        inline std::optional<std::uint32_t> getSessionId(std::uint32_t aid)
        {
            Common::Network::UnecryptedPacket packet;
            Common::Network::UnecryptedPacket responsePacket;

            try
            {
                asio::io_context ioContext;
                asio::ip::tcp::socket socket(ioContext);
                asio::ip::tcp::resolver resolver(ioContext);

                auto selfMainServerInfo = Common::Utils::SetupParser::getInstance().getSelfMainServerInfo();

                asio::error_code ec;
                auto endpoints = resolver.resolve(selfMainServerInfo.ip, std::to_string(selfMainServerInfo.ipcPort), ec);
                if (ec)
                {
                    ::Utils::Logger::log("Failed to resolve " + selfMainServerInfo.ip, ::Utils::LogType::Warning, "Cast::getSessionId");
                    return std::nullopt;
                }

                asio::connect(socket, endpoints, ec);
                if (ec)
                {
                    ::Utils::Logger::log("Failed to connect to " + selfMainServerInfo.ip, ::Utils::LogType::Warning, "Cast::getSessionId");
                    return std::nullopt;
                }

                socket.set_option(asio::ip::tcp::no_delay(true));
                packet.setTcpHeader(0);
                packet.setCommand(Common::Constants::M2C_sessionId, 0, 0, 0);
                packet.setData(reinterpret_cast<std::uint8_t*>(&aid), sizeof(aid));

                asio::write(socket, asio::buffer(packet.generateOutgoingPacket()), ec);
                if (ec)
                {
                    ::Utils::Logger::log("Failed to send request to " + selfMainServerInfo.ip, ::Utils::LogType::Warning, "Cast::getSessionId");
                    return std::nullopt;
                }

                std::vector<std::uint8_t> responseBuffer(Common::Constants::maxPacketBytes);
                asio::steady_timer timer(ioContext);
                bool timeoutOccurred = false;
                std::size_t bytesRead = 0;

                timer.expires_after(std::chrono::seconds(15));
                timer.async_wait([&](const asio::error_code& error) {
                    if (!error)
                    {
                        timeoutOccurred = true;
                        socket.cancel();
                    }
                });

                socket.async_read_some(asio::buffer(responseBuffer), [&](const asio::error_code& error, std::size_t length) {
                    if (!error) bytesRead = length;
                    timer.cancel();
                });

                ioContext.run();

                if (timeoutOccurred || bytesRead == 0)
                {
                    ::Utils::Logger::log("Timeout / No response from " + selfMainServerInfo.ip, ::Utils::LogType::Warning, "Cast::getSessionId");
                    return std::nullopt;
                }

                if (bytesRead > responseBuffer.size())
                {
                    ::Utils::Logger::log("Buffer overflow detected, bytesRead: " + std::to_string(bytesRead), ::Utils::LogType::Error, "Cast::getSessionId");
                    return std::nullopt;
                }

                if (!responsePacket.processIncomingPacket(responseBuffer.data(), bytesRead))
                {
                    return std::nullopt;
                }

                if (responsePacket.getOrder() != Common::Constants::M2C_sessionId || responsePacket.getExtra() == 1)
                {
                    ::Utils::Logger::log("Invalid session response", ::Utils::LogType::Warning, "Cast::getSessionId");
                    return std::nullopt;
                }

                if (responsePacket.getDataSize() == sizeof(std::uint32_t))
                {
                    const std::uint8_t* dataPtr = responsePacket.getData();
                    if (!dataPtr)
                    {
                        ::Utils::Logger::log("DataPtr was null", ::Utils::LogType::Error, "Cast::getSessionId");
                        return std::nullopt;
                    }

                    std::uint32_t sessionId;
                    std::memcpy(&sessionId, dataPtr, sizeof(sessionId));
                    return sessionId;
                }
                else
                {
                    ::Utils::Logger::log("Unexpected data size: " + std::to_string(responsePacket.getDataSize()), ::Utils::LogType::Error, "Cast::getSessionId");
                }
            }
            catch (const std::exception& e)
            {
                ::Utils::Logger::log("Exception: " + std::string(e.what()), ::Utils::LogType::Error, "Cast::getSessionId");
            }

            return std::nullopt;
        }
    }
}

#endif
