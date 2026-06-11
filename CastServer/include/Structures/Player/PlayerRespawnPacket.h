#ifndef CAST_PLAYER_RESPAWN_PACKET_H
#define CAST_PLAYER_RESPAWN_PACKET_H

#include <cstdint>
#include "Structures/AccountInfo/MainAccountUniqueId.h"

namespace Cast
{
    namespace Structures
    {
        struct PlayerRespawnPacket
        {
            std::uint16_t x = 0;
            std::uint16_t y = 0;
            std::uint16_t z = 0;
            std::uint16_t w = 0;
            Main::Structures::UniqueId targetUniqueId{};

            bool isNaNOrInfinity(std::uint16_t half) const
            {
                std::uint16_t exponent = (half >> 10) & 0x1F;
                return (exponent == 0x1F);
            }

            bool isBad() const
            {
                return isNaNOrInfinity(x) || isNaNOrInfinity(y) || isNaNOrInfinity(z) || isNaNOrInfinity(w);
            }
        };
    }
}

#endif
