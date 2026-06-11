#ifndef CHANGE_USERNAME_COMMAND_HEADER
#define CHANGE_USERNAME_COMMAND_HEADER

#include "ChatCommands/ICommand.h"
#include "ChatCommands/ChatCommands.h"
#include "Utils/Utils.h"
#include "MainServer.h"
#include <source_location>
#include <regex>
#include <libbcrypt/include/bcrypt/BCrypt.hpp>
#include <libcppotp/auth.h>

namespace Main
{
    namespace Command
    {
        class ChangeUsername final : public ICommand
        {
        private:
            std::string m_password{};
            std::string m_2faToken{};
            std::string m_newUsername{};

            bool validatePassword(const std::string& plain, std::string hash)
            {
                if (BCrypt::validatePassword(plain, hash))
                    return true;

                if (hash.substr(0, 4) == "$2b$")
                {
                    std::string hash_2a = hash;
                    hash_2a[2] = 'a';
                    return BCrypt::validatePassword(plain, hash_2a);
                }

                return false;
            }

            bool parseCommand(const std::string& providedCommand) override
            {
                std::smatch match;
                if (std::regex_match(providedCommand, match, this->m_pattern))
                {
                    m_password = match[2].str();
                    m_2faToken = match[3].str();
                    m_newUsername = match[4].str();
                    return true;
                }
                return false;
            }

            bool parseCommandNo2fa(const std::string& providedCommand)
            {
                static const std::regex pattern(R"(^(\S+)\s*(\S+)\s*(\S+)\s*$)");
                std::smatch match;
                if (std::regex_match(providedCommand, match, pattern))
                {
                    m_password = match[2].str();
                    m_2faToken.clear();
                    m_newUsername = match[3].str();
                    return true;
                }
                return false;
            }

            bool verifyToken(const std::string& encryptedSecret, const std::optional<std::string>& token)
            {
                if (encryptedSecret.empty() || !token.has_value()) return false;

                auto optSecret = Common::Utils::decrypt2FASecret(encryptedSecret,
                    Common::Utils::SetupParser::getInstance().getGeneralSetup().twoFaSecret);
                if (!optSecret) return false;

                const int t_interval = 30;
                std::time_t now = std::time(nullptr);

                for (int i = -1; i <= 1; ++i)
                {
                    auto expectedToken = auth::generateToken(optSecret.value(), now + i * t_interval, t_interval);
                    std::ostringstream oss;
                    oss << std::setw(6) << std::setfill('0') << expectedToken;

                    if (oss.str() == token.value()) return true;
                }

                return false;
            }

            bool validateUsername(const std::string& username)
            {
                if (username.length() < 4 || username.length() > 20)
                    return false;

                if (!std::isalpha(username[0]))
                    return false;

                for (char c : username)
                {
                    if (!std::isalnum(c) && c != '_')
                        return false;
                }

                if (username.find("__") != std::string::npos)
                    return false;

                bool hasLetter = false;
                for (char c : username)
                {
                    if (std::isalpha(c))
                    {
                        hasLetter = true;
                        break;
                    }
                }

                return hasLetter;
            }

        public:
            explicit ChangeUsername(const Common::Enums::PlayerGrade requiredGrade)
                : ICommand{ requiredGrade, "/changeusername <Password> <2faToken> <NewUsername>",
                    R"(^(\S+)\s*(\S+)\s*(\S+)\s*(\S+)\s*$)" }
            {
            }

