#ifndef BASE_ANNOUNCE_COMMAND_H
#define BASE_ANNOUNCE_COMMAND_H

#include "ChatCommands/ICommand.h"
#include "ChatCommands/ChatCommands.h"
#include "Utils/Utils.h"
#include "MainServer.h"

namespace Main
{
    namespace Command
    {
        //NO_CHECK
        class BaseAnnounceCommand : public ICommand
        {
        protected:
            std::string m_message;

            explicit BaseAnnounceCommand(const Common::Enums::PlayerGrade requiredGrade, const std::string& cmdDescr)
                : ICommand{ requiredGrade, cmdDescr, R"(^\S+\s+(.*)$)" }
            {
            }

            virtual void setPacketCommand(Common::Network::Packet& packet) const = 0;
            virtual ~BaseAnnounceCommand() = default;

            bool parseCommand(const std::string& providedCommand) override
            {
                std::smatch match;
                if (std::regex_match(providedCommand, match, this->m_pattern))
                {
                    m_message = match[1].str(); 
                    return true;
                }
                return false;
            }

        public:
            virtual Common::Enums::PlayerGrade getRequiredGrade(Main::Persistence::MainScheduler& scheduler) const override
            {
                if (scheduler.immediatePersist(std::source_location::current(), &Main::Persistence::PersistentDatabase::isCommandEventExpired))
                {
                    return m_requiredGrade;
                }
                return Common::Enums::PlayerGrade::GRADE_ES;
            }

            void execute(const std::string& command, std::shared_ptr<Main::Network::Session> session, MN::SessionsManager& sessionsManager, MC::RoomsManager&, 
                MP::MainScheduler&, std::uint32_t,
                Main::MainServer&) override
            {
                if (!parseCommand(command))
                {
                    session->sendMessage("parse error");
                    return;
                }
                if (m_message.size() > 512)
                {
                    session->sendMessage("error: the message must be smaller than 512 characters");
                    return;
                }
                Common::Network::Packet packet;
                packet.setTcpHeader(session->getId(), Common::Enums::NO_ENCRYPTION);
                setPacketCommand(packet);
                packet.setData(reinterpret_cast<std::uint8_t*>(const_cast<char*>(m_message.c_str())), m_message.size());
                sessionsManager.broadcast(packet);
            }
        };

        struct Announce final : public BaseAnnounceCommand
        {
            explicit Announce(const Common::Enums::PlayerGrade requiredGrade)
                : BaseAnnounceCommand(requiredGrade, "/announce <message>")
            {
            }

        protected:
            void setPacketCommand(Common::Network::Packet& packet) const override
            {
                packet.setCommand(402, 0, 0xA, 0);
            }
        };
        REGISTER_CMD(Announce, Common::Enums::PlayerGrade::GRADE_MOD)

        struct Tip final : public BaseAnnounceCommand
        {
            explicit Tip(const Common::Enums::PlayerGrade requiredGrade)
                : BaseAnnounceCommand(requiredGrade, "/tip <message>")
            {
            }

        protected:
            void setPacketCommand(Common::Network::Packet& packet) const override
            {
                packet.setCommand(402, 0, 0xC, 0);
            }
        };
        REGISTER_CMD(Tip, Common::Enums::PlayerGrade::GRADE_MOD)
    }
}

#endif
