#ifndef IPC_UTILS_MAIN_H
#define IPC_UTILS_MAIN_H

#include <asio.hpp>
#include <Utils/SetupParser.h>
#include <Utils/Constants.h>
#include <Network/Packet.h>
#include "../Structures/AccountInfo/MainAccountUniqueId.h"
#include "../Structures/ClientData/Structures.h"

namespace Main
{
	namespace Ipc
	{
        inline bool M2C_sendMapId(std::uint32_t hostSessionId, std::uint32_t mapId, std::uint32_t modeId)
        {
            try
            {
                auto castServerInfo = Common::Utils::SetupParser::getInstance().getSelfCastServerInfo();
                
                asio::io_context ioContext;
                asio::ip::tcp::socket socket(ioContext);
                asio::ip::tcp::resolver resolver(ioContext);
                asio::connect(socket, resolver.resolve(castServerInfo.ip, std::to_string(castServerInfo.ipcPort)));
                
                Common::Network::UnecryptedPacket packet;
                packet.setTcpHeader(hostSessionId);
                packet.setCommand(Common::Constants::M2C_mapId, 0, mapId, modeId);
                asio::write(socket, asio::buffer(packet.generateOutgoingPacket()));

                std::vector<std::uint8_t> responseBuffer(Common::Constants::maxPacketBytes);
                asio::steady_timer timer(ioContext);
                timer.expires_after(std::chrono::seconds(10));

                std::size_t bytesRead = 0;
                std::error_code ec;

                socket.async_read_some(asio::buffer(responseBuffer), [&](const std::error_code& error, std::size_t length) {
                    ec = error;
                    bytesRead = length;
                    timer.cancel();
                    });

                timer.async_wait([&](const std::error_code& error) { if (!error) socket.cancel(); });
                ioContext.run();

                if (ec == asio::error::operation_aborted || bytesRead < Common::Constants::headerSize)
                {
                    Utils::Logger::log("Timeout or operation aborted", Utils::LogType::Warning, "[Main::M2C_sendMapId");
                    return false;
                }

                Common::Network::UnecryptedPacket responsePacket;
                if (!responsePacket.processIncomingPacket(responseBuffer.data(), bytesRead))
                {
                    return false;
                }

                if (responsePacket.getOrder() == Common::Constants::M2C_mapId)
                {
                    return true;
                }
            }
            catch (const std::exception& e)
            {
                ::Utils::Logger::log("Exception: " + std::string(e.what()), ::Utils::LogType::Error, "Main::M2C_sendMapId");
                return false;
            }
            return false;
        }

        inline bool M2C_sendRoomNumber(std::uint32_t hostSessionId, std::uint32_t roomNumber)
        {
            try
            {
                auto castServerInfo = Common::Utils::SetupParser::getInstance().getSelfCastServerInfo();
               
                asio::io_context ioContext;
                asio::ip::tcp::socket socket(ioContext);
                asio::ip::tcp::resolver resolver(ioContext);
                asio::connect(socket, resolver.resolve(castServerInfo.ip, std::to_string(castServerInfo.ipcPort)));
           
                Common::Network::UnecryptedPacket packet;
                packet.setTcpHeader(hostSessionId);
                packet.setCommand(Common::Constants::M2C_roomNumber, 0, roomNumber, 0);
                asio::write(socket, asio::buffer(packet.generateOutgoingPacket()));

                std::vector<std::uint8_t> responseBuffer(Common::Constants::maxPacketBytes);
                asio::steady_timer timer(ioContext);
                timer.expires_after(std::chrono::seconds(10));

                std::size_t bytesRead = 0;
                std::error_code ec;

                socket.async_read_some(asio::buffer(responseBuffer), [&](const std::error_code& error, std::size_t length) {
                    ec = error;
                    bytesRead = length;
                    timer.cancel();
                    });

                timer.async_wait([&](const std::error_code& error) { if (!error) socket.cancel(); });
                ioContext.run();

                if (ec == asio::error::operation_aborted || bytesRead < Common::Constants::headerSize)
                {
                    Utils::Logger::log("Timeout or operation aborted", Utils::LogType::Warning, "[Main::M2C_sendRoomNumber");
                    return false;
                }

                Common::Network::UnecryptedPacket responsePacket;
                if (!responsePacket.processIncomingPacket(responseBuffer.data(), bytesRead))
                {
                    return false;
                }

                if (responsePacket.getOrder() == Common::Constants::M2C_roomNumber)
                {
                    return true;
                }
            }
            catch (const std::exception& e)
            {
                ::Utils::Logger::log("Exception: " + std::string(e.what()), ::Utils::LogType::Error, "Main::M2C_sendRoomNumber");
                return false;
            }
            return false;
        }


