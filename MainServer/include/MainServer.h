
#ifndef MAIN_SERVER_H
#define MAIN_SERVER_H

#include <optional>
#include <asio.hpp>

#include "Persistence/MainScheduler.h"
#include "Network/MainSessionManager.h"
#include "ChatCommands/ChatCommands.h"
#include "Managers/RoomsManager.h"
#include "Managers/PartiesManager.h"

#include <iostream>
#include <boost/asio/io_context.hpp>
#include <boost/asio.hpp>

#include <AntiCheat/AntiCheat.h>
#include <Managers/ReportManager.h>
#include <EmailDispatcher.h>

namespace Main
{
	using tcp = asio::ip::tcp;
	using ioContext = asio::io_context;

	class MainServer
	{
	private:
		ioContext& m_io_context;
		boost::asio::io_context& m_io_context_boost;
		tcp::acceptor m_acceptor;
		std::optional<tcp::socket> m_socket;

		std::uint16_t m_serverId{};
		bool m_isServerOffline{};
		bool m_roomCreationEnabled{ true };
		bool m_isPublic{ true };
		std::uint64_t m_timeSinceLastRestart{};

		Main::Persistence::PersistentDatabase m_database;
		Main::Persistence::MainScheduler m_scheduler;
		Main::Network::SessionsManager m_sessionsManager;
		Main::Classes::RoomsManager m_roomsManager;
		Main::Classes::PartiesManager m_partiesManager;
		Main::Command::ChatCommands m_chatCommands;
		Main::Classes::ReportManager m_reportManager;
		std::unordered_map<std::uint32_t, std::function<void(std::shared_ptr<Main::Network::Session>)>> m_generalItemCallbacks;
		std::unordered_map<std::uint32_t, std::function<void(std::shared_ptr<Main::Network::Session>)>> m_cashItemsCallbacks;
		std::unordered_map<std::uint32_t, std::function<bool(std::shared_ptr<Main::Network::Session>, const Main::Structures::ItemSerialInfo&,
			const Common::Network::Packet&)>> m_inMatchItemIds;

		// For auth server communication
		tcp::acceptor m_ipcServerAcceptor;
		std::optional<tcp::socket> m_ipcSocket; 

		// Website communication
		boost::asio::ip::tcp::acceptor m_httpServerAcceptor;
		std::optional<boost::asio::ip::tcp::socket> m_httpSocket;

		// Events
		Main::Structures::EventMissionInfo m_eventMissionInfo;
		Main::Structures::EventMissionInfo m_tradeSystemEvent;
		Main::Structures::CapsuleListDatabase m_capsuleSaleEvent;
		Main::Structures::ExpMpBonusInfo m_expMpEvent;

		// Ac
		Ac::AntiCheatManager m_acManager;

	public:
		MainServer(ioContext& io_context, boost::asio::io_context& boost_io_context, const Common::Utils::MainSetup& mainSetup, std::uint32_t websitePort);
		void asyncAccept();
		void asyncAcceptIpcServer();
		void asyncAcceptHttp(const std::string& websiteIp);
		void initializeGeneralItemCallbacks();
		constexpr void setServerOffline(bool v) { m_isServerOffline = v; }
		constexpr void setRoomCreationTo(bool v) { m_roomCreationEnabled = v; }
		constexpr bool getRoomCreation() const noexcept { return m_roomCreationEnabled; };
		Main::Classes::ReportManager& getReportManager() { return m_reportManager; }
		Main::Classes::PartiesManager getPartiesManager() const noexcept { return m_partiesManager; }

		// Emails
		Common::Utils::EmailDispatcher emailDispatcher;
	};
}

#endif
