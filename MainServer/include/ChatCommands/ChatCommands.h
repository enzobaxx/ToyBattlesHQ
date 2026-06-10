#ifndef MAIN_CHAT_COMMANDS_H
#define MAIN_CHAT_COMMANDS_H

#include <unordered_map>
#include <string>
#include <memory>
#include "Network/Sessions/MainSession.h"
#include "Network/Packet.h"
#include "ChatCommands/ICommand.h"
#include <memory>
#include "asio.hpp"


namespace Main
{
	template <typename T>
	concept IsPlayerGrade = std::same_as<T, Common::Enums::PlayerGrade>;

	class MainServer;
	namespace Classes { class RoomsManager; }
	namespace Network { class SessionsManager; }

	namespace Command
	{
		class ChatCommands
		{
		private:
			static inline std::unordered_map<std::string, std::unique_ptr<ICommand>> m_commands;

		public:
			static inline Main::Persistence::MainScheduler* m_scheduler;

			static void addCommand(std::string name, std::unique_ptr<ICommand> command);

			static bool executeCommand(const std::string& commandName, const std::string& wholeCommand, std::shared_ptr<Main::Network::Session> session, MN::SessionsManager& sessionsManager,
				MC::RoomsManager& roomsManager, MP::MainScheduler& scheduler, std::uint32_t roomNumber, Main::MainServer&);

			static void showUsages(std::shared_ptr<Main::Network::Session>, Common::Network::Packet& response, Common::Enums::PlayerGrade playerGrade);
		};

#define REGISTER_CMD(ClassName, Grade)                                     \
    static_assert(IsPlayerGrade<decltype(Grade)>,                          \
                  "Grade must be of type Common::Enums::PlayerGrade");     \
    struct ClassName##_Reg												   \
	{																	   \
        ClassName##_Reg()												   \
		{																   \
            ChatCommands::addCommand(                                      \
                #ClassName,												   \
                std::make_unique<ClassName>(Grade));                       \
        }                                                                  \
    } ClassName##_reg;

	} // namespace Command
} // namespace Main

#endif