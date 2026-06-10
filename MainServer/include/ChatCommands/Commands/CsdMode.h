#ifndef CSD_SIMPLEMODE_CMD_HEADER
#define CSD_SIMPLEMODE_CMD_HEADER


#include "ChatCommands/ICommand.h"
#include "ChatCommands/ChatCommands.h"
#include "MainServer.h"

namespace Main
{
	namespace Command
	{
		struct CsdMode final : public ICommand
		{
			explicit CsdMode(const Common::Enums::PlayerGrade requiredGrade)
				: ICommand{ requiredGrade, "/csdmode" }
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
					session->sendMessage(room->setCsdMode() ? "CSD mode ENABLED" : "CSD mode DISABLED");
				}
			}
		};

		REGISTER_CMD(CsdMode, Common::Enums::PlayerGrade::GRADE_NORMAL)
	}
}

#endif


