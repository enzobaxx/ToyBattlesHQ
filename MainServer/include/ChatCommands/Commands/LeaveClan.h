#ifndef LEAVECLAN_COMMAND_HEADER
#define LEAVECLAN_COMMAND_HEADER

#include "ChatCommands/ICommand.h"
#include "ChatCommands/ChatCommands.h"
#include "Utils/Utils.h"
#include "MainServer.h"
#include <source_location>

namespace Main
{
    namespace Command
    {
        struct LeaveClan final : public ICommand
        {
            explicit LeaveClan(const Common::Enums::PlayerGrade requiredGrade)
                : ICommand{ requiredGrade, "/leaveclan", R"(^(\S+)$)" }
            {
            }

            void execute(const std::string& command, std::shared_ptr<Main::Network::Session> session, MN::SessionsManager& sessionsManager,
                MC::RoomsManager&, MP::MainScheduler& scheduler, std::uint32_t, Main::MainServer&) override
            {
                
                const std::uint32_t accountId = session->getAccountInfo().accountID;

                Main::Enums::LeaveClanResult result = scheduler.immediatePersist(std::source_location::current(),
                    &Main::Persistence::PersistentDatabase::leaveClan,accountId);

                switch (result)
                {
                case Main::Enums::LeaveClanResult::SUCCESS:
                {
                    session->sendMessage("Success: You have left the clan. All your clan stats have been reset. Relog.");
                    break;
                }

                case Main::Enums::LeaveClanResult::NOT_IN_CLAN:
                    session->sendMessage("Error: You are not in a clan.");
                    break;

                case Main::Enums::LeaveClanResult::IS_LEADER:
                    session->sendMessage("Error: Clan leaders cannot leave the clan. You must either:");
                    session->sendMessage("  - Transfer leadership to another member first");
                    session->sendMessage("  - Use /disbandclan if you want to delete it and remove all its members");
                    break;

                case Main::Enums::LeaveClanResult::DB_ERROR:
                default:
                    session->sendMessage("Error: Failed to leave clan (database error). Try again later.");
                    break;
                }
            }
        };

        REGISTER_CMD(LeaveClan, Common::Enums::PlayerGrade::GRADE_NORMAL)
    };
}

#endif