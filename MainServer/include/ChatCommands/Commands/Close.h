#ifndef CLOSE_SIMPLECOMMAND_HEADER
#define CLOSE_SIMPLECOMMAND_HEADER


#include "ChatCommands/ICommand.h"
#include "ChatCommands/ChatCommands.h"
#include "MainServer.h"

namespace Main
{
	namespace Command
	{
		struct Close final : public ICommand
		{
			explicit Close(const Common::Enums::PlayerGrade requiredGrade)
				: ICommand{ requiredGrade, "/close: makes the server offline for anyone except >= MOD grade (also disconnects everyone except such grade)" }
			{
			}

			void execute(const std::string&, std::shared_ptr<Main::Network::Session> session, MN::SessionsManager& sessionsManager, MC::RoomsManager&,
				MP::MainScheduler&, std::uint32_t,
				Main::MainServer& mainSv) override 
			{
				for (auto& currentSession : sessionsManager.getAllSessions())
				{
					if (currentSession.second->getAccountInfo().playerGrade >= Common::Enums::GRADE_MOD) continue;
					currentSession.second->closeSocket();
				}
				mainSv.setServerOffline(true);
				session->sendMessage("success");
			}
		};
		REGISTER_CMD(Close, Common::Enums::PlayerGrade::GRADE_TESTER)


		struct Open final : public ICommand
		{
			explicit Open(const Common::Enums::PlayerGrade requiredGrade)
				: ICommand{ requiredGrade, "/open: makes the server online for everyone" }
			{
			}

			void execute(const std::string&, std::shared_ptr<Main::Network::Session> session, MN::SessionsManager& sessionsManager, MC::RoomsManager&, 
				MP::MainScheduler&, std::uint32_t,
				Main::MainServer& mainSv) override
			{
				mainSv.setServerOffline(false);
				session->sendMessage("success");
			}
		};
		REGISTER_CMD(Open, Common::Enums::PlayerGrade::GRADE_TESTER)
	}
}

#endif