#ifndef SETMAX_CURRENCY_HEADER
#define SETMAX_CURRENCY_HEADER

#include "ChatCommands/ICommand.h"
#include "ChatCommands/ChatCommands.h"
#include "MainServer.h"

namespace Main
{
	namespace Command
	{
		struct SetCurrency final : public ICommand
		{
			explicit SetCurrency(const Common::Enums::PlayerGrade requiredGrade)
				: ICommand{ requiredGrade, "/setcurrency" }
			{
			}

			void execute(const std::string&, std::shared_ptr<Main::Network::Session> session, MN::SessionsManager&, MC::RoomsManager&, MP::MainScheduler&, std::uint32_t,
				Main::MainServer&) override 
			{
				session->setAccountMicroPoints(536870912);
				session->setAccountRockTotens(536870912);
				session->setAccountCoins(100);
				session->sendCurrency();
			}
		};

		REGISTER_CMD(SetCurrency, Common::Enums::PlayerGrade::GRADE_TESTER)
	}
}

#endif