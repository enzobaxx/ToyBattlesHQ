#ifndef MODERATION_INFO_CLASS_H
#define MODERATION_INFO_CLASS_H

#include <string>

namespace Main
{
	namespace Classes
	{
		struct ModerationInfo
		{
			bool isMuted{ false };
			bool isRoomCreationEnabled{ true };
			bool isVotekickEnabled{ true };
			std::string mutedBy{};
			std::string muteReason{};
			std::string mutedUntil{};
		};
	}
}

#endif
