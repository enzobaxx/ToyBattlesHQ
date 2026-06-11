#ifndef MAIN_CHATUTILS_HANDLER_H
#define MAIN_CHATUTILS_HANDLER_H

#include "Network/Session.h"
#include "Network/Packet.h"
#include "MainEnums.h"
#include "Managers/RoomsManager.h"
#include "ChatCommands/ChatCommands.h"
#include "Rooms/Room.h"
#include <cstring> 
#include <vector>
#include "WhisperMessage.h"

namespace Main
{
	namespace Handlers
	{
		inline Main::Enums::ChatGrade getChatGrade(Common::Enums::PlayerGrade playerGrade)
		{
 		   if (playerGrade == Main::Enums::PlayerGrade::GRADE_NORMAL)
		    {
		        return static_cast<Main::Enums::ChatGrade>(playerGrade - 1);
		    }
		    else if (playerGrade == Main::Enums::PlayerGrade::GRADE_MOD || playerGrade == Main::Enums::PlayerGrade::GRADE_ES)
		    {
		        return static_cast<Main::Enums::ChatGrade>(playerGrade - 2);
		    }
		    else if (playerGrade == Main::Enums::PlayerGrade::GRADE_TESTER)
		    {
		        return Main::Enums::ChatGrade::CHAT_TESTER;
		    }
		    else if (playerGrade == Main::Enums::PlayerGrade::GRADE_GM)
 		   {
		        return Main::Enums::ChatGrade::CHAT_GM;
		    }
		    return Main::Enums::ChatGrade::CHAT_NORMAL;
		}

		inline void executeCommand(std::shared_ptr<Main::Network::Session> session, const Common::Network::Packet& request, Common::Network::Packet& response,
			Main::Classes::RoomsManager& roomsManager, Main::Command::ChatCommands& chatCommands, Main::Network::SessionsManager& sessionsManager,
			Main::Persistence::MainScheduler& scheduler, const Main::Network::Session::AccountInfo& accountInfo, Main::MainServer& mainSv)
		{
			const std::string command{ reinterpret_cast<const char*>(request.getData() + 1), static_cast<std::size_t>(request.getOption() - 1) };
			if (command == "commands" || command == "?")
			{
				Main::Command::ChatCommands::showUsages(session, response, static_cast<Common::Enums::PlayerGrade>(accountInfo.playerGrade));
				return;
			}
			const std::string commandName = command.substr(0, command.find(' '));
			Main::Command::ChatCommands::executeCommand(commandName, command, session, sessionsManager, roomsManager, scheduler,
				session->getPlayer().getRoomNumber(), mainSv);
		}

		inline bool executeCommon(const Common::Network::Packet& request, std::shared_ptr<Main::Network::Session> session,
			Main::Network::SessionsManager& sessionsManager,
			Main::Command::ChatCommands& chatCommands, Main::Classes::RoomsManager& roomsManager, Main::Persistence::MainScheduler& scheduler,
			const Main::Network::Session::AccountInfo& accountInfo, Common::Network::Packet& response, Main::MainServer& mainSv)
		{
			if (request.getExtra() == Enums::ChatExtra::COMMAND)
			{
				executeCommand(session, request, response, roomsManager, chatCommands, sessionsManager, scheduler, accountInfo, mainSv);
				return true;
			}
			else if (session->getPlayer().getModerationInfo().isMuted)
			{
				session->sendMessage("you have been muted by a moderator.");
				return true;
			}
			else if (request.getExtra() == Enums::ChatExtra::WHISPER)
			{
				handleWhisperMessage(request, session, sessionsManager, response, scheduler);
				return true;
			}
			return false;
		}
	}
}

#endif
