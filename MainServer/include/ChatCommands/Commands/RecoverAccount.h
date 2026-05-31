#ifndef RECOVER_ACCOUNT_COMMAND_HEADER
#define RECOVER_ACCOUNT_COMMAND_HEADER

#include "../ICommand.h"
#include "../ChatCommands.h"
#include "Utils/Utils.h"
#include "../../MainServer.h"
#include <source_location>
#include <libbcrypt/include/bcrypt/BCrypt.hpp>

namespace Main
{
	namespace Command
	{
		class RecoverAccount final : public ICommand
		{
		private:
			std::uint32_t m_targetAid{ 0 };
			std::string m_targetEmail{};

			bool parseCommand(const std::string& providedCommand) override
			{
				std::smatch match;
				if (std::regex_match(providedCommand, match, this->m_pattern))
				{
					const std::string& aidStr = match[2].str();
					auto [ptr, ec] = std::from_chars(aidStr.data(), aidStr.data() + aidStr.size(), m_targetAid);

					if (ec != std::errc{} || ptr != aidStr.data() + aidStr.size())
						return false;

					return true;
				}
				return false;
			}


		public:
			explicit RecoverAccount(const Common::Enums::PlayerGrade requiredGrade)
				: ICommand{ requiredGrade, "/recoveraccount <aid>", R"(^(\S+)\s*(\S+)\s*$)" }
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
					session->sendMessage("error: invalid command format. Usage: /recoveraccount <aid>");
					return;
				}

				auto emailOpt = scheduler.immediatePersist(std::source_location::current(),
					&Main::Persistence::PersistentDatabase::getColumnByAid, "Email", m_targetAid);

				if (!emailOpt.has_value())
				{
					session->sendMessage("error: could not find account with the specified aid");
					return;
				}
				m_targetEmail = emailOpt.value();

				const std::string tempPassword = Common::Utils::generateRandomPassword();
				const std::string secret2FA = Common::Utils::generate2FASecret();

				auto decryptedEmail = Common::Utils::decryptEmail(m_targetEmail, Common::Utils::SetupParser::getInstance().getGeneralSetup().emailSecret);
				if (!decryptedEmail.has_value())
				{
					session->sendMessage("error: failed to decrypt email");
					return;
				}

				auto encryptedSecret = Common::Utils::encrypt2FASecret(secret2FA, Common::Utils::SetupParser::getInstance().getGeneralSetup().twoFaSecret);
				if (!encryptedSecret.has_value())
				{
					session->sendMessage("error: failed to encrypt 2FA secret");
					return;
				}

				bool pwSuccess = scheduler.immediatePersist(std::source_location::current(),
					&Main::Persistence::PersistentDatabase::updatePasswordByAid, m_targetAid, BCrypt::generateHash(tempPassword));

				bool secretSuccess = scheduler.immediatePersist(std::source_location::current(),
					&Main::Persistence::PersistentDatabase::updateSecretByAid, m_targetAid, encryptedSecret.value());

				if (!pwSuccess || !secretSuccess)
				{
					session->sendMessage("error: failed to update password or 2FA secret in database");
					return;
				}

				session->sendMessage("success: account password and 2FA secret have been reset");

				std::ostringstream emailBody;
				emailBody << "<html><body>";
				emailBody << "<p>Hello</p>";
				emailBody << "<p>Your account credentials have been reset.</p>";
				emailBody << "<p><b>Temporary password:</b> " << tempPassword << "</p>";

				emailBody << "<p><b>2FA Secret:</b> " << secret2FA << "</p>";
				emailBody << "<p>To enable 2FA, open your authenticator app and manually enter the secret above (Time-based 2FA).</p>";
				emailBody << "<p>You can change your password in-game with the command: /changepw &lt;CurrentPassword&gt; &lt;2FaToken&gt; &lt;NewPassword&gt;</p>";

				emailBody << "<p>If you lose access to your account, contact us at "
					<< " <a href=\"mailto:support@toybattles.net\">support@toybattles.net</a>.</p>";
				emailBody << "<p>Keep this information secure.</p>";
				emailBody << "</body></html>";

				srv.emailDispatcher.sendEmailAsync({ decryptedEmail.value()}, "Your TB Account Credentials Reset", emailBody.str(),
					[session]() {
						session->sendMessage("warning: account reset but failed to send email with new password/2FA info");
					});
			}
		};

		REGISTER_CMD(RecoverAccount, Common::Enums::PlayerGrade::GRADE_DEV)
	};
}

#endif
