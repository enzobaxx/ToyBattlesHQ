#ifndef DISBANDCLAN_COMMAND_HEADER
#define DISBANDCLAN_COMMAND_HEADER

#include "ChatCommands/ICommand.h"
#include "ChatCommands/ChatCommands.h"
#include "Utils/Utils.h"
#include "MainServer.h"
#include <source_location>

namespace Main
{
    namespace Command
    {
        struct DisbandClan final : public ICommand
        {
            explicit DisbandClan(const Common::Enums::PlayerGrade requiredGrade)
                : ICommand{ requiredGrade, "/disbandclan", R"(^(\S+)$)" }
            {
            }

            void execute(const std::string& command, std::shared_ptr<Main::Network::Session> session, MN::SessionsManager& sessionsManager,
                MC::RoomsManager&, MP::MainScheduler& scheduler, std::uint32_t, Main::MainServer&) override
            {
                const std::uint32_t ownerAccountId = session->getAccountInfo().accountID;

                Main::Enums::DisbandClanResult result = scheduler.immediatePersist(std::source_location::current(),
                    &Main::Persistence::PersistentDatabase::disbandClan,ownerAccountId);

                switch (result)
                {
                case Main::Enums::DisbandClanResult::SUCCESS:
                {
                    session->sendMessage("Success: Your clan has been disbanded. All members have been removed and all clan stats reset. Relog.");
                    break;
                }

                case Main::Enums::DisbandClanResult::NOT_IN_CLAN:
                    session->sendMessage("Error: You are not in a clan.");
                    break;

                case Main::Enums::DisbandClanResult::NOT_LEADER:
                    session->sendMessage("Error: Only the clan leader can disband the clan.");
                    break;

                case Main::Enums::DisbandClanResult::DB_ERROR:
                default:
                    session->sendMessage("Error: Failed to disband clan (database error). Try again later.");
                    break;
                }
            }
        };

        REGISTER_CMD(DisbandClan, Common::Enums::PlayerGrade::GRADE_NORMAL)
    };
}

#endif