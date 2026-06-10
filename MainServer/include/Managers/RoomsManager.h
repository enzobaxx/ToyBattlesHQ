#ifndef ROOMS_MANAGER_H
#define ROOMS_MANAGER_H

#include <unordered_map>
#include <functional>
#include "Rooms/Room.h"
#include "Structures/Room/RoomsList.h"

namespace Main
{
	namespace Classes
	{
		class RoomsManager
		{
		private:
			std::unordered_map<std::uint16_t, Main::Classes::Room> m_roomByNumber{};

		public:
			void addRoom(Main::Classes::Room&& room);

			void removeRoom(std::uint16_t roomNum, std::uint32_t extra = 1);

			std::size_t getTotalRooms() const;

			std::vector<Main::Structures::SingleRoom> getRoomsList() /* const */;

			std::vector<Main::Structures::SingleRoom> getClanRoomsList();

			Main::Classes::Room* getRoomByNumber(std::uint16_t roomNumber);
		};
	}
}
#endif