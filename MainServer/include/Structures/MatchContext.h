#ifndef MATCH_CONTEXT_H
#define MATCH_CONTEXT_H

#include <cstdint>

namespace Main
{
	namespace Structures
	{
		struct MatchContext
		{
			std::uint16_t roomNumber{};
			std::uint16_t partyRoomNumber{};
			bool isInMatch{};
			std::uint32_t batteryObtainedInMatch{};
		};
	}
}

#endif
