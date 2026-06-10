#ifndef SHUTDOWN_SIMPLECOMMAND_HEADER
#define SHUTDOWN_SIMPLECOMMAND_HEADER


#include "ChatCommands/ICommand.h"
#include "ChatCommands/ChatCommands.h"
#include "MainServer.h"

namespace Main
{
	namespace Command
	{
		struct Shutdown final : public ICommand
		{
			explicit Shutdown(const Common::Enums::PlayerGrade requiredGrade)
				: ICommand{ requiredGrade, "/shutdown" }
			{
			}

			void execute(const std::string&, std::shared_ptr<Main::Network::Session>, MN::SessionsManager& sessionsManager, MC::RoomsManager&, MP::MainScheduler&,
				std::uint32_t,
				Main::MainServer&) override 
			{
				for (auto& currentSession : sessionsManager.getAllSessions())
				{
					currentSession.second->persistNow();
					currentSession.second->closeSocket();
				}
				std::terminate();
			}
		};

		REGISTER_CMD(Shutdown, Common::Enums::PlayerGrade::GRADE_TESTER)
	}
}

#endif