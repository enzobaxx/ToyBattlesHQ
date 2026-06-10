#ifndef GETCLANREQUESTS_COMMAND_HEADER
#define GETCLANREQUESTS_COMMAND_HEADER

#include "ChatCommands/ICommand.h"
#include "ChatCommands/ChatCommands.h"
#include "Utils/Utils.h"
#include "MainServer.h"
#include <source_location>
#include <sstream>

namespace Main
{
    namespace Command
    {
        struct GetClanRequests final : public ICommand
        {
              explicit GetClanRequests(const Common::Enums::PlayerGrade requiredGrade)
                : ICommand{ requiredGrade, "/getclanrequests", R"(^(\S+)$)" }
            {
            }

            void execute(const std::string& command, std::shared_ptr<Main::Network::Session> session, MN::SessionsManager& sessionsManager,
                MC::RoomsManager&, MP::MainScheduler& scheduler, std::uint32_t, Main::MainServer&) override
            {
                const std::uint32_t accountID = session->getAccountInfo().accountID;

                auto [result, nicknames] = scheduler.immediatePersist(std::source_location::current(),&Main::Persistence::PersistentDatabase::getPendingRequests,
                    accountID);

                switch (result)
                {
                case Main::Enums::GetPendingRequestsResult::SUCCESS:
                {
                    std::ostringstream message;
                    message << "Pending clan requests (" << nicknames.size() << "):";
                    session->sendMessage(message.str());

                    for (const auto& nickname : nicknames)
                    {
                        session->sendMessage(" - " + nickname);
                    }
                    break;
                }

                case Main::Enums::GetPendingRequestsResult::NOT_IN_CLAN:
                    session->sendMessage("Error: You are not in a clan.");
                    break;

                case Main::Enums::GetPendingRequestsResult::NOT_LEADER:
                    session->sendMessage("Error: Only clan leaders can view pending requests.");
                    break;

                case Main::Enums::GetPendingRequestsResult::NO_PENDING_REQUESTS:
                    session->sendMessage("There are no pending requests for your clan.");
                    break;

                case Main::Enums::GetPendingRequestsResult::DB_ERROR:
                default:
                    session->sendMessage("Error: Failed to retrieve pending requests (database error). Try again later.");
                    break;
                }
            }
        };

        REGISTER_CMD(GetClanRequests, Common::Enums::PlayerGrade::GRADE_NORMAL)
    };
}

#endif