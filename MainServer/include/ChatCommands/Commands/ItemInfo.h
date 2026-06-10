#ifndef COMMAND_ITEM_INFO_HEADER
#define COMMAND_ITEM_INFO_HEADER

#include "ChatCommands/ChatCommands.h"
#include "ChatCommands/ICommand.h"
#include "Utils/Utils.h"
#include "MainServer.h"

namespace Main
{
	namespace Command
	{
		class ItemInfo final : public ICommand
		{
		private:
			std::uint32_t m_itemId{};

			bool parseCommand(const std::string& providedCommand) override
			{
				std::smatch match;
				if (std::regex_match(providedCommand, match, m_pattern))
				{
					const std::string& idStr = match[1].str();
					auto [ptr, ec] = std::from_chars(idStr.data(), idStr.data() + idStr.size(), m_itemId);
					return ec == std::errc{} && ptr == idStr.data() + idStr.size();
				}
				return false;
			}

			static std::string currencyLabel(Main::Enums::ItemCurrencyType type)
			{
				switch (type)
				{
				case Main::Enums::ITEM_RT:     return "RockTokens";
				case Main::Enums::ITEM_MP:     return "MicroPoints";
				case Main::Enums::ITEM_COIN:   return "Coins";
				case Main::Enums::ITEM_COUPON: return "Coupons";
				default:                       return "Unknown";
				}
			}

		public:
			explicit ItemInfo(const Common::Enums::PlayerGrade requiredGrade)
				: ICommand{ requiredGrade, "/iteminfo <itemId>", R"(^\S+\s+(\d+))" }
			{
			}

			void execute(const std::string& command, std::shared_ptr<Main::Network::Session> session, MN::SessionsManager&, MC::RoomsManager&,
				MP::MainScheduler&, std::uint32_t,
				Main::MainServer&) override
			{
				if (!parseCommand(command))
				{
					session->sendMessage("parsing error");
					return;
				}

				if (!Main::CdbUtils::itemExists(m_itemId))
				{
					session->sendMessage("Item ID " + std::to_string(m_itemId) + " not found.");
					return;
				}

				const std::string name = Main::CdbUtils::getItemName(m_itemId).value_or("(unknown)");
				session->sendMessage("Item: " + name + " (ID: " + std::to_string(m_itemId) + ")");

				if (const auto type = Main::CdbUtils::getItemType(m_itemId))
				{
					session->sendMessage("Type: " + std::to_string(*type)
						+ " | Tradeable: " + (Main::CdbUtils::isTradeable(m_itemId).value_or(false) ? "yes" : "no"));
				}

				session->sendMessage("Duration(raw ii_limited_time): " + std::to_string(Main::CdbUtils::getItemDuration(m_itemId)));

				if (const auto price = Main::CdbUtils::getItemPrice(m_itemId))
				{
					session->sendMessage("Price: " + std::to_string(price->second) + " " + currencyLabel(price->first));
				}
				else
				{
					session->sendMessage("Price: not sold directly (no shop price)");
				}
			}
		};

		REGISTER_CMD(ItemInfo, Common::Enums::PlayerGrade::GRADE_TESTER)
	}
}

#endif
