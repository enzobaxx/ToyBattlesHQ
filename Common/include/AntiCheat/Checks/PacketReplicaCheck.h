#ifndef PACKET_REPLICATION_CHECK
#define PACKET_REPLICATION_CHECK

#include "../Interfaces.h"
#include <unordered_map>
#include <deque>
#include <format>
#include "../../Utils/Utils.h"
#include <functional>

namespace Ac
{
    class PacketReplicationChecker : public IACChecker<PacketReplicationEvent>
    {
    private:
        // [SEID] -> [PacketHash] -> [timestamps...]
        std::unordered_map<uint32_t, std::unordered_map<size_t, std::vector<std::uint64_t>>> playerData;
        static inline std::uint64_t analysisWindowMs = 20000;
        static inline std::size_t replicaThreshold = 5;

        std::string floatToString(float value, int precision = 2)
        {
            std::ostringstream oss;
            oss << std::fixed << std::setprecision(precision) << value;
            return oss.str();
        }

        std::size_t calculatePacketHash(const std::vector<std::uint8_t>& data)
        {
            return std::hash<std::string_view>{}(std::string_view(reinterpret_cast<const char*>(data.data()), data.size()));
        }

    public:
        std::optional<ACFlag> processEvent(const PacketReplicationEvent& event) override
        {
            const size_t packetHash = calculatePacketHash(event.data);
            auto& playerPackets = playerData[event.session->getId()];

            for (auto& [hash, timestamps] : playerPackets)
            {
                timestamps.erase(
                    std::remove_if(timestamps.begin(), timestamps.end(), [&](std::uint64_t ts) {
                        return (event.eventTime - ts) > analysisWindowMs;
                        }),
                    timestamps.end()
                );
            }

            auto& timestamps = playerPackets[packetHash];
            timestamps.push_back(event.eventTime);

            if (timestamps.size() >= replicaThreshold)
            {
                const ACFlag flag{
                    event.session->getAccountId(),
                    "Possible Packet Replication (e.g. WPE)",
                    "Packet replication detected (ID " + std::to_string(event.packetId) + "): " +
                    std::to_string(timestamps.size()) + " identical packets within " +
                    floatToString(analysisWindowMs / 1000.0f) + "s window"
                };

                playerData[event.session->getId()].clear();
                event.session->closeSocket();

                return flag;
            }

            return std::nullopt;
        }
    };
}

#endif