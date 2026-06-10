#ifndef TESTCOMMAND_HEADER
#define TESTCOMMAND_HEADER


#include "ChatCommands/ICommand.h"
#include "ChatCommands/ChatCommands.h"
#include "MainServer.h"

namespace Main
{
	namespace Command
	{
		struct TestCmd final : public ICommand
		{
			explicit TestCmd(const Common::Enums::PlayerGrade requiredGrade)
				: ICommand{ requiredGrade, "/testcmd" }
			{
			}

			void execute(const std::string&, std::shared_ptr<Main::Network::Session> session, MN::SessionsManager& sessionsManager, MC::RoomsManager& roomsManager,
				MP::MainScheduler&, std::uint32_t roomNumber,
				Main::MainServer& mainSv) override
			{
				Common::Network::Packet packet;
				packet.setTcpHeader(session->getId(), Common::Enums::USER_LARGE_ENCRYPTION);
				packet.setCommand(81, 0, 3, 0);
				session->asyncWrite(packet);
			}
		};

		REGISTER_CMD(TestCmd, Common::Enums::PlayerGrade::GRADE_TESTER)
	}
}

#endif