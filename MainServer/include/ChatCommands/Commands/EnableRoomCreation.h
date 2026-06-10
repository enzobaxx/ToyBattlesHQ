#ifndef ENABLE_ROOM_CREATION_CMD_H
#define ENABLE_ROOM_CREATION_CMD_H


#include "ChatCommands/ICommand.h"
#include "ChatCommands/ChatCommands.h"
#include "MainServer.h"

namespace Main
{
	namespace Command
	{
		struct EnableRoomCreation final : public ICommand
		{
			explicit EnableRoomCreation(const Common::Enums::PlayerGrade requiredGrade)
				: ICommand{ requiredGrade, "/enableroomcreation" }
			{
			}

			void execute(const std::string&, std::shared_ptr<Main::Network::Session> session, MN::SessionsManager& sessionsManager, MC::RoomsManager&, MP::MainScheduler&,
				std::uint32_t,
				Main::MainServer& mainSv) override
			{
				mainSv.setRoomCreationTo(true);
				session->sendMessage("success");
			}
		};
		REGISTER_CMD(EnableRoomCreation, Common::Enums::PlayerGrade::GRADE_TESTER)


		struct DisableRoomCreation final : public ICommand
		{
			explicit DisableRoomCreation(const Common::Enums::PlayerGrade requiredGrade)
				: ICommand{ requiredGrade, "/disableroomcreation" }
			{
			}

			void execute(const std::string&, std::shared_ptr<Main::Network::Session> session, MN::SessionsManager& sessionsManager, MC::RoomsManager&, MP::MainScheduler&, 
				std::uint32_t,
				Main::MainServer& mainSv) override
			{
				mainSv.setRoomCreationTo(false);
				session->sendMessage("success");
			}
		};
		REGISTER_CMD(DisableRoomCreation, Common::Enums::PlayerGrade::GRADE_TESTER)
	}
}

#endif