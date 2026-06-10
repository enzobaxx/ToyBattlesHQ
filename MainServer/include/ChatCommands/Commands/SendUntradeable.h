#ifndef COMMAND_SEND_UNTRADEABLE_HEADER
#define COMMAND_SEND_UNTRADEABLE_HEADER

#include "../ChatCommands.h"
#include "../ICommand.h"
#include "Utils/Utils.h"
#include "../../MainServer.h"
#include <charconv>
#include <source_location>

namespace Main
{
	namespace Command
	{
		class SendUntradeable final : public ICommand
		{
		private:
			std::string m_targetNickname{};
			std::uint32_t m_itemId{};

			bool parseCommand(const std::string& providedCommand) override
			{
				std::smatch match;
				if (std::regex_match(providedCommand, match, m_pattern))
				{
					m_targetNickname = match[1].str();
					if (m_targetNickname.empty())
					{
						return false;
					}
					const std::string& itemStr = match[2].str();
					auto [ptr, ec] = std::from_chars(itemStr.data(), itemStr.data() + itemStr.size(), m_itemId);
					return ec == std::errc{} && ptr == itemStr.data() + itemStr.size();
				}
				return false;
			}

		public:
			explicit SendUntradeable(const Common::Enums::PlayerGrade requiredGrade)
				: ICommand{ requiredGrade, "/senduntradeable <nickname> <itemID>", R"(^\S+\s+(\S+)\s+(\d+))" }
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

				if (!Main::CdbUtils::itemExists(m_itemId))
				{
					session->sendMessage("error: ItemID not found");
					return;
				}

				auto targetSession = sessionsManager.findSessionByName(m_targetNickname.c_str());
				if (!targetSession)
				{
					const Main::Structures::SpawnedItem spawnedItem{ m_itemId };
					const Main::Structures::Item item{ spawnedItem };
					if (!scheduler.immediatePersist(std::source_location::current(),
						&Main::Persistence::PersistentDatabase::addUntradeableItemByName, m_targetNickname, item,
						static_cast<std::uint32_t>(session->getAccountInfo().playerGrade)))
					{
						session->sendMessage("error: player not found, has a greater grade, or a db error occurred");
						return;
					}
					session->sendMessage("success: sent untradeable item to offline player " + m_targetNickname + " (target should relog)");
					return;
				}

				if (targetSession->getAccountInfo().playerGrade > session->getAccountInfo().playerGrade)
				{
					session->sendMessage("error: the target user has a greater grade than you");
					return;
				}

				if (!targetSession->spawnUntradeableItemCommand(m_itemId, "This item was spawned through /senduntradeable command"))
				{
					session->sendMessage("error: could not give the item (target inventory may be full)");
					return;
				}
				session->sendMessage("success: sent untradeable item to " + m_targetNickname);
			}
		};

		REGISTER_CMD(SendUntradeable, Common::Enums::PlayerGrade::GRADE_TESTER)
	}
}

#endif
