#include "Entities/ModerationInfo.h"

namespace Main
{
	namespace Classes
	{
		void ModerationInfo::mute(const std::string& reason, const std::string& mutedBy, const std::string& mutedUntil)
		{
			m_isMuted = true;
			m_muteReason = reason;
			m_mutedBy = mutedBy;
			m_mutedUntil = mutedUntil;
		}

		void ModerationInfo::unmute()
		{
			m_isMuted = false;
		}

		bool ModerationInfo::isMuted() const
		{
			return m_isMuted;
		}

		Main::Structures::MuteInfo ModerationInfo::getMuteInfo() const
		{
			return Main::Structures::MuteInfo{ m_isMuted, m_muteReason, m_mutedBy, m_mutedUntil };
		}

		void ModerationInfo::disableRoomCreation()
		{
			m_isRoomCreationEnabled = false;
		}

		void ModerationInfo::enableRoomCreation()
		{
			m_isRoomCreationEnabled = true;
		}

		bool ModerationInfo::isRoomCreationEnabled() const noexcept
		{
			return m_isRoomCreationEnabled;
		}

		void ModerationInfo::disableVotekick()
		{
			m_isVotekickEnabled = false;
		}

		void ModerationInfo::enableVotekick()
		{
			m_isVotekickEnabled = true;
		}

		bool ModerationInfo::isVotekickEnabled() const noexcept
		{
			return m_isVotekickEnabled;
		}
	}
}
