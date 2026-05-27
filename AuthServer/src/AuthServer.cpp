#include "../include/AuthServer.h"
#include "../include/AuthSession.h"
#include "../include/Handlers/AuthAuthorizationHandler.h"
#include "../include/Handlers/AuthChannelsHandler.h"


namespace Auth
{
	AuthServer::AuthServer(ioContext& io_context, const std::string& ip, const std::string& vpnIp, std::uint16_t port, std::uint16_t gradedPort)
		: m_io_context(io_context)
		, m_acceptor(io_context, tcp::endpoint(asio::ip::address::from_string(ip), port))
		, m_gradedAcceptor(io_context, tcp::endpoint(asio::ip::address::from_string(vpnIp.empty() ? ip : vpnIp), gradedPort))
		, m_database()
		, m_authService{ m_database, m_emailDispatcher }
	{
		Common::Network::Session::addCallback<Common::Network::PacketType::ENCRYPTED, Auth::Network::Session>(22, [&](const Common::Network::Packet& request,
			std::shared_ptr<Auth::Network::Session> session) { Auth::Handlers::handleAuthUserInformation(request, session, m_authService); });

		Common::Network::Session::addCallback<Common::Network::PacketType::ENCRYPTED, Auth::Network::Session>(23, Auth::Handlers::handleServerChannelsInfo);

		Common::Network::Session::addCallback<Common::Network::PacketType::ENCRYPTED, Auth::Network::Session>(81, Auth::Handlers::handleHwidRetrieval);
	}

	void AuthServer::asyncAccept()
	{
		asyncAcceptGraded();
		asyncAcceptUngraded();
	}

	void AuthServer::asyncAcceptUngraded()
	{
		m_socket.emplace(m_io_context);
		m_acceptor.async_accept(*m_socket, [&](asio::error_code error)
			{
				auto client = std::make_shared<Auth::Network::Session>(std::move(*m_socket), nullptr);
				client->m_checkValidSession = false;
				client->sendConnectionACK(Common::Enums::ServerType::AUTH_SERVER);
				asyncAcceptUngraded();
			});
	}

	void AuthServer::asyncAcceptGraded()
	{
		m_gradedSocket.emplace(m_io_context);
		m_gradedAcceptor.async_accept(*m_gradedSocket, [&](asio::error_code error)
			{
				auto client = std::make_shared<Auth::Network::Session>(std::move(*m_gradedSocket), nullptr);
				client->m_checkValidSession = false;
				client->sendConnectionACK(Common::Enums::ServerType::AUTH_SERVER);
				asyncAcceptGraded();
			});
	}
}
	
