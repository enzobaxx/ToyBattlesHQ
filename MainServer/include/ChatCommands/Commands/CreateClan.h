#ifndef CREATECLAN_COMMAND_HEADER
#define CREATECLAN_COMMAND_HEADER

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
        class CreateClan final : public ICommand
        {
        private:
            std::string m_clanName{};
            std::uint16_t m_clanBackIcon{};
            std::uint16_t m_clanFrontIcon{};

            bool parseCommand(const std::string& providedCommand) override
            {
                std::smatch match;
                if (std::regex_match(providedCommand, match, this->m_pattern))
                {
                    m_clanName = match[2].str();

                    const std::string& backIconStr = match[3].str();
                    const std::string& frontIconStr = match[4].str();

                    auto [ptr1, ec1] = std::from_chars(backIconStr.data(), backIconStr.data() + backIconStr.size(), m_clanBackIcon);
                    auto [ptr2, ec2] = std::from_chars(frontIconStr.data(), frontIconStr.data() + frontIconStr.size(), m_clanFrontIcon);

                    if (ec1 != std::errc{} || ptr1 != backIconStr.data() + backIconStr.size() ||
                        ec2 != std::errc{} || ptr2 != frontIconStr.data() + frontIconStr.size())
                    {
                        return false;
                    }

                    return true;
                }
                return false;
            }

        public:
            explicit CreateClan(const Common::Enums::PlayerGrade requiredGrade)
                : ICommand{ requiredGrade, "/createclan <ClanName> <ClanBackIcon> <ClanFrontIcon>", R"(^(\S+)\s+(\S+)\s+(\d+)\s+(\d+)$)" }
            {
            }

            void execute(const std::string& command, std::shared_ptr<Main::Network::Session> session, MN::SessionsManager& sessionsManager,
                MC::RoomsManager&, MP::MainScheduler& scheduler, std::uint32_t, Main::MainServer&) override
            {
                if (!parseCommand(command))
                {
                    session->sendMessage("Error: Invalid command format.");
                    return;
                }

                const std::uint32_t accountID = session->getAccountInfo().accountID;
                if (scheduler.immediatePersist(std::source_location::current(),&Main::Persistence::PersistentDatabase::isUserInClan,accountID))
                {
                    session->sendMessage("Error: You are already in a clan");
                    return;
                }
                if (m_clanName.size() > 15 || m_clanName.size() < 4 || !std::regex_match(m_clanName, std::regex(R"(^[A-Za-z0-9]+$)")))
                {
                    session->sendMessage("Error: Clan name must be alphanumeric, at least 4 characters and most 15 characters");
                    return;
                }
                if (scheduler.immediatePersist(std::source_location::current(),&Main::Persistence::PersistentDatabase::clanExists, m_clanName))
                {
                    session->sendMessage("Error: Clan name already exists");
                    return;
                }
                if (m_clanBackIcon < 1 || m_clanBackIcon > 169)
                {
                    session->sendMessage("Error: ClanBackIcon must be between 1 and 169.");
                    return;
                }
                if (m_clanFrontIcon < 1 || m_clanFrontIcon > 295)
                {
                    session->sendMessage("Error: ClanFrontIcon must be between 1 and 295.");
                    return;
                }
                if (!scheduler.immediatePersist(std::source_location::current(),&Main::Persistence::PersistentDatabase::addClan,accountID,m_clanName,m_clanFrontIcon,
                    m_clanBackIcon))
                {
                    session->sendMessage("Error: Failed to create clan (general database error). Try again later");
                    return;
                }

                session->sendMessage("Success: Clan created successfully! Relog");
            }
        };

        REGISTER_CMD(CreateClan, Common::Enums::PlayerGrade::GRADE_NORMAL)
    };
}

#endif
