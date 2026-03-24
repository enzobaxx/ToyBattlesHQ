#include <functional>
#include <asio.hpp>

#include "../include/AuthSession.h"
#include "Network/Session.h"
#include "Utils/Logger.h"

namespace Auth
{
	namespace Network
	{
		using tcp = asio::ip::tcp;

		Session::Session(tcp::socket&& socket, std::function<void(std::size_t)> fnct)
			: Common::Network::Session{ std::move(socket), fnct }
		{
		}

		void Session::onPacket(std::vector<std::uint8_t>& data)
		{
			Common::Network::Packet incomingPacket;
			if (!incomingPacket.processIncomingPacket(data.data(), static_cast<std::uint16_t>(data.size()), m_crypt.UserKey))
			{
				closeSocket();
				return;
			}

			const std::uint16_t callbackNum = incomingPacket.getOrder();
			if (!Common::Network::Session::callbacks<Common::Network::PacketType::ENCRYPTED, Session>.contains(callbackNum))
			{
				if (callbackNum)
				{
					Utils::Logger::log("[SEID: " + std::to_string(m_id) + "] No callback for order: " +
						std::to_string(callbackNum), Utils::LogType::Warning, "AuthSession::onPacket");
				}
				m_reader.clear();
				return;
			}

			Common::Network::Session::callbacks<Common::Network::PacketType::ENCRYPTED, Session>[callbackNum]
			(incomingPacket, std::static_pointer_cast<Session>(shared_from_this()));

		}
	};
}
