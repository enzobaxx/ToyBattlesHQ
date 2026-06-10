#ifndef ONLINE_SIMPLECOMMAND_HEADER
#define ONLINE_SIMPLECOMMAND_HEADER


#include "ChatCommands/ICommand.h"
#include "Utils/Utils.h"
#include "ChatCommands/ChatCommands.h"
#include "MainServer.h"

namespace Main
{
	namespace Command
	{
		struct Online final : public ICommand
		{
			explicit Online(const Common::Enums::PlayerGrade requiredGrade)
				: ICommand{ requiredGrade, "/online: shows online players" }
			{
			}

			void execute(const std::string&, std::shared_ptr<Main::Network::Session> session, MN::SessionsManager& sessionsManager, MC::RoomsManager&, 
				MP::MainScheduler&, std::uint32_t,
				Main::MainServer&) override 
			{
				const auto& allSessions = sessionsManager.getAllSessions();
				session->sendMessage("Online players: " + std::to_string(allSessions.size()));
				for (const auto& currentSession : allSessions)
				{
					const auto& accountInfo = currentSession.second->getAccountInfo();
					session->sendMessage(accountInfo.nickname);
				}
			}
		};

		REGISTER_CMD(Online, Common::Enums::PlayerGrade::GRADE_MOD)
	}
}
#endif