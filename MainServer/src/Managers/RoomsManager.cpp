
#include <functional>
#include "Rooms/Room.h"
#include "Managers/RoomsManager.h"
#include "Structures/Room/RoomsList.h"
#include "Rooms/RoomNumberManager.h"

namespace Main
{
	namespace Classes
	{
		void RoomsManager::addRoom(Main::Classes::Room&& room)
		{
			std::uint16_t roomNum = room.getRoomNumber();
			bool isClanRoom = room.isClanRoom();
			m_roomByNumber[roomNum] = std::move(room);
		}

		void RoomsManager::removeRoom(std::uint16_t roomNum, std::uint32_t extra)
		{
			auto it = m_roomByNumber.find(roomNum);
			if (it == m_roomByNumber.end())
			{
				return;
			}

			bool isClanRoom = it->second.isClanRoom();

			if (isClanRoom)
			{
				Main::Classes::RoomNumberGenerator<Main::Enums::RoomType::Clan>::getInstance().release(roomNum);
			}
			else
			{
				Main::Classes::RoomNumberGenerator<Main::Enums::RoomType::Room>::getInstance().release(roomNum);
			}

			it->second.removeAllPlayers(extra);
			m_roomByNumber.erase(it);
		}

		std::size_t RoomsManager::getTotalRooms() const
		{
			return m_roomByNumber.size();
		}

		std::vector<Main::Structures::SingleRoom> RoomsManager::getRoomsList()
		{
			std::vector<Main::Structures::SingleRoom> roomsList;
			roomsList.reserve(m_roomByNumber.size());

			for (auto it = m_roomByNumber.begin(); it != m_roomByNumber.end(); )
			{
				auto& room = it->second;

				if (room.getPlayersSize() == 0)
				{
					room.removeAllPlayers();
					it = m_roomByNumber.erase(it); 
					continue;
				}

				if (room.getRoomNumber() >= Common::Constants::clanRoomNumberStart)
				{
					++it;
					continue;
				}

				roomsList.emplace_back(room.getRoomInfo());
				++it;
			}

			return roomsList;
		}

		std::vector<Main::Structures::SingleRoom> RoomsManager::getClanRoomsList()
		{
			std::vector<Main::Structures::SingleRoom> clanRoomsList;
			clanRoomsList.reserve(m_roomByNumber.size());

			for (auto it = m_roomByNumber.begin(); it != m_roomByNumber.end(); )
			{
				auto& room = it->second;

				if (room.getPlayersSize() == 0)
				{
					room.removeAllPlayers();
					it = m_roomByNumber.erase(it);
					continue;
				}

				if (room.getRoomNumber() < Common::Constants::clanRoomNumberStart)
				{
					++it;
					continue;
				}

				clanRoomsList.emplace_back(room.getRoomInfo());
				++it;
			}

			return clanRoomsList;
		}

		Main::Classes::Room* RoomsManager::getRoomByNumber(std::uint16_t roomNumber)
		{
			auto it = m_roomByNumber.find(roomNumber);
			if (it != m_roomByNumber.end())
			{
				return &it->second; 
			}
			return nullptr;
		}
	};
}
