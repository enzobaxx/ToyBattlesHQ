#ifndef TRANSFERCLAN_COMMAND_HEADER
#define TRANSFERCLAN_COMMAND_HEADER

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
        class TransferClan final : public ICommand
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
            explicit TransferClan(const Common::Enums::PlayerGrade requiredGrade)
                : ICommand{ requiredGrade, "/transferclan <Nickname>", R"(^(\S+)\s+(\S+)$)" }
            {
            }

            void execute(const std::string& command, std::shared_ptr<Main::Network::Session> session, MN::SessionsManager& sessionsManager,
                MC::RoomsManager&, MP::MainScheduler& scheduler, std::uint32_t, Main::MainServer&) override
            {
                if (!parseCommand(command))
                {
                    session->sendMessage("Error: Invalid command format. Use: /transferclan <Nickname>");
                    return;
                }

                if (m_targetNickname.size() > 20 || m_targetNickname.size() < 1)
                {
                    session->sendMessage("Error: Invalid nickname format. Nickname must be 1-20 characters");
                    return;
                }

                const std::uint32_t ownerAccountId = session->getAccountInfo().accountID;

                Main::Enums::TransferOwnershipResult result = scheduler.immediatePersist(std::source_location::current(),
                    &Main::Persistence::PersistentDatabase::transferOwnership,ownerAccountId,m_targetNickname);

                switch (result)
                {
                case Main::Enums::TransferOwnershipResult::SUCCESS:
                {
                    session->sendMessage("Success: You have transferred clan leadership to '" + m_targetNickname + "'.");
                    break;
                }

                case Main::Enums::TransferOwnershipResult::NOT_IN_CLAN:
                    session->sendMessage("Error: You are not in a clan.");
                    break;

                case Main::Enums::TransferOwnershipResult::NOT_LEADER:
                    session->sendMessage("Error: Only the clan leader can transfer ownership.");
                    break;

                case Main::Enums::TransferOwnershipResult::TARGET_NOT_FOUND:
                    session->sendMessage("Error: Player '" + m_targetNickname + "' not found.");
                    break;

                case Main::Enums::TransferOwnershipResult::TARGET_NOT_IN_CLAN:
                    session->sendMessage("Error: Player '" + m_targetNickname + "' is not in your clan.");
                    break;

                case Main::Enums::TransferOwnershipResult::TARGET_IS_SELF:
                    session->sendMessage("Error: You cannot transfer ownership to yourself.");
                    break;

                case Main::Enums::TransferOwnershipResult::DB_ERROR:
                default:
                    session->sendMessage("Error: Failed to transfer clan ownership (database error). Try again later.");
                    break;
                }
            }
        };

        REGISTER_CMD(TransferClan, Common::Enums::PlayerGrade::GRADE_NORMAL)
    };
}

#endif