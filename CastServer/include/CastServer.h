
#ifndef CAST_SERVER_H
#define CAST_SERVER_H

#include <cstdint>
#include <asio.hpp>
#include <optional>
#include "Classes/RoomsManager.h"
#include "Network/SessionsManager.h"
#include <AntiCheat/AntiCheat.h>
#include <Network/Session.h>

namespace Cast
{
	using tcp = asio::ip::tcp;
	using ioContext = asio::io_context;

	class CastServer
	{
	private:
		ioContext& m_io_context;
		tcp::acceptor m_acceptor;
		std::optional<tcp::socket> m_socket;
		std::uint16_t m_serverId;
		Cast::Network::SessionsManager m_sessionsManager{};
		Cast::Classes::RoomsManager m_roomsManager{};
		std::shared_ptr<asio::steady_timer> m_positionTimer;
		std::shared_ptr<asio::steady_timer> m_sessionTimer;
		tcp::acceptor m_mainServerAcceptor;
		std::optional<tcp::socket> m_mainSocket;
		Ac::AntiCheatManager m_acManager{ false };
		std::shared_ptr<Common::Network::Session> m_mainIpcSession;

		void registerIpcCallbacks();
		void registerRoomCallbacks();
		void registerMatchCallbacks();
		void registerCombatCallbacks();
		void connectToMainIpc();
		void scheduleIpcReconnect();

	public:
		CastServer(ioContext& io_context, const std::string& serverIp, std::uint16_t port, std::uint16_t mainPort, std::uint16_t serverId);
		void asyncAccept();
		void asyncAcceptMainServer();
		void tickPositionFlush();
		void tickSessionHeartbeat();
	};
}

#endif
