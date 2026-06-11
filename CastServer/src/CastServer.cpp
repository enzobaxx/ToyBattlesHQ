#include "../include/CastServer.h"
#include "Network/Session.h"
#include "../include/Handlers/SimpleHandlers.h"
#include "../include/Handlers/PlayerPositionHandler.h"
#include "../include/Handlers/IpcMainHandlers.h"
#include "../include/Handlers/WeaponKillHandlers.h"
#include <chrono>

namespace Cast
{
	void CastServer::tickPositionFlush()
	{
		for (auto& room : m_roomsManager.getAllRooms())
		{
			room->flushPendingPositions();
		}

		m_positionTimer->expires_after(std::chrono::milliseconds(80));
		m_positionTimer->async_wait([this](auto) { tickPositionFlush(); });
	}

	void CastServer::connectToMainIpc()
	{
		auto socket = std::make_shared<asio::ip::tcp::socket>(m_io_context);
		auto resolver = std::make_shared<asio::ip::tcp::resolver>(m_io_context);
		auto& info = Common::Utils::SetupParser::getInstance().getSelfMainServerInfo();

		resolver->async_resolve(info.ip, std::to_string(info.ipcPort),
			[this, socket, resolver](const asio::error_code& ec, asio::ip::tcp::resolver::results_type endpoints)
			{
				if (ec) { scheduleIpcReconnect(); return; }
				asio::async_connect(*socket, endpoints,
					[this, socket](const asio::error_code& ec, const asio::ip::tcp::endpoint&)
					{
						if (ec) { scheduleIpcReconnect(); return; }
						m_mainIpcSession = std::make_shared<Common::Network::Session>(
							std::move(*socket),
							[this](std::size_t) { scheduleIpcReconnect(); });
						m_mainIpcSession->m_checkValidSession = false;
						m_mainIpcSession->sendConnectionACK(Common::Enums::IPC_SERVER);
					});
			});
	}

	void CastServer::scheduleIpcReconnect()
	{
		m_mainIpcSession.reset();
		auto timer = std::make_shared<asio::steady_timer>(m_io_context);
		timer->expires_after(std::chrono::seconds(5));
		timer->async_wait([this, timer](auto) { connectToMainIpc(); });
	}

	CastServer::CastServer(ioContext& io_context, const std::string& serverIp, std::uint16_t port, std::uint16_t mainPort, std::uint16_t serverId)
		: m_io_context{ io_context }
		, m_acceptor{ io_context, tcp::endpoint(asio::ip::address::from_string(serverIp), port) }
		, m_serverId{ serverId }
		, m_mainServerAcceptor{ io_context, tcp::endpoint(asio::ip::address::from_string(Common::Utils::SetupParser::getInstance().getSelfCastServerInfo().ip), mainPort) }
	{
		m_sessionsManager.setRoomsManager(&m_roomsManager);
		m_positionTimer = std::make_shared<asio::steady_timer>(m_io_context);
		tickPositionFlush();
		connectToMainIpc();

		registerIpcCallbacks();
		registerRoomCallbacks();
		registerMatchCallbacks();
		registerCombatCallbacks();
	}

	void CastServer::registerIpcCallbacks()
	{
		namespace CN = Common::Network;

		CN::Session::addCallback<CN::PacketType::UNECRYPTED, CN::Session>(Common::Constants::M2C_mapId,
			[&](const CN::UnecryptedPacket& request, std::shared_ptr<CN::Session> session)
			{ Cast::Handlers::handleMapId(request, session, m_roomsManager); });

		CN::Session::addCallback<CN::PacketType::UNECRYPTED, CN::Session>(Common::Constants::M2C_assassinModeInfo,
			[&](const CN::UnecryptedPacket& request, std::shared_ptr<CN::Session> session)
			{ Cast::Handlers::handleAssassinMode(request, session, m_roomsManager); });

		CN::Session::addCallback<CN::PacketType::UNECRYPTED, CN::Session>(Common::Constants::M2C_playerTeamInfoBatch,
			[&](const CN::UnecryptedPacket& request, std::shared_ptr<CN::Session> session)
			{ Cast::Handlers::handlePlayerTeamInfoBatch(request, session, m_roomsManager); });

		CN::Session::addCallback<CN::PacketType::UNECRYPTED, CN::Session>(Common::Constants::M2C_roomNumber,
			[&](const CN::UnecryptedPacket& request, std::shared_ptr<CN::Session> session)
			{ Cast::Handlers::handleRoomNumber(request, session, m_roomsManager); });

		CN::Session::addCallback<CN::PacketType::UNECRYPTED, CN::Session>(Common::Constants::A2M_passIp,
			[&](const CN::UnecryptedPacket& request, std::shared_ptr<CN::Session> session)
			{ Cast::Handlers::handleIpReq(request, session); });

		CN::Session::addCallback<CN::PacketType::UNECRYPTED, CN::Session>(Common::Constants::M2C_Invisibility,
			[&](const CN::UnecryptedPacket& request, std::shared_ptr<CN::Session> session)
			{ Cast::Handlers::handleInvisibleCmd(request, session, m_roomsManager, m_sessionsManager); });

		CN::Session::addCallback<CN::PacketType::UNECRYPTED, CN::Session>(Common::Constants::C2M_CloseSocketReq,
			[&](const CN::UnecryptedPacket& request, std::shared_ptr<CN::Session> session)
			{ Cast::Handlers::closeSocketAfterMain(request, session, m_sessionsManager); });
	}

