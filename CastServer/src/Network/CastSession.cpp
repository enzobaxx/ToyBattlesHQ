
#include "Network/Session.h"

#include <functional>
#include <chrono>
#include <vector>
#include <array>

#include <iostream>
#include "asio.hpp"
#include <Utils/Parser.h>

#include "../../../MainServer/include/Structures/AccountInfo/MainAccountInfo.h"
#include "../../include/Network/CastSession.h"
#include <Utils/Utils.h>
#include "../include/Enums/ExtrasEnums.h"
#include <Utils/Logger.h>
#include <Handlers/SimpleHandlers.h>


namespace Cast
{
	namespace Network
	{
		std::uint32_t Session::getRoomNumber() const
		{
			return m_roomNumber;
		}

		void Session::setRoomNumber(std::uint32_t roomNum)
		{
			m_roomNumber = roomNum;
		}

		bool Session::isInMatch() const
		{
			return m_isInMatch;
		}

		void Session::setIsInMatch(bool val)
		{
			m_isInMatch = val;
		}

		void Session::onPacket(std::vector<std::uint8_t>& data)
		{
			Common::Network::UnecryptedPacket incomingPacket;
			if (!incomingPacket.processIncomingPacket(data.data(), static_cast<std::uint16_t>(data.size())))
			{
				return;
			}

			const std::uint16_t callbackNum = incomingPacket.getOrder();
			if (!Common::Network::Session::callbacks<Common::Network::PacketType::UNECRYPTED, Session>.contains(callbackNum))
			{
				Utils::Logger::log("[SEID: " + std::to_string(m_id) + "] No callback for order: " + std::to_string(callbackNum),
					Utils::LogType::Error, "CastSession::onPacket");
				m_reader.resize(0);
				return;
			}

			Common::Network::Session::callbacks<Common::Network::PacketType::UNECRYPTED, Session>[callbackNum](incomingPacket, std::static_pointer_cast<Session>(shared_from_this()));
		}
	}
}
