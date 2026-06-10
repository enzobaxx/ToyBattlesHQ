#ifndef CLANS_MANAGER_H
#define CLANS_MANAGER_H

#include <unordered_map>
#include <vector>
#include <optional>
#include <memory>
#include <expected>
#include <string>
#include <utility>
#include <cstdint>
#include "Rooms/PartyRoom.h"

namespace Main
{
	namespace Classes
	{
		class PartiesManager
		{
		private:
			std::unordered_map<std::uint16_t, std::vector<std::shared_ptr<Main::Classes::PartyRoom>>> m_partyRoomByClanId;
			std::unordered_map<std::uint16_t, std::pair<std::shared_ptr<Main::Classes::PartyRoom>, std::shared_ptr<Main::Classes::PartyRoom>>> m_clanRooms;

		public:
			void addRoom(std::shared_ptr<Main::Classes::PartyRoom> room);
			bool removeExactRoom(std::uint16_t clanId, std::uint16_t roomNumber);
			void cleanupInactiveClanMatches();
			std::shared_ptr<Main::Classes::PartyRoom> getExactRoomFor(std::uint16_t clanId, std::uint16_t roomNumber);
			std::vector<Main::Structures::RegisteredClanInfo> getAllRegisteredClans() const;
			std::optional<std::uint16_t> getNextAvailableRoomNumberFor(std::uint16_t clanId) const;
			const std::vector<std::shared_ptr<Main::Classes::PartyRoom>>* getRoomsFor(std::uint16_t clanId) const;
			std::expected<bool, std::string> addClanMatch(std::uint16_t matchRoomNumber, std::shared_ptr<Main::Classes::PartyRoom> partyRoomA, std::shared_ptr<Main::Classes::PartyRoom> partyRoomB);
			std::expected<bool, std::string> removeClanMatch(std::uint16_t matchRoomNumber, bool removeParty = false);
			std::expected<std::pair<std::shared_ptr<Main::Classes::PartyRoom>, std::shared_ptr<Main::Classes::PartyRoom>>, std::string> getClanMatch(std::uint16_t matchRoomNumber) const;
			std::uint16_t getClanMatchRoomNumber(std::uint16_t clanId, std::uint16_t clanRoomNumber) const;
		};
	}
}
#endif
