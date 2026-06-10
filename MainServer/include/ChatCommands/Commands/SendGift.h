#ifndef SENDGIFT_COMPLEXCOMMAND_HEADER
#define SENDGIFT_COMPLEXCOMMAND_HEADER

#include "ChatCommands/ICommand.h"
#include "ChatCommands/ChatCommands.h"
#include "Utils/Utils.h"
#include "Utils/Constants.h"
#include "MainServer.h"
#include <source_location>

namespace Main
{
	namespace Command
	{
		class SendGift final : public ICommand
		{
		private:
			std::string m_playername{};
			std::uint32_t m_itemId{};
			std::string m_giftDescription{};

			bool parseCommand(const std::string& providedCommand) override
			{
				std::smatch match;
				if (std::regex_match(providedCommand, match, this->m_pattern))
				{
					m_playername = match[2].str();
					const std::string& itemid = match[3].str();
					auto [ptr, ec] = std::from_chars(itemid.data(), itemid.data() + itemid.size(), m_itemId);
					if (ec != std::errc{} || ptr != itemid.data() + itemid.size())
					{
						return false;
					}
					m_giftDescription = match[4].str();
					return true;
				}
				return false;
			}

		public:
			SendGift(const Common::Enums::PlayerGrade requiredGrade)
				: ICommand{ requiredGrade, "/sendgift <nickname> <itemid> <description>"
				, R"(^(\S+)\s*(\S+)\s*(\d+)\s*(.*)$)" }
			{
			}

			void execute(const std::string& command, std::shared_ptr<Main::Network::Session> session, MN::SessionsManager& sessionsManager, MC::RoomsManager&, 
				MP::MainScheduler& scheduler, std::uint32_t,
				Main::MainServer&) override 
			{
				START_BENCHMARK
				if (!parseCommand(command))
				{
					session->sendMessage("error: parsing error, see /? for command usage");
					return;
				}

				if (!Main::CdbUtils::itemExists(m_itemId))
				{
					session->sendMessage("error: ItemID not found");
					return;
				}
				if (m_giftDescription.size() >= Common::Constants::maxMailboxMessage)
				{
					session->sendMessage("error: description should be less than 255 characters");
					return;
				}

				auto targetSession = sessionsManager.findSessionByName(m_playername.c_str());
				if (!targetSession)
				{
					if (!scheduler.immediatePersist(std::source_location::current(), LIFT_MEMBER(storeGiftbox), m_playername, m_giftDescription, m_itemId))
					{
						session->sendMessage("error: player not found or their giftbox is full");
						return;
					}
				}
				else
				{
					if (!targetSession->receiveGift(m_itemId, m_giftDescription))
					{
						session->sendMessage("error: target user has not enough space inside giftbox (current max: 100)");
						return;
					}
				}
				session->sendMessage("success");
				END_BENCHMARK(SendGift::execute, session)
			}
		};

		REGISTER_CMD(SendGift, Common::Enums::PlayerGrade::GRADE_TESTER)
	}
}
#endif