	void CastServer::registerRoomCallbacks()
	{
		using namespace Cast::Network;
		namespace CN = Common::Network;

		CN::Session::addCallback<CN::PacketType::UNECRYPTED, Session>(252,
			[&](const CN::UnecryptedPacket& request, std::shared_ptr<Session> session)
			{ Cast::Handlers::connectionHandler(request, session, m_sessionsManager, m_io_context, m_mainIpcSession); });

		CN::Session::addCallback<CN::PacketType::UNECRYPTED, Session>(71,
			[&](const CN::UnecryptedPacket& request, std::shared_ptr<Session> session)
			{ Cast::Handlers::pongHandler(request, session, m_roomsManager, m_sessionsManager, m_serverId); });

		CN::Session::addCallback<CN::PacketType::UNECRYPTED, Session>(140,
			[&](const CN::UnecryptedPacket& request, std::shared_ptr<Session> session)
			{ m_roomsManager.switchRoomJoinOrExit(session, request.getSession()); });

		CN::Session::addCallback<CN::PacketType::UNECRYPTED, Session>(277,
			[&](const CN::UnecryptedPacket& request, std::shared_ptr<Session> session)
			{ m_roomsManager.addRoom(std::make_shared<Cast::Classes::Room>(session->getId(), session), session->getId()); });

		CN::Session::addCallback<CN::PacketType::UNECRYPTED, Session>(279,
			[&](const CN::UnecryptedPacket& request, std::shared_ptr<Session> session)
			{ m_roomsManager.switchRoomJoinOrExit(session); });
	}

	void CastServer::registerMatchCallbacks()
	{
		using namespace Cast::Network;
		namespace CN = Common::Network;

		CN::Session::addCallback<CN::PacketType::UNECRYPTED, Session>(257,
			[&](const CN::UnecryptedPacket& request, std::shared_ptr<Session> session)
			{ Cast::Handlers::handleMatchInitialLoading(request, session, m_roomsManager, m_serverId, m_mainIpcSession); });

		CN::Session::addCallback<CN::PacketType::UNECRYPTED, Session>(258,
			[&](const CN::UnecryptedPacket& request, std::shared_ptr<Session> session)
			{ Cast::Handlers::handleMatchStart(request, session, m_roomsManager); });

		CN::Session::addCallback<CN::PacketType::UNECRYPTED, Session>(254,
			[&](const CN::UnecryptedPacket& request, std::shared_ptr<Session> session)
			{ m_roomsManager.endMatch(session->getId()); });

		CN::Session::addCallback<CN::PacketType::UNECRYPTED, Session>(256,
			[&](const CN::UnecryptedPacket& request, std::shared_ptr<Session> session)
			{ Cast::Handlers::handleMatchLeave(request, session, m_roomsManager); });

		CN::Session::addCallback<CN::PacketType::UNECRYPTED, Session>(259,
			[&](const CN::UnecryptedPacket&, std::shared_ptr<Session>) {});

		CN::Session::addCallback<CN::PacketType::UNECRYPTED, Session>(255,
			[&](const CN::UnecryptedPacket&, std::shared_ptr<Session>) {});

		CN::Session::addCallback<CN::PacketType::UNECRYPTED, Session>(284,
			[&](const CN::UnecryptedPacket& request, std::shared_ptr<Session> session)
			{ m_roomsManager.broadcastToMatch(session->getId(), const_cast<CN::UnecryptedPacket&>(request)); });

		CN::Session::addCallback<CN::PacketType::UNECRYPTED, Session>(280,
			[&](const CN::UnecryptedPacket& request, std::shared_ptr<Session> session)
			{ Cast::Handlers::handleVoiceMessage(request, session, m_roomsManager, m_serverId); });

		CN::Session::addCallback<CN::PacketType::UNECRYPTED, Session>(166,
			[&](const CN::UnecryptedPacket& request, std::shared_ptr<Session> session)
			{
				if (m_roomsManager.getModeOf(session->getId()) != Common::Enums::BossBattle && request.getDataSize() != 0) return;
				session->isDead = false;
				m_roomsManager.playerForwardToHost(request.getSession(), session->getId(), const_cast<CN::UnecryptedPacket&>(request));
			});

		CN::Session::addCallback<CN::PacketType::UNECRYPTED, Session>(79,
			[&](const CN::UnecryptedPacket& request, std::shared_ptr<Session> session)
			{ m_roomsManager.playerForwardToHost(request.getSession(), session->getId(), const_cast<CN::UnecryptedPacket&>(request)); });

		CN::Session::addCallback<CN::PacketType::UNECRYPTED, Session>(309,
			[&](const CN::UnecryptedPacket& request, std::shared_ptr<Session> session)
			{ m_roomsManager.hostForwardToPlayer(session->getId(), request.getSession(), const_cast<CN::UnecryptedPacket&>(request)); });

		CN::Session::addCallback<CN::PacketType::UNECRYPTED, Session>(408,
			[&](const CN::UnecryptedPacket& request, std::shared_ptr<Session> session)
			{ m_roomsManager.hostForwardToPlayer(session->getId(), request.getSession(), const_cast<CN::UnecryptedPacket&>(request)); });

		CN::Session::addCallback<CN::PacketType::UNECRYPTED, Session>(306,
			[&](const CN::UnecryptedPacket& request, std::shared_ptr<Session> session)
			{ Cast::Handlers::roomInfoJoinHandler(request, session, m_roomsManager, m_sessionsManager); });
	}

