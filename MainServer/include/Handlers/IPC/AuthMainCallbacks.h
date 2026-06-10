#ifndef AUTH_MAIN_IPC_CALLBACKS_H
#define AUTH_MAIN_IPC_CALLBACKS_H

#include "Network/Packet.h"
#include "Network/MainSessionManager.h"
#include "Network/Sessions/AuthSession.h"
#include <string>

namespace Main
{
	namespace Handlers
	{
        inline void disconnectIfOnline(const Common::Network::UnecryptedPacket& request, std::shared_ptr<Common::Network::Session> session, 
            Main::Network::SessionsManager& sessionsManager)
        {
            auto response = request;

            const std::uint32_t accountId = Main::Details::parseData<std::uint32_t>(request); 

            if (auto targetSession = sessionsManager.getSessionByAccountId(accountId); targetSession)
            {
                Common::Network::Packet response;
                response.setTcpHeader(targetSession->getId(), Common::Enums::NO_ENCRYPTION);
                sessionsManager.removeSession(targetSession->getId());
                response.setOrder(73);
                response.setExtra(5);
                targetSession->asyncWrite(response);
                response.setExtra(0);
            }
            else
            { // player not found on this server, proceed to check whether they're on other servers
                response.setExtra(1);
            }
            session->asyncWrite(response); // "pong" the auth server
        }

        inline void handlePlayerCountRequest(const Common::Network::UnecryptedPacket& request, std::shared_ptr<Common::Network::Session> session,
            Main::Network::SessionsManager& sessionsManager)
        {
            auto response = request;
            const std::uint32_t totalPlayers = sessionsManager.getTotalSessions();
            response.setData(reinterpret_cast<const std::uint8_t*>(&totalPlayers), sizeof(totalPlayers));
            session->asyncWrite(response); 
        }
        
        inline bool sendIpToCast(const std::string& ip)
        {
            try
            {
                auto castServerInfo = Common::Utils::SetupParser::getInstance().getSelfCastServerInfo();

                asio::io_context ioContext;
                asio::ip::tcp::socket socket(ioContext);
                asio::ip::tcp::resolver resolver(ioContext);

                asio::connect(socket, resolver.resolve(castServerInfo.ip, std::to_string(castServerInfo.ipcPort)));

                asio::error_code ec;
                Common::Network::UnecryptedPacket packet;
                packet.setTcpHeader(0);
                packet.setCommand(Common::Constants::A2M_passIp, 0, 0, 0);
                packet.setData(reinterpret_cast<const std::uint8_t*>(ip.c_str()), ip.size() + 1);
                asio::write(socket, asio::buffer(packet.generateOutgoingPacket()), ec);
                if (ec)
                {
                    ::Utils::Logger::log("Failed to send IP to " + castServerInfo.ip, ::Utils::LogType::Warning, "Main::sendIpToCast");
                    return false;
                }
                return true;
            }
            catch (const std::exception& e)
            {
                ::Utils::Logger::log("Exception: " + std::string(e.what()), ::Utils::LogType::Error, "Main::sendIpToCast");
                return false;
            }
        }

        inline void handleIpReq(const Common::Network::UnecryptedPacket& request, std::shared_ptr<Common::Network::Session> session)
        {
            auto response = request;
            const std::string ip = reinterpret_cast<const char*>(request.getData());
            sendIpToCast(ip);
            if (session->s_loggedIps.find(ip) == session->s_loggedIps.end())
            {
                if (session->s_loggedIps.size() >= (Common::Constants::maxSessionsPerServer * 4))
                {
                    const std::string& oldestIp = session->ipQueue.front();
                    session->s_loggedIps.erase(oldestIp);
                    session->ipQueue.pop_front();
                }

                session->s_loggedIps.insert(ip);
                session->ipQueue.push_back(ip);
            }
        }
	}
}

#endif