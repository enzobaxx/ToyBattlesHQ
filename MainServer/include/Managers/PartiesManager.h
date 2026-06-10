#ifndef CLANS_MANAGER_H
#define CLANS_MANAGER_H

#include <unordered_map>
#include <vector>
#include <optional>
#include <memory>
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
			void addRoom(std::shared_ptr<Main::Classes::PartyRoom> room)
			{
				std::uint16_t clanId = room->getClanId();
				std::uint16_t roomNumber = room->getRoomNumber();
				m_partyRoomByClanId[clanId].push_back(room);
				cleanupInactiveClanMatches();
			}

			bool removeExactRoom(std::uint16_t clanId, std::uint16_t roomNumber)
			{
				auto it = m_partyRoomByClanId.find(clanId);
				if (it == m_partyRoomByClanId.end())
				{
					return false;
				}

				auto& clanRooms = it->second;
				auto roomIt = std::find_if(clanRooms.begin(), clanRooms.end(),
					[roomNumber](const std::shared_ptr<Main::Classes::PartyRoom>& room) {
						return room->getRoomNumber() == roomNumber;
					});

				if (roomIt != clanRooms.end())
				{
					(*roomIt)->removeAllPlayers();

					for (auto& matchIt : m_clanRooms)
					{
						if (matchIt.second.first == *roomIt || matchIt.second.second == *roomIt)
						{
							if (matchIt.second.first == *roomIt) matchIt.second.first.reset();
							if (matchIt.second.second == *roomIt) matchIt.second.second.reset();
						}
					}

					clanRooms.erase(roomIt);

					if (clanRooms.empty())
					{
						m_partyRoomByClanId.erase(it);
					}
					return true;
				}
				
				return false;
			}

			void cleanupInactiveClanMatches()
			{
				std::vector<std::uint16_t> matchesToRemove;

				for (const auto& [matchRoomNumber, partyRooms] : m_clanRooms)
				{
					const auto& [partyRoomA, partyRoomB] = partyRooms;

					bool roomAExists = false;
					bool roomBExists = false;

					if (partyRoomA)
					{
						auto it = m_partyRoomByClanId.find(partyRoomA->getClanId());
						if (it != m_partyRoomByClanId.end())
						{
							const auto& clanRooms = it->second;
							auto roomIt = std::find_if(clanRooms.begin(), clanRooms.end(),
								[&](const std::shared_ptr<Main::Classes::PartyRoom>& room) {
									return room == partyRoomA;
								});
							roomAExists = (roomIt != clanRooms.end());
						}
					}

					if (partyRoomB)
					{
						auto it = m_partyRoomByClanId.find(partyRoomB->getClanId());
						if (it != m_partyRoomByClanId.end())
						{
							const auto& clanRooms = it->second;
							auto roomIt = std::find_if(clanRooms.begin(), clanRooms.end(),
								[&](const std::shared_ptr<Main::Classes::PartyRoom>& room) {
									return room == partyRoomB;
								});
							roomBExists = (roomIt != clanRooms.end());
						}
					}

					if (!roomAExists && !roomBExists)
					{
						matchesToRemove.push_back(matchRoomNumber);
					}
				}

				for (auto matchRoomNumber : matchesToRemove)
				{
					m_clanRooms.erase(matchRoomNumber);
				}
			}

			std::shared_ptr<Main::Classes::PartyRoom> getExactRoomFor(std::uint16_t clanId, std::uint16_t roomNumber)
			{
				auto it = m_partyRoomByClanId.find(clanId);
				if (it == m_partyRoomByClanId.end()) return nullptr;

				auto& clanRooms = it->second;
				auto roomIt = std::find_if(clanRooms.begin(), clanRooms.end(),
					[roomNumber](const std::shared_ptr<Main::Classes::PartyRoom>& room) {
						return room->getRoomNumber() == roomNumber;
					});

				return (roomIt != clanRooms.end()) ? *roomIt : nullptr;
			}

			std::vector<Main::Structures::RegisteredClanInfo> getAllRegisteredClans() const
			{
				std::vector<Main::Structures::RegisteredClanInfo> registeredClans;
				for (const auto& [clanId, clanRooms] : m_partyRoomByClanId)
				{
					for (const auto& clanRoom : clanRooms)
					{
						if (auto info = clanRoom->getInfo(); info && clanRoom->isRegistered())
						{
							registeredClans.emplace_back(*info);
						}
					}
				}
				return registeredClans;
			}

			std::optional<std::uint16_t> getNextAvailableRoomNumberFor(std::uint16_t clanId) const
			{
				auto it = m_partyRoomByClanId.find(clanId);
				if (it == m_partyRoomByClanId.end()) return 1;

				const auto& clanRooms = it->second;
				for (std::uint16_t i = 1; i <= Common::Constants::maxPartiesPerClan; ++i)
				{
					auto roomIt = std::find_if(clanRooms.begin(), clanRooms.end(),
						[i](const std::shared_ptr<Main::Classes::PartyRoom>& room) {
							return room->getRoomNumber() == i;
						});

					if (roomIt == clanRooms.end()) return i;
				}
				return std::nullopt;
			}

			const std::vector<std::shared_ptr<Main::Classes::PartyRoom>>* getRoomsFor(std::uint16_t clanId) const
			{
				auto it = m_partyRoomByClanId.find(clanId);
				return it != m_partyRoomByClanId.end() ? &it->second : nullptr;
			}

			std::expected<bool, std::string> addClanMatch(std::uint16_t matchRoomNumber, std::shared_ptr<Main::Classes::PartyRoom> partyRoomA, std::shared_ptr<Main::Classes::PartyRoom> partyRoomB)
			{
				if (!partyRoomA || !partyRoomB)
				{
					return std::unexpected("Invalid party room pointer");
				}

				if (partyRoomA->getClanMatchRoomNumber() != 0)
				{
					return std::unexpected("Party room " + std::to_string(partyRoomA->getRoomNumber()) +
						" is already in a clan match (room: " + std::to_string(partyRoomA->getClanMatchRoomNumber()) + ")");
				}

				if (partyRoomB->getClanMatchRoomNumber() != 0)
				{
					return std::unexpected("Party room " + std::to_string(partyRoomB->getRoomNumber()) +
						" is already in a clan match (room: " + std::to_string(partyRoomB->getClanMatchRoomNumber()) + ")");
				}

				m_clanRooms[matchRoomNumber] = { partyRoomA, partyRoomB };
				partyRoomA->setClanMatchRoomNumber(matchRoomNumber);
				partyRoomB->setClanMatchRoomNumber(matchRoomNumber);

				return true;
			}

			std::expected<bool, std::string> removeClanMatch(std::uint16_t matchRoomNumber, bool removeParty = false)
			{

				auto it = m_clanRooms.find(matchRoomNumber);
				if (it == m_clanRooms.end())
				{
					return std::unexpected("Clan match not found for room: " + std::to_string(matchRoomNumber));
				}

				auto& [partyRoomA, partyRoomB] = it->second;

				if (partyRoomA) partyRoomA->setClanMatchRoomNumber(0);
				if (partyRoomB) partyRoomB->setClanMatchRoomNumber(0);

				if (removeParty)
				{
					if (partyRoomA) {
						removeExactRoom(partyRoomA->getClanId(), partyRoomA->getRoomNumber());
					}
					if (partyRoomB) {
						removeExactRoom(partyRoomB->getClanId(), partyRoomB->getRoomNumber());
					}
				}

				m_clanRooms.erase(it);
				return true;
			}

			std::expected<std::pair<std::shared_ptr<Main::Classes::PartyRoom>, std::shared_ptr<Main::Classes::PartyRoom>>, std::string> getClanMatch(std::uint16_t matchRoomNumber) const
			{
				auto it = m_clanRooms.find(matchRoomNumber);
				if (it == m_clanRooms.end())
				{
					return std::unexpected("Clan match not found");
				}
				return it->second;
			}

			std::uint16_t getClanMatchRoomNumber(std::uint16_t clanId, std::uint16_t clanRoomNumber) const
			{
				auto it = m_partyRoomByClanId.find(clanId);
				if (it == m_partyRoomByClanId.end()) return 0;

				for (const auto& partyRoom : it->second)
				{
					if (partyRoom->getRoomNumber() == clanRoomNumber)
					{
						return partyRoom->getClanMatchRoomNumber();
					}
				}
				return 0;
			}
		};
	}
}
#endif