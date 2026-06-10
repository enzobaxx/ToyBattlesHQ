#ifndef MAIN_EVENTS_LIST_H
#define MAIN_EVENTS_LIST_H

#include "Structures/AccountInfo/MainAccountUniqueId.h"
#include "Enums/RoomEnums.h"
#include <vector>
#include "Macros.h"

namespace Main
{
    namespace Structures
    {
PACK_PUSH(1)
        struct SingleModeEvent
        {
            Common::Enums::GameModes gameMode{};
            time32_t startDate{};
            time32_t endDate{};
        };
PACK_POP()

PACK_PUSH(1)
        struct SingleMapEvent
        {
            Common::Enums::GameMaps gameMap{};
            time32_t startDate{};
            time32_t endDate{};
        };
PACK_POP()
    }
}

#endif