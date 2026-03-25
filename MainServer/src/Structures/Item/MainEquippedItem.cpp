#ifndef MAIN_EQUIPPED_ITEM_CPP
#define MAIN_EQUIPPED_ITEM_CPP

#include <cstdint>
#include <ctime> 
#include "../../../include/Structures/Item/MainItem.h"
#include "../../../include/Detail/CdbUtils.h"
#include "../../../include/Structures/Item/MainEquippedItem.h"
#include "Enums/GameEnums.h"
#include <ConstantDatabase/Structures/SetItemInfo.h>
#include "Macros.h"

namespace Main
{
	namespace Structures
	{
		EquippedItem::EquippedItem(const Main::Structures::Item& item, std::uint64_t aid)
			: id{ item.itemId.itemId }
			, expirationDate{ static_cast<time32_t>(item.expirationDate) }
			, serialInfo{ item.serialInfo }
			, durability{ item.durability }
			, energy{ item.energy }
			, isSealed{ item.isSealed }
			, sealLevel{ item.sealLevel }
			, experienceEnhancement{ item.experienceEnhancement }
			, mpEnhancement{ item.mpEnhancement }
		{
			type = CdbUtils::getItemType(item.itemId.itemId).value_or(static_cast<std::uint32_t>(-1));
			auto& setItemsInstance = Common::ConstantDatabase::CdbSingleton<Common::ConstantDatabase::SetItemInfo>::getInstance();

			if (type == 22 || type == 23 /* diorama and scaffold */)
			{
				type -= 3;
			}

			if (type != static_cast<std::uint32_t>(-1) && type >= 17)
			{
				if (type != 22 && type != 23 && !setItemsInstance.getEntry(item.itemId.itemId))
				{
					type = static_cast<std::uint32_t>(-1);
				}
				else
				{
					type = Common::Enums::ItemType::SET;
				}
			}
		}
	}
}

#endif

