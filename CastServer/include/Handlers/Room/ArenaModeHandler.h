#ifndef CAST_ARENA_MODE_HANDLER_H
#define CAST_ARENA_MODE_HANDLER_H

#include "Managers/RoomsManager.h"
#include "Rooms/Room.h"
#include <thread>
#include <chrono>

namespace Cast
{
    namespace Handlers
    {
        inline void handleArenaMode(Cast::Classes::RoomsManager& roomsManager, std::shared_ptr<Cast::Classes::Room> room)
        {
            room->m_hasMatchStarted = true;

            auto totalAlive = room->getTotalAlivePlayers();
            if (totalAlive <= 1 && !room->m_arenaRoundFinished)
            {
                room->m_arenaRoundFinished = true;
                room->broadcastMessage("[ROOM: " + std::to_string(room->getRoomNumber()) + "] Arena end. Wait 10 seconds...");
                std::thread([room]() {
                    room->shuffleCoordinates();
                    std::this_thread::sleep_for(std::chrono::seconds(10));
                    room->respawnEveryoneArena();
                }).detach();
            }
        }
    }
}

#endif
