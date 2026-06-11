#ifndef MAIN_ROOMCHAT_HANDLER_H
#define MAIN_ROOMCHAT_HANDLER_H

#include "Network/Session.h"
#include "Network/Packet.h"
#include "MainEnums.h"
#include "Managers/RoomsManager.h"
#include "ChatCommands/ChatCommands.h"
#include "Rooms/Room.h"
#include <cstring> 
#include <vector>
#include "Utils.h"

namespace Main
{
	namespace Handlers
	{
		inline void handleRoomChatMessage(const Common::Network::Packet& request, std::shared_ptr<Main::Network::Session> session,
			Main::Network::SessionsManager& sessionsManager,
			Main::Command::ChatCommands& chatCommands, Main::Classes::RoomsManager& roomsManager,
			Main::Persistence::MainScheduler& scheduler, Main::MainServer& mainSv)
		{
			const auto& accountInfo = session->getAccountInfo();
			Common::Network::Packet response;
			response.setCommand(316, getChatGrade(static_cast<Common::Enums::PlayerGrade>(accountInfo.playerGrade)),
				request.getExtra(), request.getOption());
			response.setTcpHeader(request.getSession(), Common::Enums::NO_ENCRYPTION);

			const char* originalMessage = reinterpret_cast<const char*>(request.getData());

			const uint8_t* originalData = request.getData();
			std::size_t dataSize = request.getOption();
			std::string logMessage(originalData, originalData + dataSize);
			scheduler.addRepetitiveCallback(std::source_location::current(), accountInfo.accountID,
				&Main::Persistence::PersistentDatabase::logMessage, accountInfo.accountID, logMessage);

			if (executeCommon(request, session, sessionsManager, chatCommands, roomsManager, scheduler, accountInfo, response, mainSv))
			{
				return;
			}

			const char* senderNickname = accountInfo.nickname;
			std::vector<std::uint8_t> responseData(Common::Constants::maxNicknameSize + request.getOption());
			std::copy(senderNickname, senderNickname + Common::Constants::maxNicknameSize, responseData.begin());
			std::copy(originalMessage, originalMessage + request.getOption(), responseData.begin() + Common::Constants::maxNicknameSize);
			response.setData(responseData.data(), responseData.size());

			if (request.getExtra() == Enums::ChatExtra::CLAN)
			{
				sessionsManager.broadcastToClan(session->getId(), response);
				return;
			}
			else if (auto* room = roomsManager.getRoomByNumber(session->getPlayer().matchContext.roomNumber))
			{
				if (room->isMuted() && session->getAccountInfo().playerGrade < Common::Enums::GRADE_ES)
				{
					session->sendMessage("the room is currently muted");
				}
				else if (session->getPlayer().playerState == Common::Enums::STATE_DYING)
				{
					room->broadcastToDeadExceptSelf(response, session, request.getExtra());
				}
				else if (session->getPlayer().isInMatch())
				{
					room->broadcastToMatchExceptSelf(response, session, request.getExtra());
				}
				else
				{
					room->broadcastOutsideMatchExceptSelf(response, session, request.getExtra());
				}
			}
		}
	}
}

#endif
