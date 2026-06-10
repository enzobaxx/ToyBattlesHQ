#ifndef RANDOM_COMMANDS_H
#define RANDOM_COMMANDS_H


#include "ChatCommands/ICommand.h"
#include "ChatCommands/ChatCommands.h"
#include "MainServer.h"

namespace Main
{
	namespace Command
	{
		struct RespawnNpcs final : public ICommand
		{
			explicit RespawnNpcs(const Common::Enums::PlayerGrade requiredGrade)
				: ICommand{ requiredGrade, "/respawnnpcs" }
			{
			}

			void execute(const std::string&, std::shared_ptr<Main::Network::Session> session, MN::SessionsManager& sessionsManager, MC::RoomsManager& roomsManager,
				MP::MainScheduler&, std::uint32_t roomNumber,
				Main::MainServer& mainSv) override
			{
				//NO_CHECK
				struct S
				{
					std::uint32_t u0 = 0;
					std::uint32_t u1 = 0xe1c45f65;
					std::uint32_t u2 = 0xb96c68fa;
					std::uint32_t u3 = 0xb7dd385f;
					std::uint32_t u4 = 1;
					std::uint32_t u5 = 0x00070000;
				};
				if (Main::Classes::Room* room = roomsManager.getRoomByNumber(roomNumber))
				{
					S s;
					Common::Network::Packet packet;
					packet.setSession(session->getId());
					packet.setCommand(326, 0, 0, 0);
					packet.setData(reinterpret_cast<std::uint8_t*>(&s), sizeof(s));

					for (int i = 0; i < 10; ++i)
					{
						room->broadcastToRoom(packet);
					}
					session->sendMessage("success");
				}
				else
				{
					session->sendMessage("error: you must be in a room");
				}
			}
		};

		REGISTER_CMD(RespawnNpcs, Common::Enums::PlayerGrade::GRADE_TESTER)
	}
}

#endif