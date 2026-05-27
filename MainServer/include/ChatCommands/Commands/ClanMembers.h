#ifndef CLAN_MEMBERS_COMMAND_HEADER
#define CLAN_MEMBERS_COMMAND_HEADER

#include "../ICommand.h"
#include "../ChatCommands.h"
#include "Utils/Utils.h"
#include "../../MainServer.h"
#include <source_location>
#include <charconv>
#include <vector>
#include <string>

namespace Main
{
    namespace Command
    {
        struct ClanMembers final : public ICommand
        {
            explicit ClanMembers(const Common::Enums::PlayerGrade requiredGrade)
                : ICommand{ requiredGrade, "/clanmembers", R"(^(\S+)$)" }
            {
            }

            void execute(const std::string& command, std::shared_ptr<Main::Network::Session> session, MN::SessionsManager& sessionsManager,
                MC::RoomsManager&, MP::MainScheduler& scheduler, std::uint32_t, Main::MainServer&) override
            {
                if (session->getAccountInfo().clanId < 8)
                {
                    session->sendMessage("Error: You must be in a clan.");
                    return;
                }

                std::vector<std::string> members = scheduler.immediatePersist(std::source_location::current(),
                    &Main::Persistence::PersistentDatabase::getClanMembers, session->getAccountInfo().clanId);

                if (members.empty())
                {
                    session->sendMessage("Error: Failed to retrieve clan members.");
                    return;
                }

                session->sendMessage("=== Clan Members ===");
                for (const auto& nickname : members)
                {
                    session->sendMessage(nickname);
                }
            }
        };

        REGISTER_CMD(ClanMembers, Common::Enums::PlayerGrade::GRADE_NORMAL)
    };
}

#endif