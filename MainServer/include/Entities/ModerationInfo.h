#ifndef MODERATION_INFO_CLASS_H
#define MODERATION_INFO_CLASS_H

#include "Structures/AccountInfo/MuteInfo.h"

#include <string>

namespace Main
{
	namespace Classes
	{
		class ModerationInfo
		{
		private:
			bool m_isMuted{ false };
			bool m_isRoomCreationEnabled{ true };
			bool m_isVotekickEnabled{ true };
			std::string m_mutedBy{};
			std::string m_muteReason{};
			std::string m_mutedUntil{};

		public:
			// Mute
			void mute(const std::string& reason, const std::string& mutedBy, const std::string& mutedUntil);
			void unmute();
			bool isMuted() const;
			Main::Structures::MuteInfo getMuteInfo() const;

			// Room creation
			void disableRoomCreation();
			void enableRoomCreation();
			bool isRoomCreationEnabled() const noexcept;

			// Votekick
			void disableVotekick();
			void enableVotekick();
			bool isVotekickEnabled() const noexcept;
		};
	}
}

#endif
