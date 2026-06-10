#ifndef COMMAND_GET_ITEM_FAMILY_HEADER
#define COMMAND_GET_ITEM_FAMILY_HEADER

#include "../ChatCommands.h"
#include "../ICommand.h"
#include "Utils/Utils.h"
#include "../../MainServer.h"
#include "../../Detail/CdbUtils.h"
#include <array>
#include <vector>
#include <string>

namespace Main
{
	namespace Command
	{
		class GetItemFamily final : public ICommand
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
			explicit GetItemFamily(const Common::Enums::PlayerGrade requiredGrade)
				: ICommand{ requiredGrade, "/getitemfamily <familyRootID>",  R"(^\S+\s(\d+))" }
			{
			}

			void execute(const std::string& command, std::shared_ptr<Main::Network::Session> session, MN::SessionsManager&, MC::RoomsManager&, MP::MainScheduler&,
				std::uint32_t,
				Main::MainServer&) override
			{
				if (!parseCommand(command))
				{
					session->sendMessage("error: parsing failed. Usage: /getitemfamily <familyRootID> (e.g. 3913300)");
					return;
				}

				if (m_itemId % 100 != 0)
				{
					session->sendMessage("error: " + std::to_string(m_itemId) + " is not a family root, the ID must end in 00 (e.g. 3913300)");
					return;
				}

				if (!Main::CdbUtils::itemExists(m_itemId))
				{
					session->sendMessage("error: root item " + std::to_string(m_itemId) + " does not exist, check the family root id");
					return;
				}

				static constexpr std::array<std::uint32_t, 33> familyOffsets = {
					0, 10, 20, 30, 40, 50,
					51, 52, 53, 54, 55, 56, 57, 58, 59,
					61, 62, 63, 64, 65, 66, 67, 68, 69,
					71, 72, 73, 74, 75, 76, 77, 78, 79
				};

				std::vector<std::uint32_t> validIds;
				std::string missingIds;
				for (const std::uint32_t offset : familyOffsets)
				{
					const std::uint32_t id = m_itemId + offset;
					if (Main::CdbUtils::itemExists(id))
					{
						validIds.push_back(id);
					}
					else
					{
						if (!missingIds.empty()) missingIds += ", ";
						missingIds += std::to_string(id);
					}
				}

				if (!session->getPlayer().getInventory().hasEnoughInventorySpace(static_cast<std::uint16_t>(validIds.size())))
				{
					session->sendMessage("error: not enough inventory space, this family needs " + std::to_string(validIds.size())
						+ " free slots. Nothing was spawned, free up space and try again");
					return;
				}

				std::uint32_t spawnedCount = 0;
				std::string failedIds;
				for (const std::uint32_t id : validIds)
				{
					if (session->spawnItemCommand(id, "This item was spawned through /getitemfamily command"))
					{
						++spawnedCount;
					}
					else
					{
						if (!failedIds.empty()) failedIds += ", ";
						failedIds += std::to_string(id);
					}
				}

				session->sendMessage("success: spawned " + std::to_string(spawnedCount) + "/" + std::to_string(validIds.size())
					+ " items for family " + std::to_string(m_itemId));

				if (!missingIds.empty())
				{
					session->sendMessage("note: these variants do not exist for this family and were skipped: " + missingIds);
				}
				if (!failedIds.empty())
				{
					session->sendMessage("warning: these items exist but failed to spawn: " + failedIds);
				}
			}
		};

		REGISTER_CMD(GetItemFamily, Common::Enums::PlayerGrade::GRADE_TESTER)
	}
}

#endif
