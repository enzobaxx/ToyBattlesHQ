#ifndef COMMAND_GET_ITEM_HEADER
#define COMMAND_GET_ITEM_HEADER

#include "ChatCommands/ChatCommands.h"
#include "ChatCommands/ICommand.h"
#include "Utils/Utils.h"
#include "MainServer.h"

namespace Main
{
	namespace Command
	{
		class GetItem final : public ICommand
		{
		private:
			std::uint32_t m_itemId{};

			bool parseCommand(const std::string& providedCommand) override
			{
				std::smatch match;
				if (std::regex_match(providedCommand, match, m_pattern))
				{
					const std::string& matched_str = match[1].str();
					const std::from_chars_result result = std::from_chars(matched_str.data(), matched_str.data() + matched_str.size(), m_itemId);
					if (result.ec == std::errc())
					{
						return true;
					}
				}
				return false;
			}

		public:
			explicit GetItem(const Common::Enums::PlayerGrade requiredGrade)
				: ICommand{ requiredGrade, "/getitem <itemID>",  R"(^\S+\s(\d+))" }
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
				if (session->spawnItemCommand(m_itemId, "This item was spawned through /getitem command"))
				{
					session->sendMessage("success");
				}
			}
		};

		REGISTER_CMD(GetItem, Common::Enums::PlayerGrade::GRADE_TESTER)
	}
}


#endif