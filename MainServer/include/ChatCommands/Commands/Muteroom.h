#ifndef MUTEROOM_SIMPLECOMMAND_HEADER
#define MUTEROOM_SIMPLECOMMAND_HEADER


#include "ChatCommands/ICommand.h"
#include "ChatCommands/ChatCommands.h"
#include "MainServer.h"

namespace Main
{
	namespace Command
	{
		struct Muteroom final : public ICommand
		{
			explicit Muteroom(const Common::Enums::PlayerGrade requiredGrade)
				: ICommand{ requiredGrade, "/muteroom" }
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

			void execute(const std::string&, std::shared_ptr<Main::Network::Session> session, MN::SessionsManager& sessionsManager, MC::RoomsManager& roomsManager, 
				MP::MainScheduler&, std::uint32_t roomNumber,
				Main::MainServer& mainSv) override
			{
				if (Main::Classes::Room* room = roomsManager.getRoomByNumber(roomNumber))
				{
					room->muteRoom();
					session->sendMessage("success");
				}
				else
				{
					session->sendMessage("Error: Not in a room");
				}
			}
		};

		REGISTER_CMD(Muteroom, Common::Enums::PlayerGrade::GRADE_MOD)


		struct Unmuteroom final : public ICommand
		{
			explicit Unmuteroom(const Common::Enums::PlayerGrade requiredGrade)
				: ICommand{ requiredGrade, "/unmuteroom" }
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

			void execute(const std::string&, std::shared_ptr<Main::Network::Session> session, MN::SessionsManager& sessionsManager, MC::RoomsManager& roomsManager, 
				MP::MainScheduler&, std::uint32_t roomNumber,
				Main::MainServer& mainSv) override
			{
				if (Main::Classes::Room* room = roomsManager.getRoomByNumber(roomNumber))
				{
					room->unmuteRoom();
					session->sendMessage("success");
				}
				else
				{
					session->sendMessage("Error: Not in a room");
				}
			}
		};

		REGISTER_CMD(Unmuteroom, Common::Enums::PlayerGrade::GRADE_MOD)
	}
}

#endif