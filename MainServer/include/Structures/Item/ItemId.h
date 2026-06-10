#include <cstdint>
#include "Detail/CdbUtils.h"
#ifndef ITEM_ID_H
#define ITEM_ID_H

namespace Main
{
	namespace Structures
	{
		struct ItemId
		{
			std::uint32_t itemId : 23 = 0;
			std::uint32_t stock : 9 = 0;

			explicit ItemId(std::uint32_t wholeId)
			{
				itemId = wholeId & 0x7FFFFF;
				stock = Main::CdbUtils::getItemStocks(itemId).value_or(1);
			}
		};
	}
}

#endif