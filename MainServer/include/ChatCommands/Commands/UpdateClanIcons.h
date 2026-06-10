#ifndef UPDATE_CLANICONS_HEADER
#define UPDATE_CLANICONS_HEADER

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
        class UpdateFrontIcon final : public ICommand
        {
        private:
            std::uint16_t m_iconId{};

            bool parseCommand(const std::string& providedCommand) override
            {
                std::smatch match;
                if (std::regex_match(providedCommand, match, this->m_pattern))
                {
                    const std::string& iconStr = match[2].str();
                    auto [ptr, ec] = std::from_chars(iconStr.data(), iconStr.data() + iconStr.size(), m_iconId);

                    if (ec != std::errc{} || ptr != iconStr.data() + iconStr.size())
                    {
                        return false;
                    }

                    return true;
                }
                return false;
            }

        public:
            explicit UpdateFrontIcon(const Common::Enums::PlayerGrade requiredGrade)
                : ICommand{ requiredGrade, "/updatefronticon <ID>", R"(^(\S+)\s+(\d+)$)" }
            {
            }

            void execute(const std::string& command, std::shared_ptr<Main::Network::Session> session, MN::SessionsManager& sessionsManager,
                MC::RoomsManager&, MP::MainScheduler& scheduler, std::uint32_t, Main::MainServer&) override
            {
                if (!parseCommand(command))
                {
                    session->sendMessage("Error: Invalid command format. Use: /updatefronticon <ID>");
                    return;
                }

                if (m_iconId < 1 || m_iconId > 295)
                {
                    session->sendMessage("Error: Front icon ID must be between 1 and 295.");
                    return;
                }

                const std::uint32_t accountId = session->getAccountInfo().accountID;

                Main::Enums::UpdateClanIconResult result = scheduler.immediatePersist(std::source_location::current(),
                    &Main::Persistence::PersistentDatabase::updateClanFrontIcon,accountId,m_iconId);

                switch (result)
                {
                case Main::Enums::UpdateClanIconResult::SUCCESS:
                    session->sendMessage("Success: Clan front icon has been updated to ID: " + std::to_string(m_iconId));
                    break;

                case Main::Enums::UpdateClanIconResult::NOT_IN_CLAN:
                    session->sendMessage("Error: You are not in a clan.");
                    break;

                case Main::Enums::UpdateClanIconResult::NOT_LEADER:
                    session->sendMessage("Error: Only the clan leader can update clan icons.");
                    break;

                case Main::Enums::UpdateClanIconResult::INVALID_FRONT_ICON:
                    session->sendMessage("Error: Front icon ID must be between 1 and 295.");
                    break;

                case Main::Enums::UpdateClanIconResult::DB_ERROR:
                default:
                    session->sendMessage("Error: Failed to update clan front icon (database error). Try again later.");
                    break;
                }
            }
        };
        REGISTER_CMD(UpdateFrontIcon, Common::Enums::PlayerGrade::GRADE_NORMAL)


        class UpdateBackIcon final : public ICommand
        {
        private:
            std::uint16_t m_iconId{};

            bool parseCommand(const std::string& providedCommand) override
            {
                std::smatch match;
                if (std::regex_match(providedCommand, match, this->m_pattern))
                {
                    const std::string& iconStr = match[2].str();
                    auto [ptr, ec] = std::from_chars(iconStr.data(), iconStr.data() + iconStr.size(), m_iconId);

                    if (ec != std::errc{} || ptr != iconStr.data() + iconStr.size())
                    {
                        return false;
                    }

                    return true;
                }
                return false;
            }

        public:
            explicit UpdateBackIcon(const Common::Enums::PlayerGrade requiredGrade)
                : ICommand{ requiredGrade, "/updatebackicon <ID>", R"(^(\S+)\s+(\d+)$)" }
            {
            }

            void execute(const std::string& command, std::shared_ptr<Main::Network::Session> session, MN::SessionsManager& sessionsManager,
                MC::RoomsManager&, MP::MainScheduler& scheduler, std::uint32_t, Main::MainServer&) override
            {
                if (!parseCommand(command))
                {
                    session->sendMessage("Error: Invalid command format. Use: /updatebackicon <ID>");
                    return;
                }

                if (m_iconId < 1 || m_iconId > 169)
                {
                    session->sendMessage("Error: Back icon ID must be between 1 and 169.");
                    return;
                }

                const std::uint32_t accountId = session->getAccountInfo().accountID;

                Main::Enums::UpdateClanIconResult result = scheduler.immediatePersist(std::source_location::current(),
                    &Main::Persistence::PersistentDatabase::updateClanBackIcon,accountId,m_iconId);

                switch (result)
                {
                case Main::Enums::UpdateClanIconResult::SUCCESS:
                    session->sendMessage("Success: Clan back icon has been updated to ID: " + std::to_string(m_iconId));
                    break;

                case Main::Enums::UpdateClanIconResult::NOT_IN_CLAN:
                    session->sendMessage("Error: You are not in a clan.");
                    break;

                case Main::Enums::UpdateClanIconResult::NOT_LEADER:
                    session->sendMessage("Error: Only the clan leader can update clan icons.");
                    break;

                case Main::Enums::UpdateClanIconResult::INVALID_BACK_ICON:
                    session->sendMessage("Error: Back icon ID must be between 1 and 169.");
                    break;

                case Main::Enums::UpdateClanIconResult::DB_ERROR:
                default:
                    session->sendMessage("Error: Failed to update clan back icon (database error). Try again later.");
                    break;
                }
            }
        };
        REGISTER_CMD(UpdateBackIcon, Common::Enums::PlayerGrade::GRADE_NORMAL)
    };
}

#endif