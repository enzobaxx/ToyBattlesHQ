#ifndef MAILBOX_GIFT_DISPLAY_HANDLER_H
#define MAILBOX_GIFT_DISPLAY_HANDLER_H

#include "Network/Sessions/MainSession.h"
#include "Network/MainSessionManager.h"
#include "Network/Packet.h"
#include "MainEnums.h"
#include "Structures/Mailbox/Mailbox.h"

#include <algorithm>
#include <cstring>
#include <Macros.h>


namespace Main
{
    namespace Handlers
    {
		// Note: This is not how the original server works
		// Here, the client expects a specific packet, but for simplicity I re-used existing packets (like spawning items)
		// Since we never send back the packet that the client actually wants, the "giftbox notice" will remain even if a gift box is opened. This is "fixed" by relogging
		// Also, after opening a giftbox, said giftbox will remain there until the mailbox is closed (although opening the same giftbox won't work, as it gets deleted from the session
		// after the first deletion)
		inline void handleMailboxGiftSend(const Common::Network::Packet& request, std::shared_ptr<Main::Network::Session> session, std::uint32_t serverId)
		{
			auto response = request;
			auto latestItemNumber = session->getPlayer().getInventory().getLatestItemNumber();
			START_BENCHMARK
			if (request.getExtra() == 53)
			{
				const std::uint32_t totalSentGifts = Main::Details::parseData<std::uint32_t>(request); 
				for (std::uint32_t currentSentGift = 0; currentSentGift < totalSentGifts; ++currentSentGift)
				{
					const std::uint32_t timestamp = Main::Details::parseData<std::uint32_t>(request, 8 * (currentSentGift + 1)); 
					if (const auto itemIdOpt = session->getPlayer().getMailbox().getGiftbox(timestamp); itemIdOpt)
					{
						Main::Structures::Giftbox2 giftbox{ *itemIdOpt };
						giftbox.serialInfo.itemNumber = ++latestItemNumber;
						giftbox.serialInfo.m_serverId = serverId;
						session->getPlayer().getInventory().setLatestItemNumber(latestItemNumber);
						const std::uint32_t duration = Main::CdbUtils::getItemDuration(giftbox.itemId.itemId);
						giftbox.expiration = duration <= 3 ? duration : static_cast<time32_t>(std::time(0)) + duration;

						if (session->addItem(giftbox))
						{
							session->logItemInfo(giftbox.serialInfo.itemNumber, giftbox.itemId.itemId, giftbox.expiration, 
								"This item was received through the giftbox system");
							response.setData(reinterpret_cast<const std::uint8_t*>(&giftbox), sizeof(giftbox));
							session->asyncWrite(response);
							session->deleteGiftbox(timestamp);
						}
						else
						{
							session->sendMessage("Your inventory is full. Delete an item before opening the gift box");
						}
					}
					else
					{
						session->sendMessage("[handleMailboxGiftSend] error: mailbox with given timestamp not found");
					}
				}
			}
			END_BENCHMARK(handleMailboxGiftSend, session)
		}
    }
}

#endif