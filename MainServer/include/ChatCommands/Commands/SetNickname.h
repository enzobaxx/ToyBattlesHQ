#ifndef COMMAND_SET_NICKNAME_H
#define COMMAND_SET_NICKNAME_H

#include "ChatCommands/ChatCommands.h"
#include "ChatCommands/ICommand.h"
#include "Utils/Utils.h"
#include "MainServer.h"
#include <cstring>
#include "../../../../Common/include/Network/Packet.h"
#include "../../../../Common/include/Enums/MiscellaneousEnums.h"

namespace Main
{
	namespace Command
	{
		class SetName final : public ICommand
		{
		private:
			std::string m_nickname;

			bool parseCommand(const std::string& providedCommand) override
			{
				std::smatch match;
				if (std::regex_match(providedCommand, match, this->m_pattern))
				{
					m_nickname = match[1].str();
					return true;
				}
				return false;
			}

		public:
			explicit SetName(const Common::Enums::PlayerGrade requiredGrade)
				: ICommand{ requiredGrade, "/setname <nickname>", R"(^\S+\s+([^\s]+).*)" }
			{
			}

			void execute(const std::string& command, std::shared_ptr<Main::Network::Session> session, MN::SessionsManager&, MC::RoomsManager&, MP::MainScheduler&,
				std::uint32_t,
				Main::MainServer&) override
			{
				if (!parseCommand(command))
				{
					session->sendMessage("parsing error");
					return;
				}

				if (!session->setPlayerName(m_nickname)) return;
				
				struct SetName {
					std::uint32_t unknown0;
					std::uint32_t unknown1;
					char newNick[16]{};
				};

				SetName nicknameData{ 0, 0 };
				std::memcpy(nicknameData.newNick, m_nickname.c_str(), m_nickname.length());

				Common::Network::Packet nicknamePacket;
				nicknamePacket.setTcpHeader(session->getId(), Common::Enums::NO_ENCRYPTION);
				nicknamePacket.setCommand(102, 1, 53, 0);
				nicknamePacket.setData(reinterpret_cast<const std::uint8_t*>(&nicknameData), sizeof(nicknameData));
				session->asyncWrite(nicknamePacket);
			}
		};

		REGISTER_CMD(SetName, Common::Enums::PlayerGrade::GRADE_NORMAL);
	}
}

#endif
