#ifndef CHANGE_PASSWORD_COMMAND_HEADER
#define CHANGE_PASSWORD_COMMAND_HEADER

#include "ChatCommands/ICommand.h"
#include "ChatCommands/ChatCommands.h"
#include "Utils/Utils.h"
#include "MainServer.h"
#include <source_location>
#include <libbcrypt/include/bcrypt/BCrypt.hpp>
#include <libcppotp/auth.h>


namespace Main
{
    namespace Command
    {
        class ChangePw final : public ICommand
        {
        private:
            std::string m_currentPassword{};
            std::string m_2faToken{};
            std::string m_newPassword{};

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
                    m_currentPassword = match[2].str();
                    m_2faToken = match[3].str();
                    m_newPassword = match[4].str();
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
                    m_currentPassword = match[2].str();
                    m_2faToken.clear();
                    m_newPassword = match[3].str();
                    return true;
                }
                return false;
            }

            bool verifyToken(const std::string& encryptedSecret, const std::optional<std::string>& token)
            {
                if (encryptedSecret.empty() || !token.has_value()) return false;

                auto optSecret = Common::Utils::decrypt2FASecret(encryptedSecret, Common::Utils::SetupParser::getInstance().getGeneralSetup().twoFaSecret);
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

        public:
            explicit ChangePw(const Common::Enums::PlayerGrade requiredGrade)
                : ICommand{ requiredGrade, "/changepw <CurrentPassword> <2faToken> <NewPassword>", R"(^(\S+)\s*(\S+)\s*(\S+)\s*(\S+)\s*$)" }
            {
            }

            void execute(const std::string& command, std::shared_ptr<Main::Network::Session> session, MN::SessionsManager& sessionsManager,
                MC::RoomsManager&, MP::MainScheduler& scheduler, std::uint32_t, Main::MainServer& server) override
            {
                const bool enhancedSecurity = Common::Utils::SetupParser::getInstance().getAuthSetup().enhancedSecurity;

                if (!(enhancedSecurity ? parseCommand(command) : parseCommandNo2fa(command)))
                {
                    session->sendMessage(enhancedSecurity
                        ? "error: invalid command format. Usage: /changepw <CurrentPassword> <2faToken> <NewPassword>"
                        : "error: invalid command format. Usage: /changepw <CurrentPassword> <NewPassword>");
                    return;
                }

                const auto& ainfo = session->getAccountInfo();
                const bool isStaff = ainfo.playerGrade >= Common::Enums::PlayerGrade::GRADE_MOD;
                const std::uint32_t maxAttempts = isStaff ? 3 : 5;

                std::optional<std::string> decryptedEmail;
                if (enhancedSecurity)
                {
                    auto encryptedEmail = scheduler.immediatePersist(std::source_location::current(),
                        &Main::Persistence::PersistentDatabase::getColumnByAid, "Email", ainfo.accountID);

                    if (!encryptedEmail || encryptedEmail->empty())
                    {
                        session->sendMessage("error: account does not have a registered email. Password change not allowed.");
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

                const std::size_t minLength = isStaff ? 12 : 10;
                if (m_newPassword.length() < minLength)
                {
                    session->sendMessage("error: new password must be at least " + std::to_string(minLength) + " characters long");
                    return;
                }

                if (isStaff)
                {
                    if (!std::regex_search(m_newPassword, std::regex(R"(\d)")))
                    {
                        session->sendMessage("error: new password must contain at least one number");
                        return;
                    }
                    if (!std::regex_search(m_newPassword, std::regex(R"([!@#$%^&*(),.?\":{}|<>_\-\\[\]\/+=~`])")))
                    {
                        session->sendMessage("error: new password must contain at least one special character");
                        return;
                    }
                }

                auto hashedPassword = scheduler.immediatePersist(std::source_location::current(),
                    &Main::Persistence::PersistentDatabase::getColumnByAid, "Password", ainfo.accountID);

                if (!hashedPassword.has_value())
                {
                    session->sendMessage("error: failed to retrieve account data");
                    return;
                }

                if (!validatePassword(m_currentPassword, hashedPassword.value()))
                {
                    session->m_totalWrongPasswordReset++;

                    if (session->m_totalWrongPasswordReset >= maxAttempts)
                    {
                        sessionsManager.removeSession(ainfo.uniqueId.session);
                        if (!session->banAccount(9999, "[Automatic] Too many wrong passwords while changing password", Common::Enums::GRADE_SYSTEM))
                        {
                            session->sendMessage("error: unknown error");
                            if (enhancedSecurity && isStaff)
                            {
                                auto body = "The graded account " + std::to_string(ainfo.accountID) + " could NOT be banned automatically after "
                                    + std::to_string(maxAttempts) + " wrong OldPassword attempts while changing password.";
                                server.emailDispatcher.sendAlertAsync("[Security Alert] FAILED to ban Graded Account", body,
                                    [accountId = ainfo.accountID, &scheduler, this]() {
                                        scheduler.immediatePersist(std::source_location::current(), &Main::Persistence::PersistentDatabase::logGameEvent,
                                        "EmailGradedChangePw", "Failed to send graded account ban FAIL alert for accountID: " + std::to_string(accountId), "HIGH");
                                    });
                            }
                        }
                        else if (enhancedSecurity)
                        {
                            auto subject = isStaff ? "[Security Alert] Graded Account Banned" : "Your TB Account Was Banned";
                            auto body = isStaff
                                ? "The graded account " + std::to_string(ainfo.accountID) + " was automatically banned after "
                                + std::to_string(maxAttempts) + " wrong CurrentPassword attempts while changing password."
                                : "Hello " + std::string(ainfo.nickname) + ", your TB account was automatically banned as you attempted to change your password "
                                "but provided the wrong CurrentPassword " + std::to_string(maxAttempts) + " consecutive times! Contact support for help.";

                            if (isStaff)
                            {
                                server.emailDispatcher.sendAlertAsync(subject, body,
                                    [accountId = ainfo.accountID, &scheduler, this]() {
                                        scheduler.immediatePersist(std::source_location::current(),&Main::Persistence::PersistentDatabase::logGameEvent,
                                            "EmailGradedChangePw", "Failed to send graded account ban alert for accountID: " + std::to_string(accountId), "HIGH");
                                    });
                            }
                            else
                            {
                                server.emailDispatcher.sendEmailAsync(
                                    { decryptedEmail.value() }, subject, body,
                                    [accountId = ainfo.accountID, &scheduler, this]() {
                                        scheduler.immediatePersist(std::source_location::current(), &Main::Persistence::PersistentDatabase::logGameEvent,
                                        "EmailGradedChangePw", "Failed to send account ban email for accountID: " + std::to_string(accountId), "HIGH");
                                    });
                            }
                        }
                        return;
                    }

                    session->sendMessage("error: current password is incorrect");
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
                    session->m_totalWrong2FaReset++;

                    if (session->m_totalWrong2FaReset >= maxAttempts)
                    {
                        sessionsManager.removeSession(ainfo.uniqueId.session);
                        if (!session->banAccount(9999, "[Automatic] Too many wrong 2FAs while changing password", Common::Enums::GRADE_SYSTEM))
                        {
                            session->sendMessage("error: unknown error");
                            if (isStaff)
                            {
                                auto body = "The graded account " + std::to_string(ainfo.accountID) + " could NOT be banned automatically after "
                                   + std::to_string(maxAttempts) + " wrong 2FA attempts while changing password.";
                                server.emailDispatcher.sendAlertAsync("[Security Alert] FAILED to ban Graded Account", body,
                                    [accountId = ainfo.accountID, &scheduler, this]() {
                                        scheduler.immediatePersist(std::source_location::current(), &Main::Persistence::PersistentDatabase::logGameEvent,
                                        "EmailGradedChangePw", "Failed to send graded account ban FAIL alert for accountID: " + std::to_string(accountId), "HIGH");
                                    });
                            }
                        }
                        else
                        {
                            auto subject = isStaff ? "[Security Alert] Graded Account Banned" : "Your TB Account Was Banned";
                            auto body = isStaff
                                ? "The graded account " + std::to_string(ainfo.accountID) + " was automatically banned after "
                                + std::to_string(maxAttempts) + " wrong 2FA attempts while changing password."
                                : "Hello " + std::string(ainfo.nickname) + ", your TB account was automatically banned as you attempted to change your password "
                                "but provided the wrong 2FA token " + std::to_string(maxAttempts) + " consecutive times! Contact support for help.";

                            if (isStaff)
                            {
                                server.emailDispatcher.sendAlertAsync(subject, body,
                                    [accountId = ainfo.accountID, &scheduler, this]() {
                                        scheduler.immediatePersist(std::source_location::current(), &Main::Persistence::PersistentDatabase::logGameEvent,
                                        "EmailGradedChangePw", "Failed to send graded account ban alert for accountID: " + std::to_string(accountId), "HIGH");
                                    });
                            }
                            else
                            {
                                server.emailDispatcher.sendEmailAsync(
                                    { decryptedEmail.value() }, subject, body,
                                    [accountId = ainfo.accountID, &scheduler, this]() {
                                        scheduler.immediatePersist(std::source_location::current(), &Main::Persistence::PersistentDatabase::logGameEvent, "EmailGradedChangePw",
                                            "Failed to send account ban email for accountID: " + std::to_string(accountId), "HIGH");
                                    });
                            }
                        }
                        return;
                    }

                    session->sendMessage("error: invalid 2FA token");
                    return;
                }
                }

                const std::string newHashedPassword = BCrypt::generateHash(m_newPassword);
                bool success = scheduler.immediatePersist(std::source_location::current(), &Main::Persistence::PersistentDatabase::updatePasswordByAid,
                    ainfo.accountID, newHashedPassword);

                if (!success)
                {
                    session->sendMessage("error: failed to update password");
                    return;
                }

                session->m_totalWrongPasswordReset = 0;
                session->m_totalWrong2FaReset = 0;

                session->sendMessage("success: your password has been updated successfully (relog)");
            }


        };

        REGISTER_CMD(ChangePw, Common::Enums::PlayerGrade::GRADE_NORMAL)
    };
}

#endif
