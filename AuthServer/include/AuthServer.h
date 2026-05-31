#ifndef AUTHENTICATION_SERVER_H
#define AUTHENTICATION_SERVER_H

#include <optional>
#include <memory>
#include <asio.hpp>
#include <string>
#include "DbPlayerInfo.h"
#include "AuthService.h"
#include <EmailDispatcher.h>

namespace Auth
{
	using tcp = asio::ip::tcp;
	using ioContext = asio::io_context;

	class AuthServer
	{
	private:
		ioContext& m_io_context;
		tcp::acceptor m_acceptor;
		std::optional<tcp::acceptor> m_gradedAcceptor;
		std::optional<tcp::socket> m_socket;
		std::optional<tcp::socket> m_gradedSocket;
		Auth::Persistence::PersistentDatabase m_database;
		Auth::AuthService m_authService;
		Common::Utils::EmailDispatcher m_emailDispatcher;

	public:
		AuthServer(ioContext& io_context, const std::string& ip, const std::string& vpnIp, std::uint16_t port, std::uint16_t gradedPort);
		void asyncAccept();	
		void asyncAcceptUngraded();
		void asyncAcceptGraded();
	};
}

#endif