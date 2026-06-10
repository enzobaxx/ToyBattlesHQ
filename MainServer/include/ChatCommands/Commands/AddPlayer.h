#ifndef ADD_PLAYER_COMMAND_HEADER
#define ADD_PLAYER_COMMAND_HEADER

#include "ChatCommands/ICommand.h"
#include "ChatCommands/ChatCommands.h"
#include "Utils/Utils.h"
#include "MainServer.h"
#include <source_location>
#include <libbcrypt/include/bcrypt/BCrypt.hpp>

namespace Main
{
	namespace Command
	{
		class AddPlayer final : public ICommand
		{
		private:
			std::string m_targetUsername{};
			std::string m_targetNickname{};
			std::string m_targetEmail{};

			bool parseCommand(const std::string& providedCommand) override
			{
				std::smatch match;
				if (std::regex_match(providedCommand, match, this->m_pattern))
				{
					m_targetUsername = match[2].str();
					m_targetNickname = match[3].str();
					m_targetEmail = match[4].str();
					return true;
				}
				return false;
			}

		public:
			explicit AddPlayer(const Common::Enums::PlayerGrade requiredGrade)
				: ICommand{ requiredGrade, "/addplayer <username> <nickname> <email>",R"(^(\S+)\s*(\S+)\s*(\S+)\s*(\S+@\S+\.\S+)\s*$)" }
			{
			}

			void execute(const std::string& command, std::shared_ptr<Main::Network::Session> session,
				MN::SessionsManager& sessionsManager, MC::RoomsManager&, MP::MainScheduler& scheduler, std::uint32_t,
				Main::MainServer& srv) override
			{
				if (!Common::Utils::SetupParser::getInstance().getAuthSetup().enhancedSecurity)
				{
					session->sendMessage("error: this command is disabled because EnhancedSecurity is off");
					return;
				}

				if (!parseCommand(command))
				{
					session->sendMessage("error: invalid command format. Usage: /addplayer <username> <nickname> <email>");
					return;
				}

				auto usernameExists = scheduler.immediatePersist(std::source_location::current(), &Main::Persistence::PersistentDatabase::playerExistsByUsername,
					m_targetUsername);
				if (usernameExists)
				{
					session->sendMessage("error: a player with this username already exists");
					return;
				}

				auto nicknameExists = scheduler.immediatePersist(std::source_location::current(), &Main::Persistence::PersistentDatabase::playerExistsByNickname,
					m_targetNickname);
				if (nicknameExists)
				{
					session->sendMessage("error: a player with this nickname already exists");
					return;
				}

				const std::string tempPassword = Common::Utils::generateRandomPassword();
				const std::string secret2FA = Common::Utils::generate2FASecret();

				auto encryptedEmail = Common::Utils::encryptEmail(m_targetEmail, Common::Utils::SetupParser::getInstance().getGeneralSetup().emailSecret);
				if (!encryptedEmail.has_value())
				{
					session->sendMessage("error: failed to encrypt email");
					return;
				}

				auto encryptedSecret = Common::Utils::encrypt2FASecret(secret2FA, Common::Utils::SetupParser::getInstance().getGeneralSetup().twoFaSecret);
				if (!encryptedSecret.has_value())
				{
					session->sendMessage("error: failed to encrypt 2FA secret");
					return;
				}

				bool success = scheduler.immediatePersist(std::source_location::current(), &Main::Persistence::PersistentDatabase::addPlayer,
					m_targetUsername, BCrypt::generateHash(tempPassword), m_targetNickname, encryptedEmail.value(), encryptedSecret.value());
				if (!success)
				{
					session->sendMessage("error: failed to add player to database");
					return;
				}

				session->sendMessage("success: player " + m_targetUsername + " (nickname: " + m_targetNickname + ") added successfully");

				const bool enhancedSecurity =
					Common::Utils::SetupParser::getInstance().getAuthSetup().enhancedSecurity;

				std::ostringstream emailBody;
				emailBody << "<html><body>";
				emailBody << "<p>Hello " << m_targetUsername << ",</p>";
				emailBody << "<p>Your account has been created successfully!</p>";
				emailBody << "<p><b>Temporary password:</b> " << tempPassword << "</p>";

				if (enhancedSecurity)
				{
					emailBody << "<p><b>2FA Secret:</b> " << secret2FA << "</p>";
					emailBody << "<p>To enable 2FA, open your authenticator app and manually enter the secret above (Time-based 2FA).</p>";
					emailBody << "<p>YOU NEED 2FA TO CHANGE YOUR PASSWORD OR YOUR USERNAME. You can change your password in-game with the command: /changepw "
						"&lt;CurrentPassword&gt; &lt;2FaToken&gt; &lt;NewPassword&gt;</p>";
				}

				emailBody << "<p>Keep this information secure and eventually delete this email once you have saved it.</p>";
				emailBody << "</body></html>";

				srv.emailDispatcher.sendEmailAsync({ m_targetEmail }, "Your New TB Account", emailBody.str(),
					[session]() {
						session->sendMessage("warning: account created but failed to send email with password/2FA info");
					});
			}
		};

		REGISTER_CMD(AddPlayer, Common::Enums::PlayerGrade::GRADE_MOD)
	};
}

#endif