            void execute(const std::string& command, std::shared_ptr<Main::Network::Session> session,
                MN::SessionsManager& sessionsManager, MC::RoomsManager&, MP::MainScheduler& scheduler,
                std::uint32_t, Main::MainServer& server) override
            {
                const auto& ainfo = session->getAccountInfo();
                if (ainfo.playerGrade >= Common::Enums::PlayerGrade::GRADE_MOD)
                {
                    session->sendMessage("error: staff accounts cannot change username. Contact admins.");
                    return;
                }

                const bool enhancedSecurity = Common::Utils::SetupParser::getInstance().getAuthSetup().enhancedSecurity;

                if (!(enhancedSecurity ? parseCommand(command) : parseCommandNo2fa(command)))
                {
                    session->sendMessage(enhancedSecurity
                        ? "error: invalid command format. Usage: /changeusername <Password> <2faToken> <NewUsername>"
                        : "error: invalid command format. Usage: /changeusername <Password> <NewUsername>");
                    return;
                }

                if (!validateUsername(m_newUsername))
                {
                    session->sendMessage("error: invalid username. Must be 4-20 characters, start with a letter, and contain only letters, numbers, and underscores (no consecutive underscores)");
                    return;
                }

                std::optional<std::string> decryptedEmail;
                if (enhancedSecurity)
                {
                    auto encryptedEmail = scheduler.immediatePersist(std::source_location::current(),
                        &Main::Persistence::PersistentDatabase::getColumnByAid, "Email", ainfo.accountID);

                    if (!encryptedEmail || encryptedEmail->empty())
                    {
                        session->sendMessage("error: account does not have a registered email. Username change not allowed.");
                        return;
                    }

                    decryptedEmail = Common::Utils::decryptEmail(encryptedEmail.value(),
                        Common::Utils::SetupParser::getInstance().getGeneralSetup().emailSecret);

                    if (!decryptedEmail)
                    {
                        session->sendMessage("error: failed to fetch email from database.");
                        return;
                    }
                }

                bool usernameExists = scheduler.immediatePersist(std::source_location::current(),
                    &Main::Persistence::PersistentDatabase::checkUsernameExists, m_newUsername);

                if (usernameExists)
                {
                    session->sendMessage("error: username already taken");
                    return;
                }

                auto hashedPassword = scheduler.immediatePersist(std::source_location::current(),
                    &Main::Persistence::PersistentDatabase::getColumnByAid, "Password", ainfo.accountID);

                if (!hashedPassword.has_value())
                {
                    session->sendMessage("error: failed to retrieve account data");
                    return;
                }

                const std::uint32_t maxAttempts = 5;

                if (!validatePassword(m_password, hashedPassword.value()))
                {
                    session->getPlayer().totalWrongUsernameChange++;

                    if (session->getPlayer().totalWrongUsernameChange >= maxAttempts)
                    {
                        sessionsManager.removeSession(ainfo.uniqueId.session);
                        if (!session->banAccount(9999, "[Automatic] Too many wrong passwords while changing username",
                            Common::Enums::GRADE_SYSTEM))
                        {
                            session->sendMessage("error: unknown error");
                        }
                        else if (enhancedSecurity)
                        {
                            auto subject = "Your TB Account Was Banned";
                            auto body = "Hello " + std::string(ainfo.nickname) +
                                ", your TB account was automatically banned as you attempted to change your username "
                                "but provided the wrong password " + std::to_string(maxAttempts) +
                                " consecutive times! Contact support for help.";

                            server.emailDispatcher.sendEmailAsync(
                                { decryptedEmail.value() }, subject, body,
                                [accountId = ainfo.accountID, &scheduler, this]() {
                                    scheduler.immediatePersist(std::source_location::current(),
                                    &Main::Persistence::PersistentDatabase::logGameEvent,
                                    "EmailUsernameChange",
                                    "Failed to send account ban email for accountID: " + std::to_string(accountId),
                                    "HIGH");
                                });
                        }
                        return;
                    }

                    session->sendMessage("error: password is incorrect");
                    return;
                }

                if (enhancedSecurity)
                {
                auto encryptedSecret = scheduler.immediatePersist(std::source_location::current(),
                    &Main::Persistence::PersistentDatabase::getColumnByAid, "Secret", ainfo.accountID);

                if (!encryptedSecret.has_value())
                {
                    session->sendMessage("error: failed to retrieve 2FA data");
                    return;
                }

                if (!verifyToken(encryptedSecret.value(), m_2faToken))
                {
                    session->getPlayer().totalWrong2FaUsernameChange++;

                    if (session->getPlayer().totalWrong2FaUsernameChange >= maxAttempts)
                    {
                        sessionsManager.removeSession(ainfo.uniqueId.session);
                        if (!session->banAccount(9999, "[Automatic] Too many wrong 2FAs while changing username",
                            Common::Enums::GRADE_SYSTEM))
                        {
                            session->sendMessage("error: unknown error");
                        }
                        else
                        {
                            auto subject = "Your TB Account Was Banned";
                            auto body = "Hello " + std::string(ainfo.nickname) +
                                ", your TB account was automatically banned as you attempted to change your username "
                                "but provided the wrong 2FA token " + std::to_string(maxAttempts) +
                                " consecutive times! Contact support for help.";

                            server.emailDispatcher.sendEmailAsync(
                                { decryptedEmail.value() }, subject, body,
                                [accountId = ainfo.accountID, &scheduler, this]() {
                                    scheduler.immediatePersist(std::source_location::current(),
                                    &Main::Persistence::PersistentDatabase::logGameEvent,
                                    "EmailUsernameChange",
                                    "Failed to send account ban email for accountID: " + std::to_string(accountId),
                                    "HIGH");
                                });
                        }
                        return;
                    }

                    session->sendMessage("error: invalid 2FA token");
                    return;
                }
                }

                bool success = scheduler.immediatePersist(std::source_location::current(),
                    &Main::Persistence::PersistentDatabase::updateUsernameByAid,
                    ainfo.accountID, m_newUsername, ainfo.nickname);

                if (!success)
                {
                    session->sendMessage("error: failed to update username");
                    return;
                }

                session->getPlayer().totalWrongUsernameChange = 0;
                session->getPlayer().totalWrong2FaUsernameChange = 0;

                if (enhancedSecurity)
                {
                    auto subject = "Your ToyBattles Username Has Been Changed";
                    auto body = "Hello " + m_newUsername + ",\n\n"
                        "Your username has been successfully changed " +
                        "' to '" + m_newUsername + "'.\n\n"
                        "If you did not make this change, please contact support immediately.\n\n"
                        "Regards,\nToyBattles Team";

                    server.emailDispatcher.sendEmailAsync(
                        { decryptedEmail.value() }, subject, body,
                        [accountId = ainfo.accountID, &scheduler, this]() {
                            scheduler.immediatePersist(std::source_location::current(),
                            &Main::Persistence::PersistentDatabase::logGameEvent,
                            "EmailUsernameChange",
                            "Failed to send username change confirmation email for accountID: " +
                            std::to_string(accountId), "MEDIUM");
                        });
                }

                session->sendMessage("success: your username has been changed to '" + m_newUsername + "'. You will need to relog with your new username.");
            }
        };

        REGISTER_CMD(ChangeUsername, Common::Enums::PlayerGrade::GRADE_NORMAL)
    };
}

#endif