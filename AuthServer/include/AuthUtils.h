
#ifndef AUTH_UTILITIES_H
#define AUTH_UTILITIES_H

#include <unordered_map>
#include <cstdint>
#include <asio.hpp>
#include <Utils/SetupParser.h>
#include <Network/Packet.h>
#include <Utils/Constants.h>
#include <Utils/Logger.h>

namespace Auth
{
	namespace Utils
	{
        // this also disconnects the player if they're already online through a different session
        inline std::unordered_map<std::uint32_t, std::uint32_t> getPlayersPerServer(std::uint32_t aid, const std::string& ip)
        {
            std::unordered_map<std::uint32_t, std::uint32_t> playerCounts;

            try
            {
                for (const auto& server : Common::Utils::SetupParser::getInstance().getMainServersInfo())
                {
                    try
                    {
                        asio::io_context ioContext;
                        asio::ip::tcp::resolver resolver(ioContext);
                        asio::ip::tcp::socket socket(ioContext);

                        asio::error_code ec;
                        auto endpoints = resolver.resolve(server.ip, std::to_string(server.ipcPort), ec);
                        if (ec)
                        {
                            continue;
                        }

                        asio::connect(socket, endpoints, ec);
                        if (ec)
                        {
                            continue;
                        }

                        Common::Network::UnecryptedPacket packet;

                        // pass IP
                        packet.setTcpHeader(0);
                        packet.setCommand(Common::Constants::A2M_passIp, 0, 0, 0);
                        packet.setData(reinterpret_cast<const std::uint8_t*>(ip.c_str()), ip.size() + 1);
                        asio::write(socket, asio::buffer(packet.generateOutgoingPacket()), ec);
                        if (ec)
                        {
                            continue;
                        }

                        // Request players per server
                        packet.setTcpHeader(0);
                        packet.setCommand(Common::Constants::A2M_getPlayersPerServer, 0, 0, 0);
                        packet.setData(nullptr, 0);
                        asio::write(socket, asio::buffer(packet.generateOutgoingPacket()), ec);
                        if (ec)
                        {
                            continue;
                        }

                        // Disconnect request
                        packet.setTcpHeader(0);
                        packet.setCommand(Common::Constants::A2M_disconnectOnlinePlayer, 0, 0, 0);
                        packet.setData(reinterpret_cast<const std::uint8_t*>(&aid), sizeof(aid));
                        asio::write(socket, asio::buffer(packet.generateOutgoingPacket()));

                        std::vector<std::uint8_t> responseBuffer(Common::Constants::maxPacketBytes);
                        asio::steady_timer timer(ioContext);
                        timer.expires_after(std::chrono::milliseconds(2000));

                        std::size_t bytesRead = 0;
                        socket.async_read_some(asio::buffer(responseBuffer), [&](const std::error_code& error, std::size_t length) {
                            ec = error;
                            bytesRead = length;
                            timer.cancel();
                            });

                        timer.async_wait([&](const std::error_code& error) { if (!error) socket.cancel(); });

                        ioContext.run();

                        if (ec == asio::error::operation_aborted || bytesRead == 0)
                        {
                            continue;
                        }

                        Common::Network::UnecryptedPacket responsePacket;
                        if (!responsePacket.processIncomingPacket(responseBuffer.data(), bytesRead))
                            continue;

                        if (responsePacket.getOrder() != Common::Constants::A2M_getPlayersPerServer)
                            continue;

                        if (bytesRead >= sizeof(std::uint32_t))
                        {
                            const std::uint32_t playerCount = *reinterpret_cast<const std::uint32_t*>(responsePacket.getData());
                            playerCounts[server.serverNumber] = playerCount;
                        }
                    }
                    catch (const std::exception& e)
                    {
                        ::Utils::Logger::log("Exception for server " + server.ip + ": " + std::string(e.what()), ::Utils::LogType::Warning, "Auth::getPlayerCountFromMain");
                    }
                }
            }
            catch (const std::exception& e)
            {
                ::Utils::Logger::log("Exception: " + std::string(e.what()), ::Utils::LogType::Error, "Auth::getPlayerCountFromMain");
            }

            return playerCounts;
        }

	}
}
#endif
