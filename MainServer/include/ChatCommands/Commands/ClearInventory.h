#ifndef COMMAND_CLEAR_INVENTORY_HEADER
#define COMMAND_CLEAR_INVENTORY_HEADER

#include "../ChatCommands.h"
#include "../ICommand.h"
#include "Utils/Utils.h"
#include "../../MainServer.h"
#include <source_location>

namespace Main
{
	namespace Command
	{
		class ClearInventory final : public ICommand
		{
		private:
			std::string m_targetPlayerName{};

			bool parseCommand(const std::string& providedCommand) override
			{
				std::smatch match;
				if (std::regex_match(providedCommand, match, m_pattern))
				{
					m_targetPlayerName = match[1].str();
					return !m_targetPlayerName.empty();
				}
				return false;
			}

		public:
			explicit ClearInventory(const Common::Enums::PlayerGrade requiredGrade)
				: ICommand{ requiredGrade, "/clearinventory <nickname>", R"(^\S+\s+(\S+))" }
			{
			}

			void execute(const std::string& command, std::shared_ptr<Main::Network::Session> session, MN::SessionsManager& sessionsManager, MC::RoomsManager&,
				MP::MainScheduler& scheduler, std::uint32_t,
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
					if (!scheduler.immediatePersist(std::source_location::current(),
						&Main::Persistence::PersistentDatabase::removeAllPlayerItems, m_targetPlayerName,
						static_cast<std::uint32_t>(session->getAccountInfo().playerGrade)))
					{
						session->sendMessage("error: player not found, has a greater grade, or a db error occurred");
						return;
					}
					session->sendMessage("success: cleared inventory for offline player " + m_targetPlayerName);
					return;
				}

				const bool isSelf = targetSession->getAccountInfo().accountID == session->getAccountInfo().accountID;
				if (targetSession->getAccountInfo().playerGrade > session->getAccountInfo().playerGrade)
				{
					session->sendMessage("error: the target user has a greater grade than you");
					return;
				}

				const auto items = targetSession->getPlayer().getInventory().getItemsAsVec();
				std::uint32_t cleared = 0;
				for (const auto& item : items)
				{
					if (targetSession->deleteItem(item.serialInfo, "Removed via /clearinventory"))
					{
						++cleared;
					}
				}

				session->sendMessage("success: cleared " + std::to_string(cleared) + " items from " + m_targetPlayerName);
				if (!isSelf)
				{
					targetSession->sendMessage("A staff member cleared your inventory.");
				}
			}
		};

		REGISTER_CMD(ClearInventory, Common::Enums::PlayerGrade::GRADE_TESTER)
	}
}

#endif
