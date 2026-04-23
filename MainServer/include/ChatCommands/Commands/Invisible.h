#ifndef INVISIBLE_COMMAND_H
#define INVISIBLE_COMMAND_H


#include "../ICommand.h"
#include "../ChatCommands.h"
#include "../../MainServer.h"
#include "../../Detail/IpcUtils.h"

namespace Main
{
	namespace Command
	{
		struct InvisibleOn final : public ICommand
		{
			explicit InvisibleOn(const Common::Enums::PlayerGrade requiredGrade)
				: ICommand{ requiredGrade, "/invisibleon" }
			{
			}

			void execute(const std::string&, std::shared_ptr<Main::Network::Session> session, MN::SessionsManager& sessionsManager, MC::RoomsManager& roomsManager,
				MP::MainScheduler&, std::uint32_t roomNumber,
				Main::MainServer& mainSv) override
			{
				if (roomNumber >= Common::Constants::clanRoomNumberStart)
				{
					session->sendMessage("Error: this command is currently disabled for clanwars!");
					return;
				}
				if (Main::Classes::Room* room = roomsManager.getRoomByNumber(roomNumber))
				{
					if (!room->hasMatchStarted())
					{
						session->sendMessage("error: must use this command inside a match");
						return;
					}
					else
					{
						Main::Ipc::M2C_sendInvisibilityCommand(session->getId(), Main::Ipc::SELF_INVISIBLE);
					}
				}
				else
				{
					session->sendMessage("error: must be in a room");
				}
			}
		};
		REGISTER_CMD(InvisibleOn, Common::Enums::PlayerGrade::GRADE_TESTER)

		struct InvisibleOff final : public ICommand
		{
			explicit InvisibleOff(const Common::Enums::PlayerGrade requiredGrade)
				: ICommand{ requiredGrade, "/invisibleff" }
			{
			}

			void execute(const std::string&, std::shared_ptr<Main::Network::Session> session, MN::SessionsManager& sessionsManager, MC::RoomsManager& roomsManager, 
				MP::MainScheduler&, std::uint32_t roomNumber,
				Main::MainServer& mainSv) override
			{
				if (Main::Classes::Room* room = roomsManager.getRoomByNumber(roomNumber))
				{
					if (!room->hasMatchStarted())
					{
						session->sendMessage("error: must use this command inside a match");
						return;
					}
					else
					{
						Main::Ipc::M2C_sendInvisibilityCommand(session->getId(), Main::Ipc::SELF_NOT_INVISIBLE);
					}
				}
				else
				{
					session->sendMessage("error: must be in a room");
				}
			}
		};
		REGISTER_CMD(InvisibleOff, Common::Enums::PlayerGrade::GRADE_TESTER)

		struct InvisibleAllOn final : public ICommand
		{
			explicit InvisibleAllOn(const Common::Enums::PlayerGrade requiredGrade)
				: ICommand{ requiredGrade, "/invisibleallon" }
			{
			}

			void execute(const std::string&, std::shared_ptr<Main::Network::Session> session, MN::SessionsManager& sessionsManager, MC::RoomsManager& roomsManager, 
				MP::MainScheduler&, std::uint32_t roomNumber,
				Main::MainServer& mainSv) override
			{
				if (Main::Classes::Room* room = roomsManager.getRoomByNumber(roomNumber))
				{
					if (!room->hasMatchStarted())
					{
						session->sendMessage("error: must use this command inside a match");
						return;
					}
					else
					{
						Main::Ipc::M2C_sendInvisibilityCommand(session->getId(), Main::Ipc::ALL_INVISIBLE);
					}
				}
				else
				{
					session->sendMessage("error: must be in a room");
				}
			}
		};
		REGISTER_CMD(InvisibleAllOn, Common::Enums::PlayerGrade::GRADE_TESTER)

		struct InvisibleAllOff final : public ICommand
		{
			explicit InvisibleAllOff(const Common::Enums::PlayerGrade requiredGrade)
				: ICommand{ requiredGrade, "/invisiblealloff" }
			{
			}

			void execute(const std::string&, std::shared_ptr<Main::Network::Session> session, MN::SessionsManager& sessionsManager, MC::RoomsManager& roomsManager, 
				MP::MainScheduler&, std::uint32_t roomNumber,
				Main::MainServer& mainSv) override
			{
				if (Main::Classes::Room* room = roomsManager.getRoomByNumber(roomNumber))
				{
					if (!room->hasMatchStarted())
					{
						session->sendMessage("error: must use this command inside a match");
						return;
					}
					else
					{
						Main::Ipc::M2C_sendInvisibilityCommand(session->getId(), Main::Ipc::NONE_INVISIBLE);
					}
				}
				else
				{
					session->sendMessage("error: must be in a room");
				}
			}
		};
		REGISTER_CMD(InvisibleAllOff, Common::Enums::PlayerGrade::GRADE_TESTER)

	}
}

#endif


