#ifndef CAPSULE_WINRATE_TEST_HEADER
#define CAPSULE_WINRATE_TEST_HEADER

#include "ChatCommands/ICommand.h"
#include "ChatCommands/ChatCommands.h"
#include "Handlers/Item/CapsuleSpinHandler.h"
#include "Detail/CdbUtils.h"
#include <charconv>
#include <filesystem>
#include <fstream>
#include <thread>
#include <string>

namespace Main
{
	namespace Command
	{
		class CapsuleWinrate final : public ICommand
		{
		private:
			std::uint32_t m_iterations{};

			bool parseCommand(const std::string& providedCommand) override
			{
				std::smatch match;
				if (std::regex_match(providedCommand, match, m_pattern))
				{
					const std::string& matched = match[1].str();
					const auto result = std::from_chars(matched.data(), matched.data() + matched.size(), m_iterations);
					return result.ec == std::errc() && m_iterations > 0;
				}
				return false;
			}

		public:
			explicit CapsuleWinrate(const Common::Enums::PlayerGrade requiredGrade)
				: ICommand{ requiredGrade, "/capsulewinrate <iterations>", R"(^\S+\s(\d+))" }
			{
			}

			void execute(const std::string& command, std::shared_ptr<Main::Network::Session> session,
				MN::SessionsManager&, MC::RoomsManager&, MP::MainScheduler&, std::uint32_t,
				Main::MainServer&) override
			{
				if (!parseCommand(command))
				{
					session->sendMessage("Usage: /capsulewinrate <iterations>");
					return;
				}

				const std::uint32_t iterations = m_iterations;
				const std::filesystem::path outPath = std::filesystem::current_path() / "CapsuleWinrates.txt";
				session->sendMessage("Capsule winrate test started (" + std::to_string(iterations) + " spins per capsule). Output: " + outPath.string());

				std::thread([iterations, session, outPath]()
				{
					const auto& capsuleEntries = Main::CdbUtils::cdbCapsuleInfos::getInstance().getEntries();

					std::ofstream out(outPath);
					if (!out.is_open())
					{
						session->sendMessage("Error: could not open " + outPath.string() + " for writing.");
						return;
					}

					out << "Total iterations per capsule: " << iterations << "\n\n";

					for (const auto& [unused, capsuleInfo] : capsuleEntries)
					{
						const std::uint32_t gi_type = capsuleInfo.gi_type >= 3 ? 1 : capsuleInfo.gi_type;

						std::uint32_t rareWins = 0;
						for (std::uint32_t i = 0; i < iterations; ++i)
						{
							auto result = Main::Handlers::itemSelectionAlgorithm(capsuleInfo.gi_infoid, capsuleInfo.gi_price, gi_type, false);
							if (result && result->second == 1)
								++rareWins;
						}

						const double winRate = (static_cast<double>(rareWins) / iterations) * 100.0;
						std::string name(capsuleInfo.gi_name, strnlen(capsuleInfo.gi_name, sizeof(capsuleInfo.gi_name)));

						out << name << " | " << winRate << "% | " << iterations << "\n";
					}

					session->sendMessage("Capsule winrate test complete. Results written to " + outPath.string());
				}).detach();
			}
		};

		REGISTER_CMD(CapsuleWinrate, Common::Enums::PlayerGrade::GRADE_TESTER)
	}
}

#endif