	void CastServer::registerCombatCallbacks()
	{
		using namespace Cast::Network;
		namespace CN = Common::Network;

		CN::Session::addCallback<CN::PacketType::UNECRYPTED, Session>(281,
			[&](const CN::UnecryptedPacket& request, std::shared_ptr<Session> session)
			{ Cast::Handlers::handlePlayerPosition(request, session, m_roomsManager, m_serverId, m_sessionsManager, m_acManager); });

		CN::Session::addCallback<CN::PacketType::UNECRYPTED, Session>(253,
			[&](const CN::UnecryptedPacket& request, std::shared_ptr<Session> session)
			{ Cast::Handlers::handleCrash(request, session, m_roomsManager, m_serverId); });

		CN::Session::addCallback<CN::PacketType::UNECRYPTED, Session>(276,
			[&](const CN::UnecryptedPacket& request, std::shared_ptr<Session> session)
			{ Cast::Handlers::handlePlayerRespawn(request, session, m_roomsManager, m_sessionsManager, m_acManager, m_mainIpcSession); });

		for (std::size_t order : { 146, 147, 149, 151, 152, 153, 154 })
		{
			CN::Session::addCallback<CN::PacketType::UNECRYPTED, Session>(order,
				[&](const CN::UnecryptedPacket& request, std::shared_ptr<Session> session)
				{
					if (session->m_team == Common::Enums::TEAM_OBSERVER) return;
					m_roomsManager.playerForwardToHost(request.getSession(), session->getId(), const_cast<CN::UnecryptedPacket&>(request));
				});
		}

		CN::Session::addCallback<CN::PacketType::UNECRYPTED, Session>(272,
			[&](const CN::UnecryptedPacket& request, std::shared_ptr<Session> session)
			{ m_roomsManager.broadcastToMatch(session->getId(), const_cast<CN::UnecryptedPacket&>(request)); });

		CN::Session::addCallback<CN::PacketType::UNECRYPTED, Session>(275,
			[&](const CN::UnecryptedPacket& request, std::shared_ptr<Session> session)
			{ session->asyncWrite(const_cast<CN::UnecryptedPacket&>(request)); });

		for (std::size_t order : { 266, 269 })
		{
			CN::Session::addCallback<CN::PacketType::UNECRYPTED, Session>(order,
				[&](const CN::UnecryptedPacket& request, std::shared_ptr<Session> session)
				{ Cast::Handlers::handleSpecialWeaponDamage(request, session, m_roomsManager, m_sessionsManager, m_acManager, m_mainIpcSession); });
		}

		for (std::size_t order : { 265, 267, 268, 270 })
		{
			CN::Session::addCallback<CN::PacketType::UNECRYPTED, Session>(order,
				[&](const CN::UnecryptedPacket& request, std::shared_ptr<Session> session)
				{ Cast::Handlers::handleNormalWeaponDamage(request, session, m_roomsManager, m_sessionsManager, m_acManager, m_mainIpcSession); });
		}

		CN::Session::addCallback<CN::PacketType::UNECRYPTED, Session>(264,
			[&](const CN::UnecryptedPacket& request, std::shared_ptr<Session> session)
			{ Cast::Handlers::handleExplosiveDamage(request, session, m_roomsManager, m_sessionsManager, m_mainIpcSession); });

		for (std::size_t order : { 90, 165, 260, 261, 271 })
		{
			CN::Session::addCallback<CN::PacketType::UNECRYPTED, Session>(order,
				[&](const CN::UnecryptedPacket& request, std::shared_ptr<Session> session)
				{ m_roomsManager.broadcastToMatch(session->getId(), const_cast<CN::UnecryptedPacket&>(request)); });
		}

		for (std::size_t order : { 155, 156, 78 })
		{
			CN::Session::addCallback<CN::PacketType::UNECRYPTED, Session>(order,
				[&](const CN::UnecryptedPacket& request, std::shared_ptr<Session> session)
				{ m_roomsManager.playerForwardToHost(request.getSession(), session->getId(), const_cast<CN::UnecryptedPacket&>(request)); });
		}

		CN::Session::addCallback<CN::PacketType::UNECRYPTED, Session>(262,
			[&](const CN::UnecryptedPacket& request, std::shared_ptr<Session> session)
			{ Cast::Handlers::handleItemPickup<Common::Enums::HOST>(request, session, m_roomsManager); });

		CN::Session::addCallback<CN::PacketType::UNECRYPTED, Session>(263,
			[&](const CN::UnecryptedPacket& request, std::shared_ptr<Session> session)
			{ Cast::Handlers::handleZombieAbility<Common::Enums::HOST>(request, session, m_roomsManager); });

		for (std::size_t order : { 96, 94 })
		{
			CN::Session::addCallback<CN::PacketType::UNECRYPTED, Session>(order,
				[&](const CN::UnecryptedPacket& request, std::shared_ptr<Session> session)
				{ Cast::Handlers::handleItemPickup<Common::Enums::NON_HOST>(request, session, m_roomsManager); });
		}

		CN::Session::addCallback<CN::PacketType::UNECRYPTED, Session>(102,
			[&](const CN::UnecryptedPacket& request, std::shared_ptr<Session> session)
			{ Cast::Handlers::handleZombieAbility<Common::Enums::NON_HOST>(request, session, m_roomsManager); });

		CN::Session::addCallback<CN::PacketType::UNECRYPTED, Session>(282,
			[&](const CN::UnecryptedPacket& request, std::shared_ptr<Session> session)
			{ m_roomsManager.broadcastToMatchExceptSelf(session->getId(), const_cast<CN::UnecryptedPacket&>(request)); });

		for (std::size_t order : { 285, 286 })
		{
			CN::Session::addCallback<CN::PacketType::UNECRYPTED, Session>(order,
				[&](const CN::UnecryptedPacket& request, std::shared_ptr<Session> session)
				{
					if (m_roomsManager.getModeOf(session->getId()) == Common::Enums::AiBattle)
					{
						m_roomsManager.broadcastToMatch(session->getId(), const_cast<CN::UnecryptedPacket&>(request));
					}
				});
		}

		for (std::size_t order : { 326, 328, 331, 304 })
		{
			CN::Session::addCallback<CN::PacketType::UNECRYPTED, Session>(order,
				[&](const CN::UnecryptedPacket& request, std::shared_ptr<Session> session)
				{ Cast::Handlers::handleBossBattleForwardPacket(request, session, m_roomsManager); });
		}
	}

