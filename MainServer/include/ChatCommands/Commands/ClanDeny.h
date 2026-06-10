#ifndef DENY_CLAN_REQUEST_COMMAND_HEADER
#define DENY_CLAN_REQUEST_COMMAND_HEADER

#include "ChatCommands/ICommand.h"
#include "ChatCommands/ChatCommands.h"
#include "Utils/Utils.h"
#include "MainServer.h"
#include <source_location>
#include <charconv>

namespace Main
{
    namespace Command
    {
        class ClanDeny final : public ICommand
        {
        private:
            std::string m_targetNickname{};

            bool parseCommand(const std::string& providedCommand) override
            {
                std::smatch match;
                if (std::regex_match(providedCommand, match, this->m_pattern))
                {
                    m_targetNickname = match[2].str();
                    return true;
                }
                return false;
            }

        public:
            explicit ClanDeny(const Common::Enums::PlayerGrade requiredGrade)
                : ICommand{ requiredGrade, "/clandeny <Nickname>", R"(^(\S+)\s+(\S+)$)" }
            {
            }

            void execute(const std::string& command, std::shared_ptr<Main::Network::Session> session, MN::SessionsManager& sessionsManager,
                MC::RoomsManager&, MP::MainScheduler& scheduler, std::uint32_t, Main::MainServer&) override
            {
                if (!parseCommand(command))
                {
                    session->sendMessage("Error: Invalid command format. Use: /deny <Nickname>");
                    return;
                }

                if (m_targetNickname.size() > 20 || m_targetNickname.size() < 1)
                {
                    session->sendMessage("Error: Invalid nickname format. Nickname must be 1-20 characters");
                    return;
                }

                const std::uint32_t ownerAccountId = session->getAccountInfo().accountID;

                Main::Enums::DenyClanRequestResult result = scheduler.immediatePersist(std::source_location::current(),
                    &Main::Persistence::PersistentDatabase::denyClanRequest,ownerAccountId,m_targetNickname);

                switch (result)
                {
                case Main::Enums::DenyClanRequestResult::SUCCESS:
                    session->sendMessage("Success: Request from '" + m_targetNickname + "' has been denied.");
                    break;

                case Main::Enums::DenyClanRequestResult::NOT_IN_CLAN:
                    session->sendMessage("Error: You are not in a clan.");
                    break;

                case Main::Enums::DenyClanRequestResult::NOT_LEADER:
                    session->sendMessage("Error: Only clan leaders can deny membership requests.");
                    break;

                case Main::Enums::DenyClanRequestResult::TARGET_NOT_FOUND:
                    session->sendMessage("Error: Player '" + m_targetNickname + "' not found.");
                    break;

                case Main::Enums::DenyClanRequestResult::TARGET_NOT_IN_REQUESTS:
                    session->sendMessage("Error: Player '" + m_targetNickname + "' has not requested to join your clan.");
                    break;

                case Main::Enums::DenyClanRequestResult::DB_ERROR:
                default:
                    session->sendMessage("Error: Failed to deny clan request (database error). Try again later.");
                    break;
                }
            }
        };

        REGISTER_CMD(ClanDeny, Common::Enums::PlayerGrade::GRADE_NORMAL)
    };
}

#endif