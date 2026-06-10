#ifndef ENLISTCLAN_COMMAND_HEADER
#define ENLISTCLAN_COMMAND_HEADER

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
        class Enlist final : public ICommand
        {
        private:
            std::string m_clanName{};

            bool parseCommand(const std::string& providedCommand) override
            {
                std::smatch match;
                if (std::regex_match(providedCommand, match, this->m_pattern))
                {
                    m_clanName = match[2].str();
                    return true;
                }
                return false;
            }

        public:
            explicit Enlist(const Common::Enums::PlayerGrade requiredGrade)
                : ICommand{ requiredGrade, "/enlist <ClanName>", R"(^(\S+)\s+(\S+)$)" }
            {
            }

            void execute(const std::string& command, std::shared_ptr<Main::Network::Session> session, MN::SessionsManager& sessionsManager,
                MC::RoomsManager&, MP::MainScheduler& scheduler, std::uint32_t, Main::MainServer&) override
            {
                if (!parseCommand(command))
                {
                    session->sendMessage("Error: Invalid command format. Use: /enlist <ClanName>");
                    return;
                }

                if (m_clanName.size() > 15 || m_clanName.size() < 4 ||
                    !std::regex_match(m_clanName, std::regex(R"(^[A-Za-z0-9]+$)")))
                {
                    session->sendMessage("Error: Clan name must be alphanumeric, at least 4 characters and at most 15 characters");
                    return;
                }

                const std::uint32_t accountID = session->getAccountInfo().accountID;

                Main::Enums::ClanEnlist result = scheduler.immediatePersist(std::source_location::current(),&Main::Persistence::PersistentDatabase::enlistToClan,
                    accountID,m_clanName);

                switch (result)
                {
                case Main::Enums::ClanEnlist::CLAN_ENLIST_SUCCESS:
                    session->sendMessage("Success. The clan leader will see your enlistment.");
                    break;

                case Main::Enums::ClanEnlist::CLAN_NOT_FOUND:
                    session->sendMessage("Error: Clan not found. Check the clan name and try again.");
                    break;

                case Main::Enums::ClanEnlist::USER_NOT_FOUND:
                    session->sendMessage("Error: There's something wrong with your accountID. Please contact the support team.");
                    break;

                case Main::Enums::ClanEnlist::USER_ALREADY_IN_CLAN: 
                    session->sendMessage("Error: You are already in a clan. Leave your current clan first.");
                    break;

                case Main::Enums::ClanEnlist::CLAN_FULL:
                    session->sendMessage("Error: This clan has reached the maximum member limit (30).");
                    break;

                case Main::Enums::ClanEnlist::CLAN_ENLIST_DB_ERROR:
                default:
                    session->sendMessage("Error: General database error. If this persists contact the support team.");
                    break;
                }
            }
        };

        REGISTER_CMD(Enlist, Common::Enums::PlayerGrade::GRADE_NORMAL)
    };
}

#endif