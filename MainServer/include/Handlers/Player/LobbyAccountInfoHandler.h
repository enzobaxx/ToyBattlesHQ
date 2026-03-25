#ifndef ACCOUNT_INFO_IN_LOBBY_HANDLER_H
#define ACCOUNT_INFO_IN_LOBBY_HANDLER_H

#include "Network/Session.h"
#include "Network/Packet.h"
#include "../../Structures/AccountInfo/MainLobbyAccountInfo.h"
#include "../../MainEnums.h"
#include "../../../include/Network/MainSession.h"
#include "../../../include/Network/MainSessionManager.h"
#include <ConstantDatabase/Structures/SetItemInfo.h>

namespace Main
{
	namespace Handlers
	{
        inline void handleLobbyAccountInfo(const Common::Network::Packet& request, std::shared_ptr<Main::Network::Session> session,
            Main::Network::SessionsManager& sessionsManager, std::uint32_t serverId, const Main::Structures::UniqueId& uniqueId)
        {
            START_BENCHMARK

            auto targetSession = sessionsManager.getSessionBySessionId(uniqueId.session);
            if (!targetSession) return;

            Main::Structures::LobbyAccountInfo lobbyAccountInfo(targetSession->getAccountInfo());
            auto& setItemsInstance = Common::ConstantDatabase::CdbSingleton<Common::ConstantDatabase::SetItemInfo>::getInstance();
            const std::size_t offset = targetSession->getAccountInfo().latestSelectedCharacter * Common::Enums::MAX_ITEMTYPE;
            const auto& targetEquippedItems = targetSession->getPlayer().getEquippedItems();

            std::uint32_t equippedScaffoldId = 0;
            std::uint32_t equippedDioramaId = 0;

            for (std::size_t i = 0; i < Common::Enums::MAX_ITEMTYPE; ++i)
            {
                if (offset + i >= targetEquippedItems.size())
                {
                    session->sendMessage("[handleLobbyAccountInfo] error offset + i >= targetEquippedItems.size()!");
                    return;
                }
                auto& itemByCharacter = targetEquippedItems[offset + i];

                if (!itemByCharacter.serialInfo.itemNumber)
                {
                    continue;
                }
                if (itemByCharacter.type >= Common::Enums::SET)
                {
                    if (itemByCharacter.type == 19) // scaffold
                    {
                        equippedScaffoldId = itemByCharacter.id;
                    }
                    else if (itemByCharacter.type == 20) // diorama
                    {
                        equippedDioramaId = itemByCharacter.id;
                    }
                    else if (auto entry = setItemsInstance.getEntry(itemByCharacter.id); entry)
                    {
                        for (auto currentTypeNotNull 
                            : Common::Utils::getPartTypesWhereSetItemInfoTypeNotNull(*entry, targetSession->getAccountInfo().latestSelectedCharacter))
                        {
                            lobbyAccountInfo.items[currentTypeNotNull] = (itemByCharacter.id);
                        }
                    }
                }
                else if (itemByCharacter.type >= Common::Enums::ItemType::HAIR && itemByCharacter.type <= Common::Enums::ItemType::GRENADE)
                {
                    lobbyAccountInfo.items[itemByCharacter.type] = (itemByCharacter.id);
                }
            }

            constexpr std::uint64_t mask23 = 0x7FFFFFull;
            lobbyAccountInfo.dioramaInfo = (std::uint64_t(equippedScaffoldId) & mask23) | ((std::uint64_t(equippedDioramaId) & mask23) << 23);

            Common::Network::Packet response;
            response.setTcpHeader(request.getSession(), Common::Enums::NO_ENCRYPTION);
            response.setCommand(request.getOrder(), 0, (targetSession->getAccountInfo().clanId >= 8 ? 1 : 0), serverId);
            response.setData(reinterpret_cast<std::uint8_t*>(&lobbyAccountInfo), sizeof(Main::Structures::LobbyAccountInfo));
            session->asyncWrite(response);

            END_BENCHMARK(handleLobbyAccountInfo, session)
        }
	}
}

#endif
