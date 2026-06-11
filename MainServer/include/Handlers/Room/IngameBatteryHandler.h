#ifndef ROOM_INGAME_BATTERYHANDLER_H
#define ROOM_INGAME_BATTERYHANDLER_H

#include "Network/Sessions/MainSession.h"
#include "Network/Packet.h"
#include "Managers/RoomsManager.h"
#include <array>
#include <random>

namespace Main
{
    namespace Handlers
    {
        inline void handleBattery(const Common::Network::Packet& request, std::shared_ptr<Main::Network::Session> session,
            Main::Classes::RoomsManager& roomsManager, const Main::Structures::UniqueId& targetUniqueId)
        {
            if (Main::Classes::Room* room = roomsManager.getRoomByNumber(session->getPlayer().matchContext.roomNumber))
            {
                std::array<std::uint32_t, 4> batteryValues = { 0, 30, 50, 100 };
                std::array<int, 4> weights = { 60, 20, 15, 5 };

                static std::random_device rd;
                static std::mt19937 gen(rd());
                static std::discrete_distribution<> dist(weights.begin(), weights.end());

                std::uint32_t selectedBattery = batteryValues[dist(gen)];

                if (!selectedBattery) return;

                auto targetSession = room->getPlayer(targetUniqueId);
                if (targetSession)
                {
                    Common::Network::Packet response;
                    response.setTcpHeader(targetSession->getId(), Common::Enums::USER_LARGE_ENCRYPTION);
                    response.setCommand(request.getOrder(), 0, 1, selectedBattery);
                    response.setData(reinterpret_cast<std::uint8_t*>(&selectedBattery), sizeof(selectedBattery));
                    targetSession->asyncWrite(response);
                    targetSession->getPlayer().matchContext.batteryObtainedInMatch += selectedBattery;
                }
            }
        }
    }
}

#endif