	void CastServer::asyncAccept()
	{
		m_socket.emplace(m_io_context);
		m_acceptor.async_accept(*m_socket, [&](asio::error_code error)
			{
				auto client = std::make_shared<Cast::Network::Session>(std::move(*CastServer::m_socket),
					std::bind(&Cast::Network::SessionsManager::removeSession, &m_sessionsManager, std::placeholders::_1));
				client->m_checkValidSession = true;
				client->sendConnectionACK(Common::Enums::CAST_SERVER);
				asyncAccept();
			});
	}

	void CastServer::asyncAcceptMainServer()
	{
		m_mainSocket.emplace(m_io_context);
		m_mainServerAcceptor.async_accept(*m_mainSocket, [this](asio::error_code error)
			{
				if (!error)
				{
					asio::ip::tcp::endpoint remoteEndpoint = m_mainSocket->remote_endpoint();

					if (remoteEndpoint.address() == asio::ip::address::from_string(Common::Utils::SetupParser::getInstance().getSelfMainServerInfo().ip) ||
						remoteEndpoint.address() == asio::ip::address::from_string("::1"))
					{
						auto mainIpc = std::make_shared<Common::Network::Session>(std::move(*m_mainSocket), nullptr);
						mainIpc->m_checkValidSession = false;
						mainIpc->sendConnectionACK(Common::Enums::IPC_SERVER);
					}
					else
					{
						::Utils::Logger::log("Unauthorized connection attempt from IP " + remoteEndpoint.address().to_string(), ::Utils::LogType::Warning);
						m_mainSocket->close();
					}
				}
				asyncAcceptMainServer();
			});
	}
}