        inline bool M2C_sendAssassinModeInfo(bool isAssasinMode, std::uint32_t hostSessionId, const Main::Structures::UniqueId& assassinUidBlue = {},
            const std::string& assassinNameBlue = {},
            const Main::Structures::UniqueId& assassinUidRed = {}, const std::string& assassinNameRed = {})
        {
            try
            {
                auto castServerInfo = Common::Utils::SetupParser::getInstance().getSelfCastServerInfo();

                asio::io_context ioContext;
                asio::ip::tcp::socket socket(ioContext);
                asio::ip::tcp::resolver resolver(ioContext);
                asio::connect(socket, resolver.resolve(castServerInfo.ip, std::to_string(castServerInfo.ipcPort)));

                Common::Network::UnecryptedPacket packet;
                packet.setTcpHeader(hostSessionId);
                packet.setCommand(Common::Constants::M2C_assassinModeInfo, 0, 0, 0);
                struct AssassinData
                {
                    bool isAssassinMode{};
                    Main::Structures::UniqueId uidRed{};
                    char nameRed[16]{};
                    Main::Structures::UniqueId uidBlue{};
                    char nameBlue[16]{};
                } assassinInfo;
                assassinInfo.isAssassinMode = isAssasinMode;
                assassinInfo.uidRed = assassinUidRed;
                assassinInfo.uidBlue = assassinUidBlue;
                std::strncpy(assassinInfo.nameRed, assassinNameRed.c_str(), sizeof(assassinInfo.nameRed) - 1);
                std::strncpy(assassinInfo.nameBlue, assassinNameBlue.c_str(), sizeof(assassinInfo.nameBlue) - 1);
                packet.setData(reinterpret_cast<std::uint8_t*>(&assassinInfo), sizeof(assassinInfo));
                asio::write(socket, asio::buffer(packet.generateOutgoingPacket()));

                std::vector<std::uint8_t> responseBuffer(Common::Constants::maxPacketBytes);
                asio::steady_timer timer(ioContext);
                timer.expires_after(std::chrono::seconds(10));

                std::size_t bytesRead = 0;
                std::error_code ec;

                socket.async_read_some(asio::buffer(responseBuffer), [&](const std::error_code& error, std::size_t length) {
                    ec = error;
                    bytesRead = length;
                    timer.cancel();
                    });

                timer.async_wait([&](const std::error_code& error) { if (!error) socket.cancel(); });
                ioContext.run();

                if (ec == asio::error::operation_aborted || bytesRead < Common::Constants::headerSize)
                {
                    Utils::Logger::log("Timeout or operation aborted", Utils::LogType::Warning, "[Main::M2C_sendMapId");
                    return false;
                }

                Common::Network::UnecryptedPacket responsePacket;
                if (!responsePacket.processIncomingPacket(responseBuffer.data(), bytesRead))
                {
                    return false;
                }

                if (responsePacket.getOrder() == Common::Constants::M2C_assassinModeInfo)
                {
                    return true;
                }
            }
            catch (const std::exception& e)
            {
                ::Utils::Logger::log("Exception: " + std::string(e.what()), ::Utils::LogType::Error, "Main::M2C_sendMapId");
                return false;
            }
            return false;
        }

        inline bool M2C_sendPlayerTeamInfoBatch(std::uint32_t hostSessionId, const std::vector<Main::ClientData::PlayerTeamInfo>& playerInfos)
        {
            try
            {
                auto castServerInfo = Common::Utils::SetupParser::getInstance().getSelfCastServerInfo();

                asio::io_context ioContext;
                asio::ip::tcp::socket socket(ioContext);
                asio::ip::tcp::resolver resolver(ioContext);
                asio::connect(socket, resolver.resolve(castServerInfo.ip, std::to_string(castServerInfo.ipcPort)));

                Common::Network::UnecryptedPacket packet;
                packet.setTcpHeader(hostSessionId);
                packet.setCommand(Common::Constants::M2C_playerTeamInfoBatch, 0, 0, 0);

                packet.setData(reinterpret_cast<const std::uint8_t*>(playerInfos.data()),
                    playerInfos.size() * sizeof(Main::ClientData::PlayerTeamInfo));

                asio::write(socket, asio::buffer(packet.generateOutgoingPacket()));

                return true;
            }
            catch (const std::exception& e)
            {
                Utils::Logger::log("Exception: " + std::string(e.what()), Utils::LogType::Error, "M2C_sendPlayerTeamInfoBatch");
                return false;
            }
        }

        enum InvisibilityType { SELF_NOT_INVISIBLE = 0, SELF_INVISIBLE, ALL_INVISIBLE, NONE_INVISIBLE };
        inline void M2C_sendInvisibilityCommand(std::uint32_t sessionId, InvisibilityType type)
        {
            try
            {
                auto castServerInfo = Common::Utils::SetupParser::getInstance().getSelfCastServerInfo();

                asio::io_context ioContext;
                asio::ip::tcp::socket socket(ioContext);
                asio::ip::tcp::resolver resolver(ioContext);
                asio::connect(socket, resolver.resolve(castServerInfo.ip, std::to_string(castServerInfo.ipcPort)));

                Common::Network::UnecryptedPacket packet;
                packet.setTcpHeader(sessionId);
                packet.setCommand(Common::Constants::M2C_Invisibility, 0, type, 0);
                asio::write(socket, asio::buffer(packet.generateOutgoingPacket()));
            }
            catch (const std::exception& e)
            {
                ::Utils::Logger::log("Exception: " + std::string(e.what()), ::Utils::LogType::Error, "Main::M2C_sendMapId");
            }
        }
	}
}
#endif
