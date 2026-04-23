#ifndef CLAN_PARTY_SETTINGS_HANDLER_H
#define CLAN_PARTY_SETTINGS_HANDLER_H

#include "../../Network/MainSession.h"
#include "Network/Packet.h"
#include "../../Classes/PartiesManager.h"
#include <memory>

namespace Main
{
    namespace Handlers
    {
        enum ClanChangeLeaderExtra
        {
            CHANGE_LEADER_SUCCESS = 1,
            CHANGE_LEADER_FAIL = 2,
            CHANGE_LEADER_NEW_LEADER_DOESNT_EXIST = 13,
            CHANGE_LEADER_NO_PERMISSION = 16
        };

        template<std::size_t OrderId>
        inline void handlePartySettings(const Common::Network::Packet& request, std::shared_ptr<Main::Network::Session> session,
            Main::Classes::PartiesManager& clansManager)
        {
            const auto& ainfo = session->getAccountInfo();
            auto clanRoom = clansManager.getExactRoomFor(ainfo.clanId, session->getPlayer().getPartyRoomNumber());

            if (!clanRoom || !clanRoom->isLeader(ainfo.uniqueId.session))
                return;

            if constexpr (OrderId == 116)
            {
                clanRoom->updateMap(request.getOption());
                clanRoom->updateMode(request.getExtra());
                clanRoom->broadcast(const_cast<Common::Network::Packet&>(request));
            }
            else if constexpr (OrderId == 117)
            {
                clanRoom->updatePlayersPerTeam(request.getOption());
                auto response = request;
                response.setExtra(1);
                clanRoom->broadcast(response);
            }
        }

        inline void handlePartyLeaderChange(const Common::Network::Packet& request, std::shared_ptr<Main::Network::Session> session, 
            Main::Classes::PartiesManager& clansManager)
        {
            const auto& ainfo = session->getAccountInfo();
            auto response = request;
            if (auto clanRoom = clansManager.getExactRoomFor(ainfo.clanId, session->getPlayer().getPartyRoomNumber()))
            {
                if (!clanRoom->isLeader(ainfo.uniqueId.session))
                {
                    response.setExtra(ClanChangeLeaderExtra::CHANGE_LEADER_NO_PERMISSION);
                    session->asyncWrite(response);
                }
                else if (!clanRoom->changeLeaderTo(request.getOption()))
                {
                    response.setExtra(ClanChangeLeaderExtra::CHANGE_LEADER_NEW_LEADER_DOESNT_EXIST);
                    session->asyncWrite(response);
                }
                else
                {
                    response.setExtra(ClanChangeLeaderExtra::CHANGE_LEADER_SUCCESS);
                    clanRoom->broadcast(response);
                }
            }
            else
            {
                response.setExtra(ClanChangeLeaderExtra::CHANGE_LEADER_FAIL);
                session->asyncWrite(response);
            }
        }
    }
}

#endif
