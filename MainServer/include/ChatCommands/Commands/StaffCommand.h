#ifndef COMMAND_STAFF_H
#define COMMAND_STAFF_H

#include "ChatCommands/ChatCommands.h"
#include "ChatCommands/ICommand.h"
#include "Utils/Utils.h"
#include "MainServer.h"

namespace Main
{
	namespace Command
	{
		class Staff final : public ICommand
		{
		public:
			explicit Staff(const Common::Enums::PlayerGrade requiredGrade)
				: ICommand{ requiredGrade, "/staff - Toggles Staff prefix in nickname", "^\\S+$" }
			{
			}

			void execute(const std::string& command, std::shared_ptr<Main::Network::Session> session, MN::SessionsManager& sessionsManager, MC::RoomsManager& roomsManager, MP::MainScheduler& scheduler,
				std::uint32_t roomNumber,
				Main::MainServer& mainServer) override
			{
				if (session->getPlayer().getRoomNumber() != 0)
				{
					session->sendMessage("You cannot use this command while in a room.");
					return;
				}

				const auto& accountInfo = session->getAccountInfo();
				std::string currentNickname = accountInfo.nickname;

				std::string prefix;
				if (accountInfo.playerGrade == Common::Enums::PlayerGrade::GRADE_MOD)
				{
					prefix = "[MOD]";
				}
				else if (accountInfo.playerGrade == Common::Enums::PlayerGrade::GRADE_ES)
				{
					prefix = "[ES]";
				}
				else if (accountInfo.playerGrade == Common::Enums::PlayerGrade::GRADE_GM || accountInfo.playerGrade == Common::Enums::PlayerGrade::GRADE_TESTER)
				{
					prefix = "[GM]";
				}

				std::string newNickname;

				if (currentNickname.find(prefix) == 0)
				{
					newNickname = currentNickname.substr(prefix.length());
					size_t start = newNickname.find_first_not_of(" \t");
					if (start != std::string::npos)
					{
						newNickname = newNickname.substr(start);
					}
					else
					{
						newNickname = "";
					}
				}
				else
				{
					newNickname = prefix + currentNickname;
				}

				if (newNickname.length() >= 16)
				{
					session->sendMessage("New nickname would be too long.");
					return;
				}

				struct Staff {
					std::uint32_t unknown0;
					std::uint32_t unknown1;
					char newNick[16]{};
				};
				Staff nicknameData{ 0, 0 };
				std::memcpy(nicknameData.newNick, newNickname.c_str(), newNickname.length());
				Common::Network::Packet nicknamePacket;
				nicknamePacket.setTcpHeader(session->getId(), Common::Enums::NO_ENCRYPTION);
				nicknamePacket.setCommand(102, 1, 53, 0);
				nicknamePacket.setData(reinterpret_cast<const std::uint8_t*>(&nicknameData), sizeof(nicknameData));
				session->asyncWrite(nicknamePacket);
				session->setPlayerName(newNickname.c_str());

				session->sendMessage("Staff prefix toggled.");
			}
		};

		REGISTER_CMD(Staff, Common::Enums::PlayerGrade::GRADE_ES);
	}
}

#endif