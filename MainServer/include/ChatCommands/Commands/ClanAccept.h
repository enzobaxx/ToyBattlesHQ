#ifndef ACCEPT_CLAN_REQUEST_COMMAND_HEADER
#define ACCEPT_CLAN_REQUEST_COMMAND_HEADER

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
        class ClanAccept final : public ICommand
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
            explicit ClanAccept(const Common::Enums::PlayerGrade requiredGrade)
                : ICommand{ requiredGrade, "/clanaccept <Nickname>", R"(^(\S+)\s+(\S+)$)" }
            {
            }

            void execute(const std::string& command, std::shared_ptr<Main::Network::Session> session, MN::SessionsManager& sessionsManager,
                MC::RoomsManager&, MP::MainScheduler& scheduler, std::uint32_t, Main::MainServer&) override
            {
                if (!parseCommand(command))
                {
                    session->sendMessage("Error: Invalid command format. Use: /accept <Nickname>");
                    return;
                }

                if (m_targetNickname.size() > 20 || m_targetNickname.size() < 1)
                {
                    session->sendMessage("Error: Invalid nickname format. Nickname must be 1-20 characters.");
                    return;
                }

                const std::uint32_t ownerAccountId = session->getAccountInfo().accountID;
                Main::Enums::AcceptClanRequestResult result = scheduler.immediatePersist(std::source_location::current(),
                    &Main::Persistence::PersistentDatabase::acceptClanRequest,ownerAccountId,m_targetNickname);

                switch (result)
                {
                case Main::Enums::AcceptClanRequestResult::SUCCESS:
                    session->sendMessage("Success: Player '" + m_targetNickname + "' has been accepted into your clan!");
                    break;

                case Main::Enums::AcceptClanRequestResult::NOT_IN_CLAN:
                    session->sendMessage("Error: You are not in a clan.");
                    break;

                case Main::Enums::AcceptClanRequestResult::NOT_LEADER:
                    session->sendMessage("Error: Only clan leaders can accept membership requests.");
                    break;

                case Main::Enums::AcceptClanRequestResult::TARGET_NOT_FOUND:
                    session->sendMessage("Error: Player '" + m_targetNickname + "' not found.");
                    break;

                case Main::Enums::AcceptClanRequestResult::TARGET_NOT_IN_REQUESTS:
                    session->sendMessage("Error: Player '" + m_targetNickname + "' has not requested to join your clan.");
                    break;

                case Main::Enums::AcceptClanRequestResult::TARGET_ALREADY_IN_CLAN:
                    session->sendMessage("Error: Player '" + m_targetNickname + "' is already in a clan.");
                    break;

                case Main::Enums::AcceptClanRequestResult::CLAN_FULL:
                    session->sendMessage("Error: Your clan has reached the maximum member limit (30).");
                    break;

                case Main::Enums::AcceptClanRequestResult::DB_ERROR:
                default:
                    session->sendMessage("Error: Failed to accept clan request (database error). Try again later.");
                    break;
                }
            }
        };

        REGISTER_CMD(ClanAccept, Common::Enums::PlayerGrade::GRADE_NORMAL)
    };
}

#endif