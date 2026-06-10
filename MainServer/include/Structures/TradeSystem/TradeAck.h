#ifndef TRADE_ACK_STRUCTURE_H
#define TRADE_ACK_STRUCTURE_H

#include <cstdint>
#include "Structures/AccountInfo/MainAccountUniqueId.h"
#include "Macros.h"

namespace Main	
{
	namespace Structures
	{
PACK_PUSH(1)
		struct TradeAck
		{
			UniqueId uniqueId{};
			std::uint32_t accountId{};
		};
PACK_POP()
	}
}

#endif