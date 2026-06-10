#ifndef EQUIPPED_ITEMS_HANDLER
#define EQUIPPED_ITEMS_HANDLER

#include "Network/Sessions/MainSession.h"
#include "Network/Packet.h"
#include "MainEnums.h"
#include "Structures/Item/MainBoughtItem.h"

namespace Main
{
	namespace Handlers
	{
		inline void handleEquippedItemSwitch(const Common::Network::Packet& request, std::shared_ptr<Main::Network::Session> session,
			Main::Classes::RoomsManager& roomsManager)
		{
			START_BENCHMARK

			if (request.getExtra() == 51) 
			{
				for (std::size_t idx = 0; idx < request.getOption(); ++idx)
				{
					const std::uint16_t character = Main::Details::parseData<std::uint16_t>(request, idx * 12);
					const std::uint32_t itemNumber = Main::Details::parseData<std::uint32_t>(request, idx * 12 + 4);
					session->switchItemEquip(character, itemNumber);
				}
			}
			else if (request.getMission() == 1)
			{ // Remove a single item based on the passed extra, which is the EquippedItem's type
				session->unequipItem(request.getExtra()); 
			}
			else if (request.getMission() == 0)
			{
				for (std::size_t idx = 0; idx < request.getOption(); ++idx)
				{
					const Main::Structures::ItemSerialInfo itemSerialInfo = Main::Details::parseData<Main::Structures::ItemSerialInfo>(request, idx * 8);
					const std::uint64_t val = Main::Details::parseData<std::uint64_t>(request, idx * 8);

					if (val < static_cast<std::uint64_t>(Common::Constants::maxItemType))
					{ // val == equippedItem.type
						session->unequipItem(val); 
					}
					else
					{
						session->equipItem(itemSerialInfo.itemNumber);
					}
				}
			}

			Common::Network::Packet response;
			response.setTcpHeader(request.getSession(), Common::Enums::USER_LARGE_ENCRYPTION);
			response.setCommand(request.getOrder(), 0, request.getExtra(), request.getOption());
			session->asyncWrite(response);

			END_BENCHMARK(handleEquippedItemSwitch, session)
		}
	}
}

#endif