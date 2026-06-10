#ifndef ASSASSINMODE_SIMPLECOMMAND_HEADER
#define ASSASSINMODE_SIMPLECOMMAND_HEADER


#include "ChatCommands/ICommand.h"
#include "ChatCommands/ChatCommands.h"
#include "MainServer.h"

namespace Main
{
	namespace Command
	{
		struct AssassinMode final : public ICommand
		{
			explicit AssassinMode(const Common::Enums::PlayerGrade requiredGrade)
				: ICommand{ requiredGrade, "/assassinmode" }
			{
			}

			void execute(const std::string&, std::shared_ptr<Main::Network::Session> session, MN::SessionsManager& sessionsManager, 
				MC::RoomsManager& roomsManager, MP::MainScheduler&, std::uint32_t roomNumber,
				Main::MainServer& mainSv) override
			{
				if (Main::Classes::Room* room = roomsManager.getRoomByNumber(roomNumber))
				{
					if (!room->isHost(session->getAccountInfo().uniqueId))
					{
						return;
					}
					if (room->hasMatchStarted())
					{
						session->sendMessage("This mode can only be enabled before the match starts!");
						return;
					}
					auto settings = room->getRoomSettings();
					if (settings.mode == Common::Enums::Elimination)
					{
						bool isAssassinMode = room->setAssassinMode();
						session->sendMessage(isAssassinMode ? "Assassin mode ENABLED" : "Assassin mode DISABLED");
					}
					else
					{
						session->sendMessage("ERROR: This mode currently requires elimination and magic paper land");
					}
				}
			}
		};

		REGISTER_CMD(AssassinMode, Common::Enums::PlayerGrade::GRADE_NORMAL)

	}
}

#endif


