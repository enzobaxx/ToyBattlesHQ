#ifndef MAIN_WHISPERCHAT_HANDLER_H
#define MAIN_WHISPERCHAT_HANDLER_H

#include "Network/Session.h"
#include "Network/Packet.h"
#include "MainEnums.h"
#include "Managers/RoomsManager.h"
#include "ChatCommands/ChatCommands.h"
#include "Rooms/Room.h"
#include <cstring> 
#include <vector>

namespace Main
{
	namespace Handlers
	{
		inline void handleWhisperMessage(const Common::Network::Packet& request, std::shared_ptr<Main::Network::Session> session,
			Main::Network::SessionsManager& sessionsManager, Common::Network::Packet& response, Main::Persistence::MainScheduler& scheduler)
		{
			// Rationale: the client sends a nickname if the receiver is outside a room, otherwise it sends its uniqueId
			char receiverNickname[16];
			std::memcpy(receiverNickname, request.getData(), Common::Constants::maxNicknameSize);

			bool inRoom = false;
			auto targetSession = sessionsManager.findSessionByName(receiverNickname);
			if (!targetSession)
			{
				const Main::Structures::UniqueId uniqueId = Main::Details::parseData<Main::Structures::UniqueId>(request);
				targetSession = sessionsManager.getSessionBySessionId(uniqueId.session);
				inRoom = true;
			}

			const auto& accountInfo = session->getAccountInfo();
			const char* senderNickname = accountInfo.nickname;
			const uint8_t* uid = reinterpret_cast<const uint8_t*>(&(accountInfo.uniqueId));
			const char* message = reinterpret_cast<const char*>(request.getData() + (inRoom ? sizeof(Main::Structures::UniqueId) : Common::Constants::maxNicknameSize));

			std::size_t dataSize = request.getOption();
			std::string prefix = "[Whisper To: " + std::string(receiverNickname) + "]: ";
			std::string logMessage = prefix + std::string(message, message + dataSize);
			scheduler.addRepetitiveCallback(std::source_location::current(), accountInfo.accountID,
				&Main::Persistence::PersistentDatabase::logMessage, accountInfo.accountID, logMessage);

			std::vector<std::uint8_t> responseData(Common::Constants::maxNicknameSize + request.getOption() + sizeof(accountInfo.uniqueId));
			std::copy(uid, uid + sizeof(accountInfo.uniqueId), responseData.begin());
			std::copy(senderNickname, senderNickname + Common::Constants::maxNicknameSize, responseData.begin() + sizeof(accountInfo.uniqueId));
			std::copy(message, message + request.getOption(), responseData.begin() + Common::Constants::maxNicknameSize + sizeof(accountInfo.uniqueId));
			response.setData(responseData.data(), responseData.size());

			if (targetSession)
			{
				if (session->getPlayer().getSocialInfo().hasBlocked(targetSession->getAccountInfo().accountID))
				{
					response.setExtra(Enums::WhisperExtra::WHISPER_SENDER_BLOCKED_RECEIVER);
				}
				else if (targetSession->getPlayer().getSocialInfo().hasBlocked(accountInfo.accountID))
				{
					response.setExtra(Enums::WhisperExtra::WHISPER_RECEIVER_BLOCKED_SENDER);
				}
				else
				{
					sessionsManager.sendTo(targetSession->getId(), response);
					response.setExtra(Enums::WhisperExtra::WHISPER_SENT);
				}
			}
			else
			{
				response.setExtra(Enums::WhisperExtra::RECEIVER_OFFLINE);
			}

			// Auto-sends the message, sort of a confirmation
			response.setOrder(315);
			response.setMission(1);  // 1 = WhisperConfirmation
			response.setData(nullptr, 0);
			session->asyncWrite(response);
		}
	}
}

#endif
