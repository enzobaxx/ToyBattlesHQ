#ifndef DISCONNECT_COMMAND_HEADER
#define DISCONNECT_COMMAND_HEADER

#include "../ICommand.h"
#include "../ChatCommands.h"
#include "Utils/Utils.h"
#include "../../MainServer.h"

namespace Main
{
	namespace Command
	{
		class AcSs final : public ICommand
		{
		private:
			std::string m_targetPlayerName{};

			bool parseCommand(const std::string& providedCommand) override
			{
				std::smatch match;
				if (std::regex_match(providedCommand, match, this->m_pattern))
				{
					m_targetPlayerName = match[1].str();
					return true;
				}
				return false;
			}

		public:
			explicit AcSs(const Common::Enums::PlayerGrade requiredGrade)
				: ICommand{ requiredGrade, "/acss <nickname>",  R"(^\S+\s+(.*)$)" }
			{
			}

			void execute(const std::string& command, std::shared_ptr<Main::Network::Session> session, MN::SessionsManager& sessionsManager, MC::RoomsManager&,
				MP::MainScheduler&, std::uint32_t,
				Main::MainServer&) override
			{
				if (!parseCommand(command))
				{
					session->sendMessage("parsing error");
					return;	
				}

				auto targetSession = sessionsManager.findSessionByName(m_targetPlayerName.c_str());
				if (!targetSession)
				{
					session->sendMessage("error: player not found");
					return;
				}
				if (targetSession->getAccountInfo().playerGrade > session->getAccountInfo().playerGrade)
				{
					session->sendMessage("error: The target player has a greater grade than you.");
					return;
				}
				Common::Network::Packet acss;
				acss.setTcpHeader(targetSession->getId(), Common::Enums::USER_LARGE_ENCRYPTION);
				acss.setCommand(81, 0, 0, 0);
				targetSession->asyncWrite(acss);

				session->sendMessage("success");
			}
		};

		REGISTER_CMD(AcSs, Common::Enums::PlayerGrade::GRADE_DEV)
	};
}


#endif
