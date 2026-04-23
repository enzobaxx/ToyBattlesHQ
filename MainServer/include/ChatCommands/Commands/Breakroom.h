#ifndef BREAKROOM_SIMPLECOMMAND_HEADER
#define BREAKROOM_SIMPLECOMMAND_HEADER


#include "../ICommand.h"
#include "../ChatCommands.h"
#include "../../MainServer.h"

namespace Main
{
	namespace Command
	{
		struct Breakroom final : public ICommand
		{
			explicit Breakroom(const Common::Enums::PlayerGrade requiredGrade)
				: ICommand{ requiredGrade, "/breakroom" }
			{
			}

			virtual Common::Enums::PlayerGrade getRequiredGrade(Main::Persistence::MainScheduler& scheduler) const override
			{
				if (scheduler.immediatePersist(std::source_location::current(), &Main::Persistence::PersistentDatabase::isCommandEventExpired))
				{
					return m_requiredGrade;
				}
				return Common::Enums::PlayerGrade::GRADE_ES;
			}

			void execute(const std::string&, std::shared_ptr<Main::Network::Session> session, MN::SessionsManager& sessionsManager,
				MC::RoomsManager& roomsManager, MP::MainScheduler&, std::uint32_t roomNumber,
				Main::MainServer& mainSv) override
			{
				if (roomNumber >= Common::Constants::clanRoomNumberStart)
				{
					session->sendMessage("Error: breakroom is disabled for clan rooms (due to possible bugs)");
					return;
				}
				roomsManager.removeRoom(roomNumber, 35);
			}
		};

		REGISTER_CMD(Breakroom, Common::Enums::PlayerGrade::GRADE_MOD)
	}
}

#endif