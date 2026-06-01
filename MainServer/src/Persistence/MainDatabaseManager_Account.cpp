#include <memory>
#include <string>

#include "../../include/Structures/AccountInfo/MainAccountInfo.h"
#include "../../include/Structures/Item/MainItem.h"
#include "../../include/Structures/Item/MainBoughtItem.h"
#include "../../include/Structures/Item/MainEquippedItem.h"
#include "../../include/Structures/MainEventsList.h"
#include "../../include/Structures/PlayerLists/Friend.h"
#include "../../include/Structures/PlayerLists/BlockedPlayer.h"
#include "../../include/Structures/Mailbox.h"
#include "../../include/Structures/AccountInfo/MuteInfo.h"
#include "../../include/Persistence/TransactionGuard.h"
#include "../../include/Persistence/MainDatabaseManager.h"
#include "../../include/MainEnums.h"
#include "Utils/Constants.h"
#include <mariadb/conncpp.hpp>
#include <mariadb/conncpp/Driver.hpp>
#include <mariadb/conncpp/Connection.hpp>
#include "Utils/SetupParser.h"
#include <cstring> 
#include <Utils.h>
#include <Macros.h>
#include <regex>

namespace Main
{
    namespace Persistence
    {


        void PersistentDatabase::updatePlayerCurrencyByType(std::uint32_t accountID, std::uint32_t newAmount, Main::Enums::ItemCurrencyType currencyType)
        {
            try
            {
                std::string sql;
                if (currencyType == Main::Enums::ITEM_MP) sql = "UPDATE Users SET MicroPoints = ? WHERE AccountID = ?";
                else if (currencyType == Main::Enums::ITEM_RT) sql = "UPDATE Users SET RockTotens = ? WHERE AccountID = ?";
                else if (currencyType == Main::Enums::ITEM_COUPON) sql = "UPDATE Users SET Coupons = ? WHERE AccountID = ?";
                else if (currencyType == Main::Enums::ITEM_COIN) sql = "UPDATE Users SET Coins = ? WHERE AccountID = ?";

                std::unique_ptr<sql::PreparedStatement> stmt(m_con->prepareStatement(sql));
                stmt->setUInt(1, newAmount);
                stmt->setUInt(2, accountID);

                stmt->executeUpdate();
            }
            catch (const sql::SQLException& e)
            {
                ::Utils::Logger::log("MariaDB exception: " + std::string(e.what()), Utils::LogType::Error, "PersistentDatabase::updatePlayerCurrencyByType");
                return;
            }
        }

        void PersistentDatabase::addPlayerAchievement(std::uint32_t accountID, std::uint32_t achievementIndex)
        {
            try
            {
                std::string sql = "INSERT INTO UserAchievements (AccountID, AchievementIndex) VALUES (?, ?)";

                std::unique_ptr<sql::PreparedStatement> stmt(m_con->prepareStatement(sql));
                stmt->setUInt(1, accountID);
                stmt->setUInt(2, achievementIndex);

                stmt->executeUpdate();
            }
            catch (const sql::SQLException& e)
            {
                ::Utils::Logger::log("MariaDB exception: " + std::string(e.what()), Utils::LogType::Error, "PersistentDatabase::addPlayerAchievement");
                return;
            }
        }

        std::optional<std::string> PersistentDatabase::getColumnByAid(const std::string& columnName, std::uint32_t accountID)
        {
            try
            {
                std::string sql = "SELECT " + columnName + " FROM Users WHERE AccountID = ? LIMIT 1";

                std::unique_ptr<sql::PreparedStatement> stmt(m_con->prepareStatement(sql));
                stmt->setUInt(1, accountID);

                std::unique_ptr<sql::ResultSet> res(stmt->executeQuery());

                if (res->next())
                {
                    return res->getString(columnName).c_str();
                }

                return std::nullopt;
            }
            catch (const sql::SQLException& e)
            {
                ::Utils::Logger::log("MariaDB exception: " + std::string(e.what()),Utils::LogType::Error,"PersistentDatabase::getColumnByAid");
                return std::nullopt;
            }
        }

        bool PersistentDatabase::updatePasswordByAid(std::uint32_t accountID, const std::string& newHashedPassword)
        {
            try
            {
                std::string sql = "UPDATE Users SET Password = ? WHERE AccountID = ?";

                std::unique_ptr<sql::PreparedStatement> stmt(m_con->prepareStatement(sql));
                stmt->setString(1, newHashedPassword);
                stmt->setUInt(2, accountID);

                const int affectedRows = stmt->executeUpdate();
                return affectedRows > 0;
            }
            catch (const sql::SQLException& e)
            {
                ::Utils::Logger::log("MariaDB exception: " + std::string(e.what()),Utils::LogType::Error,"PersistentDatabase::updatePasswordByAid");
                return false;;
            }
        }

        bool PersistentDatabase::updateSecretByAid(std::uint32_t accountID, const std::string& newEncryptedSecret)
        {
            try
            {
                std::string sql = "UPDATE Users SET Secret = ? WHERE AccountID = ?";

                std::unique_ptr<sql::PreparedStatement> stmt(m_con->prepareStatement(sql));
                stmt->setString(1, newEncryptedSecret);
                stmt->setUInt(2, accountID);

                const int affectedRows = stmt->executeUpdate();
                return affectedRows > 0;
            }
            catch (const sql::SQLException& e)
            {
                ::Utils::Logger::log("MariaDB exception: " + std::string(e.what()), Utils::LogType::Error, "PersistentDatabase::updateSecretByAid");
                return false;
            }
        }

        std::vector<std::pair<std::uint32_t, std::uint32_t>> PersistentDatabase::getPlayerAchievements(std::uint32_t accountID)
        {
            std::vector<std::pair<std::uint32_t, std::uint32_t>> achievements;

            try
            {
                std::string sql = "SELECT AchievementIndex, AchievementType FROM UserAchievements WHERE AccountID = ?";

                std::unique_ptr<sql::PreparedStatement> stmt(m_con->prepareStatement(sql));
                stmt->setUInt(1, accountID);

                std::unique_ptr<sql::ResultSet> res(stmt->executeQuery());

                while (res->next())
                {
                    std::uint32_t achievementIndex = res->getUInt("AchievementIndex");
                    std::uint32_t achievementType = res->getUInt("AchievementType");
                    achievements.emplace_back(achievementIndex, achievementType);
                }
            }
            catch (const sql::SQLException& e)
            {
                ::Utils::Logger::log("MariaDB exception: " + std::string(e.what()), Utils::LogType::Error, "PersistentDatabase::getPlayerAchievements");
                return achievements;
            }

            return achievements;
        }

        void PersistentDatabase::updateLatestSelectedCharacter(std::uint32_t accountID, std::uint16_t characterId)
        {
            try
            {
                const std::string updateCharacterQuery = "UPDATE Users SET LastCharacterUsed = ? WHERE AccountID = ?";
                std::unique_ptr<sql::PreparedStatement> stmt(m_con->prepareStatement(updateCharacterQuery));
                stmt->setUInt(1, characterId);
                stmt->setUInt(2, accountID);
                stmt->executeUpdate();
            }
            catch (const sql::SQLException& e)
            {
                ::Utils::Logger::log("MariaDB exception: " + std::string(e.what()), Utils::LogType::Error, "PersistentDatabase::updateLatestSelectedCharacter");
                return;
            }
        }

        bool PersistentDatabase::updateUsernameByAid(std::uint32_t accountId,
            const std::string& newUsername, const std::string& oldUsername)
        {
            try
            {
                std::string queryStr = "UPDATE Users SET Username = ? WHERE AccountID = ?";

                std::unique_ptr<sql::PreparedStatement> stmt(m_con->prepareStatement(queryStr));
                stmt->setString(1, newUsername);
                stmt->setUInt(2, accountId);

                return stmt->executeUpdate() > 0;
            }
            catch (const sql::SQLException& e)
            {
                ::Utils::Logger::log("MariaDB exception: " + std::string(e.what()),
                    Utils::LogType::Error, "PersistentDatabase::updateUsernameByAid");
                return false;
            }
        }

        bool PersistentDatabase::checkUsernameExists(const std::string& username)
        {
            try
            {
                std::string queryStr = "SELECT COUNT(*) FROM Users WHERE LOWER(Username) = LOWER(?)";

                std::unique_ptr<sql::PreparedStatement> stmt(m_con->prepareStatement(queryStr));
                stmt->setString(1, username);

                std::unique_ptr<sql::ResultSet> res(stmt->executeQuery());

                if (res->next())
                {
                    return res->getInt(1) > 0;
                }

                return false;
            }
            catch (const sql::SQLException& e)
            {
                ::Utils::Logger::log("MariaDB exception: " + std::string(e.what()), Utils::LogType::Error, "PersistentDatabase::checkUsernameExists");

                return true;
            }
        }

        std::optional<Main::Structures::AccountInfo> PersistentDatabase::getPlayerInfo(std::uint32_t playerID)
        {
            Main::Structures::AccountInfo playerInfoStructure{};
            try
            {
                std::string queryStr = "SELECT Users.*, Clans.Clanname as Clan_Clanname, Clans.ClanFrontIcon as Clan_FrontIcon, Clans.ClanBackIcon as Clan_BackIcon "
                    "FROM Users LEFT JOIN Clans ON Users.ClanID = Clans.ClanId WHERE AccountID = ?";

                std::unique_ptr<sql::PreparedStatement> stmt(m_con->prepareStatement(queryStr));
                stmt->setUInt(1, playerID);
                std::unique_ptr<sql::ResultSet> res(stmt->executeQuery());

                if (res->next())
                {
                    playerInfoStructure.accountID = playerID;
                    std::memcpy(playerInfoStructure.nickname, res->getString("Nickname"), Common::Constants::maxNicknameSize);
                    std::memcpy(playerInfoStructure.clanName, res->getString("Clan_Clanname"), Common::Constants::maxNicknameSize);
                    playerInfoStructure.accountKey = static_cast<std::uint32_t>(res->getUInt("AccountKey"));
                    playerInfoStructure.totalKills = static_cast<std::uint32_t>(res->getInt("Kills"));
                    playerInfoStructure.deaths = static_cast<std::uint32_t>(res->getInt("Deaths"));
                    playerInfoStructure.assists = static_cast<std::uint32_t>(res->getInt("Assists"));
                    playerInfoStructure.wins = static_cast<std::uint32_t>(res->getInt("Wins"));
                    playerInfoStructure.losses = static_cast<std::uint32_t>(res->getInt("Loses"));
                    playerInfoStructure.draws = static_cast<std::uint32_t>(res->getInt("Draws"));
                    playerInfoStructure.meleeKills = static_cast<std::uint32_t>(res->getInt("MeleeKills"));
                    playerInfoStructure.rifleKills = static_cast<std::uint32_t>(res->getInt("RifleKills"));
                    playerInfoStructure.shotgunKills = static_cast<std::uint32_t>(res->getInt("ShotgunKills"));
                    playerInfoStructure.sniperKills = static_cast<std::uint32_t>(res->getInt("SniperKills"));
                    playerInfoStructure.microgunKills = static_cast<std::uint32_t>(res->getInt("GatlingKills"));
                    playerInfoStructure.bazookaKills = static_cast<std::uint32_t>(res->getInt("BazookaKills"));
                    playerInfoStructure.grenadeKills = static_cast<std::uint32_t>(res->getInt("GrenadeKills"));
                    playerInfoStructure.killstreak = static_cast<std::uint64_t>(res->getInt("HighestKillstreak"));
                    playerInfoStructure.headshots = static_cast<std::uint64_t>(res->getInt("Headshots"));
                    playerInfoStructure.playtime = static_cast<std::uint32_t>(res->getInt("Playtime"));
                    playerInfoStructure.clanId = static_cast<std::uint32_t>(res->getInt("ClanID"));
                    playerInfoStructure.latestSelectedCharacter = static_cast<std::uint64_t>(res->getInt("LastCharacterUsed"));
                    playerInfoStructure.playerLevel = static_cast<std::uint64_t>(res->getInt("Level")) + 1;
                    playerInfoStructure.battery = static_cast<std::uint64_t>(res->getInt("Battery"));
                    playerInfoStructure.luckyPoints = static_cast<std::uint64_t>(res->getInt("LuckyPoints"));
                    playerInfoStructure.coins = static_cast<std::uint64_t>(res->getInt("Coins"));
                    playerInfoStructure.playerGrade = static_cast<std::uint64_t>(res->getInt("Grade"));
                    playerInfoStructure.experience = static_cast<std::uint32_t>(res->getInt("Experience"));
                    playerInfoStructure.microPoints = static_cast<std::uint64_t>(res->getInt64("MicroPoints"));
                    playerInfoStructure.rockTotens = static_cast<std::uint64_t>(res->getInt64("RockTotens"));
                    playerInfoStructure.inventorySpace = static_cast<std::uint32_t>(res->getInt("MaxInventory"));
                    playerInfoStructure.isTutorialDone = static_cast<std::uint32_t>(res->getInt("HasFinishedTutorial"));
                    playerInfoStructure.maxBattery = static_cast<std::uint32_t>(res->getInt("MaxBattery"));
                    playerInfoStructure.singleWaveAttempts = static_cast<std::uint32_t>(res->getInt("SingleWaveAttempts"));
                    playerInfoStructure.highestSinglewaveStage = static_cast<std::uint32_t>(res->getInt("SingleWaveAttempts"));
                    playerInfoStructure.highestSingleWaveScore = static_cast<std::uint32_t>(res->getInt("HighestSinglewaveScore"));
                    playerInfoStructure.vipExperience = static_cast<std::uint32_t>(res->getInt("VipExperience"));
                    playerInfoStructure.clanContribution = static_cast<std::uint64_t>(res->getInt64("ClanContribution"));
                    playerInfoStructure.clanLogoFrontId = static_cast<std::uint64_t>(res->getInt("Clan_FrontIcon"));
                    playerInfoStructure.clanLogoBackId = static_cast<std::uint64_t>(res->getInt("Clan_BackIcon"));
                    playerInfoStructure.clanWins = static_cast<std::uint64_t>(res->getInt("ClanWins"));
                    playerInfoStructure.clanLosses = static_cast<std::uint64_t>(res->getInt("ClanLoses"));
                    playerInfoStructure.clanDraws = static_cast<std::uint64_t>(res->getInt("ClanDraws"));
                    playerInfoStructure.clanKills = static_cast<std::uint32_t>(res->getInt("ClanKills"));
                    playerInfoStructure.clanDeaths = static_cast<std::uint32_t>(res->getInt("ClanDeaths"));
                    playerInfoStructure.clanAssists = static_cast<std::uint32_t>(res->getInt("ClanAssists"));
                    playerInfoStructure.infected = static_cast<std::uint32_t>(res->getInt("InfectedKills"));
                    playerInfoStructure.zombieKills = res->getInt("ZombieKills");

                    std::vector<Common::Enums::Characters> boughtCharacterTypes{};
                    boughtCharacterTypes.reserve(static_cast<std::size_t>(Common::Enums::Characters::Sophitia));
                    for (std::size_t currentCharacter = 0; currentCharacter <= static_cast<std::size_t>(Common::Enums::Characters::Sophitia); ++currentCharacter)
                    {
                        boughtCharacterTypes.push_back(static_cast<Common::Enums::Characters>(currentCharacter));
                    }
                    playerInfoStructure.setBoughtCharacters(boughtCharacterTypes);

                    // achievements
                    for (const auto& currentAchievement : getPlayerAchievements(playerID))
                    {
                        playerInfoStructure.achievements.setAchievementTier1(currentAchievement.first);
                    }
                }
            }
            catch (const sql::SQLException& e)
            {
                ::Utils::Logger::log("MariaDB exception: " + std::string(e.what()), Utils::LogType::Error, "PersistentDatabase::getPlayerInfo");
                return std::nullopt;
            }

            return playerInfoStructure;
        }

        bool PersistentDatabase::resetAccountKey(std::uint32_t accountId)
        {
            try
            {
                std::string queryStr = "UPDATE Users SET AccountKey = 0 WHERE AccountID = ?";

                std::unique_ptr<sql::PreparedStatement> stmt(m_con->prepareStatement(queryStr));
                stmt->setUInt(1, accountId);

                return stmt->executeUpdate() > 0;
            }
            catch (const sql::SQLException& e)
            {
                ::Utils::Logger::log("MariaDB exception: " + std::string(e.what()), Utils::LogType::Error, "PersistentDatabase::resetAccountKey");
                return false;
            }
        }

        std::optional<std::pair<Main::Structures::AccountInfo, std::string>> PersistentDatabase::getPlayerInfoByNickname(const std::string& nickname)
        {
            Main::Structures::AccountInfo playerInfoStructure{};
            try
            {
                std::string queryStr = "SELECT AccountID, Grade, Level, MicroPoints, RockTotens, LastLogged "
                    "FROM Users WHERE Nickname = ?";

                std::unique_ptr<sql::PreparedStatement> stmt(m_con->prepareStatement(queryStr));
                stmt->setString(1, nickname);
                std::unique_ptr<sql::ResultSet> res(stmt->executeQuery());

                if (res->next())
                {
                    playerInfoStructure.accountID = static_cast<std::uint32_t>(res->getUInt("AccountID"));
                    playerInfoStructure.playerGrade = static_cast<std::uint32_t>(res->getInt("Grade"));
                    playerInfoStructure.playerLevel = static_cast<std::uint64_t>(res->getInt("Level")) + 1;
                    playerInfoStructure.microPoints = static_cast<std::uint64_t>(res->getInt64("MicroPoints"));
                    playerInfoStructure.rockTotens = static_cast<std::uint64_t>(res->getInt64("RockTotens"));

                    const char* lastLoggedCStr = res->getString("LastLogged").c_str();
                    std::string lastLogged = std::string(lastLoggedCStr);
                    return std::make_optional<std::pair<Main::Structures::AccountInfo, std::string>>(playerInfoStructure, lastLogged);
                }
                else
                {
                    return std::nullopt;
                }
            }
            catch (const sql::SQLException& e)
            {
                ::Utils::Logger::log("MariaDB exception: " + std::string(e.what()), Utils::LogType::Error, "PersistentDatabase::getPlayerInfoByNickname");
                return std::nullopt;;
            }
        }

        Main::Structures::MuteInfo PersistentDatabase::isMuted(std::uint32_t playerID)
        {
            try
            {
                std::unique_ptr<sql::PreparedStatement> stmt(
                    m_con->prepareStatement("SELECT * FROM Users WHERE AccountID = ?")
                );
                stmt->setUInt(1, playerID);
                std::unique_ptr<sql::ResultSet> res(stmt->executeQuery());

                if (res->next())
                {
                    const std::string mutedUntilStr = res->getString("MutedUntil").c_str();
                    const std::string muteReason = res->getString("MuteReason").c_str();
                    const std::string mutedBy = res->getString("MutedBy").c_str();

                    std::uint64_t mutedUntilEpoch = Common::Utils::datetimeToEpoch(mutedUntilStr);
                    std::uint64_t nowEpoch = static_cast<std::uint64_t>(std::chrono::duration_cast<std::chrono::seconds>(std::chrono::system_clock::now().time_since_epoch()).count());
                    bool isMuted = nowEpoch < mutedUntilEpoch;
                    return Main::Structures::MuteInfo{ isMuted, muteReason, mutedBy, mutedUntilStr };
                }
                else
                {
                    ::Utils::Logger::log("No muteinfo found for AccountID: " + std::to_string(playerID),Utils::LogType::Warning,"PersistentDatabase::isMuted");return {};
                }
            }
            catch (const sql::SQLException& e)
            {
                ::Utils::Logger::log("MariaDB exception: " + std::string(e.what()),Utils::LogType::Error,"PersistentDatabase::isMuted");
                return Main::Structures::MuteInfo{ false, "", "", ""};
            }
        }

        std::optional<Main::Structures::MuteInfo> PersistentDatabase::getMuteInfoByNickname(const std::string& nickname)
        {
            try
            {
                std::unique_ptr<sql::PreparedStatement> stmt(m_con->prepareStatement("SELECT * FROM Users WHERE Nickname = ?"));
                stmt->setString(1, nickname);
                std::unique_ptr<sql::ResultSet> res(stmt->executeQuery());

                if (res->next())
                {
                    const std::string mutedUntilStr = res->getString("MutedUntil").c_str();
                    const std::string muteReason = res->getString("MuteReason").c_str();
                    const std::string mutedBy = res->getString("MutedBy").c_str();

                    std::uint64_t mutedUntilEpoch = Common::Utils::datetimeToEpoch(mutedUntilStr);
                    std::uint64_t nowEpoch = static_cast<std::uint64_t>(std::chrono::duration_cast<std::chrono::seconds>(std::chrono::system_clock::now().time_since_epoch()).count());
                    bool isMuted = nowEpoch < mutedUntilEpoch;
                    return Main::Structures::MuteInfo{ isMuted, muteReason, mutedBy, mutedUntilStr };
                }
                else
                {
                    ::Utils::Logger::log("No muteinfo found for nickname: " + nickname,Utils::LogType::Warning,"PersistentDatabase::getMuteInfoByNickname");
                    return std::nullopt;
                }
            }
            catch (const sql::SQLException& e)
            {
                ::Utils::Logger::log("MariaDB exception: " + std::string(e.what()),Utils::LogType::Error,"PersistentDatabase::getMuteInfoByNickname");
                return std::nullopt;;
            }
        }

        std::optional<Main::Structures::BanInfo> PersistentDatabase::getBanInfoByNickname(const std::string& nickname)
        {
            try
            {
                std::unique_ptr<sql::PreparedStatement> stmt(m_con->prepareStatement("SELECT * FROM Users WHERE Nickname = ?"));
                stmt->setString(1, nickname);
                std::unique_ptr<sql::ResultSet> res(stmt->executeQuery());

                if (res->next())
                {
                    const std::string suspendedUntilStr = res->getString("SuspendedUntil").c_str();
                    const std::string suspensionReason = res->getString("SuspensionReason").c_str();

                    std::uint64_t suspendedUntilEpoch = Common::Utils::datetimeToEpoch(suspendedUntilStr);
                    std::uint64_t nowEpoch = static_cast<std::uint64_t>(std::chrono::duration_cast<std::chrono::seconds>(std::chrono::system_clock::now().time_since_epoch()).count());
                    bool isBanned = nowEpoch < suspendedUntilEpoch;
                    return Main::Structures::BanInfo{ isBanned, suspensionReason, suspendedUntilStr };
                }
                else
                {
                    ::Utils::Logger::log("No baninfo found for targetPlayer " + nickname, Utils::LogType::Warning, "PersistentDatabase::getBanInfoByNickname");
                    return std::nullopt;
                }
            }
            catch (const sql::SQLException& e)
            {
                ::Utils::Logger::log("MariaDB exception: " + std::string(e.what()), Utils::LogType::Error, "PersistentDatabase::getBanInfoByNickname");
                return std::nullopt;;
            }
        }

        std::optional<std::string> PersistentDatabase::getRoomCreationDisabledUntil(const std::string& nickname)
        {
            try
            {
                std::string selectQueryStr = "SELECT RoomCreationDisabledUntil FROM Users WHERE Nickname = ?";
                std::unique_ptr<sql::PreparedStatement> stmt(m_con->prepareStatement(selectQueryStr));
                stmt->setString(1, nickname);

                std::unique_ptr<sql::ResultSet> res(stmt->executeQuery());
                if (res->next())
                {
                    const std::string disabledUntilStr = res->getString("RoomCreationDisabledUntil").c_str();
                    std::uint64_t disabledUntilEpoch = Common::Utils::datetimeToEpoch(disabledUntilStr);
                    std::uint64_t nowEpoch = static_cast<std::uint64_t>(std::chrono::duration_cast<std::chrono::seconds>(std::chrono::system_clock::now().time_since_epoch()).count());

                    if (nowEpoch < disabledUntilEpoch)
                        return disabledUntilStr;

                    return std::nullopt;
                }

                return std::nullopt;
            }
            catch (const sql::SQLException& e)
            {
                ::Utils::Logger::log("MariaDB exception: " + std::string(e.what()), Utils::LogType::Error, "PersistentDatabase::getRoomCreationDisabledUntil");
                return std::nullopt;;
            }
        }

        bool PersistentDatabase::isRoomCreationDisabled(std::uint32_t playerID)
        {
            try
            {
                std::unique_ptr<sql::PreparedStatement> stmt(m_con->prepareStatement("SELECT RoomCreationDisabledUntil FROM Users WHERE AccountID = ?"));
                stmt->setUInt(1, playerID);
                std::unique_ptr<sql::ResultSet> res(stmt->executeQuery());

                if (res->next())
                {
                    const std::string disabledUntilStr = res->getString("RoomCreationDisabledUntil").c_str();
                    std::uint64_t disabledUntilEpoch = Common::Utils::datetimeToEpoch(disabledUntilStr);
                    std::uint64_t nowEpoch = static_cast<std::uint64_t>(
                        std::chrono::duration_cast<std::chrono::seconds>(
                            std::chrono::system_clock::now().time_since_epoch()
                        ).count()
                        );

                    return nowEpoch < disabledUntilEpoch;
                }

                ::Utils::Logger::log("No room creation ban info found for AccountID: " + std::to_string(playerID), Utils::LogType::Warning, "PersistentDatabase::isRoomCreationDisabled");
                return false;
            }
            catch (const sql::SQLException& e)
            {
                ::Utils::Logger::log("MariaDB exception: " + std::string(e.what()), Utils::LogType::Error, "PersistentDatabase::isRoomCreationDisabled");
                return false;
            }
        }
       
        std::optional<std::string> PersistentDatabase::getVotekickDisabledUntil(const std::string& nickname)
        {
            try
            {
                std::string selectQueryStr = "SELECT VotekickDisabledUntil FROM Users WHERE Nickname = ?";
                std::unique_ptr<sql::PreparedStatement> stmt(m_con->prepareStatement(selectQueryStr));
                stmt->setString(1, nickname);

                std::unique_ptr<sql::ResultSet> res(stmt->executeQuery());
                if (res->next())
                {
                    const std::string disabledUntilStr = res->getString("VotekickDisabledUntil").c_str();
                    std::uint64_t disabledUntilEpoch = Common::Utils::datetimeToEpoch(disabledUntilStr);
                    std::uint64_t nowEpoch = static_cast<std::uint64_t>(
                        std::chrono::duration_cast<std::chrono::seconds>(
                            std::chrono::system_clock::now().time_since_epoch()
                        ).count()
                        );

                    if (nowEpoch < disabledUntilEpoch)
                        return disabledUntilStr;

                    return std::nullopt;
                }
                return std::nullopt;
            }
            catch (const sql::SQLException& e)
            {
                ::Utils::Logger::log("MariaDB exception: " + std::string(e.what()), Utils::LogType::Error, "PersistentDatabase::getVotekickDisabledUntil");
                return std::nullopt;;
            }
        }

        bool PersistentDatabase::isVotekickDisabled(std::uint32_t playerID)
        {
            try
            {
                std::unique_ptr<sql::PreparedStatement> stmt(m_con->prepareStatement("SELECT VotekickDisabledUntil FROM Users WHERE AccountID = ?"));
                stmt->setUInt(1, playerID);

                std::unique_ptr<sql::ResultSet> res(stmt->executeQuery());
                if (res->next())
                {
                    const std::string disabledUntilStr = res->getString("VotekickDisabledUntil").c_str();
                    std::uint64_t disabledUntilEpoch = Common::Utils::datetimeToEpoch(disabledUntilStr);
                    std::uint64_t nowEpoch = static_cast<std::uint64_t>(
                        std::chrono::duration_cast<std::chrono::seconds>(
                            std::chrono::system_clock::now().time_since_epoch()
                        ).count()
                        );

                    return nowEpoch < disabledUntilEpoch;
                }
                return false;
            }
            catch (const sql::SQLException& e)
            {
                ::Utils::Logger::log("MariaDB exception: " + std::string(e.what()), Utils::LogType::Error, "PersistentDatabase::isVotekickDisabled");
                return false;
            }
        }

        bool PersistentDatabase::unbanPlayer(const std::string& nickname)
        {
            try
            {
                std::string queryStr = "UPDATE Users SET SuspendedUntil = 0, SuspensionReason = '' WHERE Nickname = ?";
                std::unique_ptr<sql::PreparedStatement> stmt(m_con->prepareStatement(queryStr));
                stmt->setString(1, nickname);

                if (stmt->executeUpdate() == 0)
                {
                    ::Utils::Logger::log("No rows changed for player: " + nickname, Utils::LogType::Warning, "PersistentDatabase::unbanPlayer");
                    return false;
                }
                return true;
            }
            catch (const sql::SQLException& e)
            {
                ::Utils::Logger::log("MariaDB exception: " + std::string(e.what()), Utils::LogType::Error, "PersistentDatabase::unbanPlayer");
                return false;
            }
        }

        bool PersistentDatabase::addPlayer(const std::string& username, const std::string& password, const std::string& nickname, const std::string& email,
            const std::string& secret2fa)
        {
            try
            {
                std::string queryStr = "INSERT INTO Users (Username, Password, Nickname, Email, Secret) VALUES (?, ?, ?, ?, ?)";
                std::unique_ptr<sql::PreparedStatement> stmt(m_con->prepareStatement(queryStr));
                stmt->setString(1, username);
                stmt->setString(2, password);
                stmt->setString(3, nickname);
                stmt->setString(4, email);
                stmt->setString(5, Common::Utils::SetupParser::getInstance().getAuthSetup().enhancedSecurity ? secret2fa : "");

                if (stmt->executeUpdate() == 0)
                {
                    ::Utils::Logger::log("Error in query execution: " + queryStr, Utils::LogType::Warning, "PersistentDatabase::addPlayer");
                    return false;
                }
                return true;
            }
            catch (const sql::SQLException& e)
            {
                ::Utils::Logger::log("MariaDB exception: " + std::string(e.what()), Utils::LogType::Error, "PersistentDatabase::addPlayer");
                return false;
            }
        }

        bool PersistentDatabase::playerExistsByNickname(const std::string& nickname)
        {
            try
            {
                std::unique_ptr<sql::PreparedStatement> stmt(m_con->prepareStatement("SELECT COUNT(*) FROM Users WHERE Nickname = ?"));
                stmt->setString(1, nickname);
                std::unique_ptr<sql::ResultSet> res(stmt->executeQuery());

                if (res->next())
                {
                    return res->getUInt(1) > 0;
                }
                return false;
            }
            catch (const sql::SQLException& e)
            {
                ::Utils::Logger::log("MariaDB exception: " + std::string(e.what()),Utils::LogType::Error,"PersistentDatabase::playerExistsByNickname");
                return false;
            }
        }

        bool PersistentDatabase::playerExistsByUsername(const std::string& username)
        {
            try
            {
                std::unique_ptr<sql::PreparedStatement> stmt(m_con->prepareStatement("SELECT COUNT(*) FROM Users WHERE Username = ?"));
                stmt->setString(1, username);
                std::unique_ptr<sql::ResultSet> res(stmt->executeQuery());

                if (res->next())
                {
                    return res->getUInt(1) > 0;
                }
                return false;
            }
            catch (const sql::SQLException& e)
            {
                ::Utils::Logger::log("MariaDB exception: " + std::string(e.what()), Utils::LogType::Error, "PersistentDatabase::playerExistsByUsername");
                return false;
            }
        }

        bool PersistentDatabase::setPlayerLevelByName(const std::string& nickname, std::uint16_t level, std::uint32_t experience, std::uint32_t executorGrade)
        {
            try
            {
                const std::string checkQuery = "SELECT AccountID, Grade FROM Users WHERE Nickname = ?";
                std::unique_ptr<sql::PreparedStatement> checkStmt(m_con->prepareStatement(checkQuery));
                checkStmt->setString(1, nickname);

                std::unique_ptr<sql::ResultSet> res(checkStmt->executeQuery());
                if (!res->next())
                {
                    return false;
                }
                if (res->getInt("Grade") > static_cast<int>(executorGrade))
                {
                    return false;
                }

                const std::uint32_t accountId = res->getUInt("AccountID");
                const std::string updateQuery = "UPDATE Users SET Level = ?, Experience = ? WHERE AccountID = ?";
                std::unique_ptr<sql::PreparedStatement> stmt(m_con->prepareStatement(updateQuery));
                stmt->setUInt(1, level);
                stmt->setUInt(2, experience);
                stmt->setUInt(3, accountId);
                stmt->executeUpdate();
                return true;
            }
            catch (const sql::SQLException& e)
            {
                ::Utils::Logger::log("[Main::Database::setPlayerLevelByName] MariaDB exception: " + std::string(e.what()) +
                    " | Nickname: " + nickname,
                    Utils::LogType::Error, "PersistentDatabase::setPlayerLevelByName");
                return false;
            }
        }

        bool PersistentDatabase::setCurrencyByName(const std::string& nickname, std::uint32_t rockTotens, std::uint32_t microPoints, std::uint16_t coins, std::uint32_t executorGrade)
        {
            try
            {
                const std::string checkQuery = "SELECT AccountID, Grade FROM Users WHERE Nickname = ?";
                std::unique_ptr<sql::PreparedStatement> checkStmt(m_con->prepareStatement(checkQuery));
                checkStmt->setString(1, nickname);

                std::unique_ptr<sql::ResultSet> res(checkStmt->executeQuery());
                if (!res->next())
                {
                    return false;
                }
                if (res->getInt("Grade") > static_cast<int>(executorGrade))
                {
                    return false;
                }

                const std::uint32_t accountId = res->getUInt("AccountID");
                const std::string updateQuery = "UPDATE Users SET RockTotens = ?, MicroPoints = ?, Coins = ? WHERE AccountID = ?";
                std::unique_ptr<sql::PreparedStatement> stmt(m_con->prepareStatement(updateQuery));
                stmt->setUInt(1, rockTotens);
                stmt->setUInt(2, microPoints);
                stmt->setUInt(3, coins);
                stmt->setUInt(4, accountId);
                stmt->executeUpdate();
                return true;
            }
            catch (const sql::SQLException& e)
            {
                ::Utils::Logger::log("[Main::Database::setCurrencyByName] MariaDB exception: " + std::string(e.what()) +
                    " | Nickname: " + nickname,
                    Utils::LogType::Error, "PersistentDatabase::setCurrencyByName");
                return false;
            }
        }

        void PersistentDatabase::updatePlayerLevel(std::uint32_t accountID, std::uint16_t level)
        {
            try
            {
                std::string updateLevelQuery = "UPDATE Users SET Level = ? WHERE AccountID = ?";
                std::unique_ptr<sql::PreparedStatement> stmt(m_con->prepareStatement(updateLevelQuery));

                stmt->setUInt(1, level);
                stmt->setUInt(2, accountID);

                if (stmt->executeUpdate() == 0)
                {
                    ::Utils::Logger::log("No rows changed with query", Utils::LogType::Warning, "PersistentDatabase::updatePlayerLevel");
                    return;
                }
            }
            catch (const sql::SQLException& e)
            {
                ::Utils::Logger::log(std::string("MariaDB exception: ") + e.what(), Utils::LogType::Error, "PersistentDatabase::updatePlayerLevel");
                return;
            }
        }

        void PersistentDatabase::updatePlayerExperience(std::uint32_t accountID, std::uint32_t exp)
        {
            try
            {
                std::string updateLevelQuery = "UPDATE Users SET Experience = ? WHERE AccountID = ?";
                std::unique_ptr<sql::PreparedStatement> stmt(m_con->prepareStatement(updateLevelQuery));

                stmt->setUInt(1, exp);
                stmt->setUInt(2, accountID);

                if (stmt->executeUpdate() == 0)
                {
                    ::Utils::Logger::log("No rows changed with query", Utils::LogType::Warning, "PersistentDatabase::updatePlayerExperience");
                    return;
                }
            }
            catch (const sql::SQLException& e)
            {
                ::Utils::Logger::log(std::string("MariaDB exception: ") + e.what(), Utils::LogType::Error, "PersistentDatabase::updatePlayerExperience");
                return;
            }
        }

        std::expected<bool, std::string> PersistentDatabase::updatePlayerName(std::uint32_t accountID, const char* name, bool isStaff)
        {
            try
            {
                std::unique_ptr<sql::PreparedStatement> checkDuplicateStmt(m_con->prepareStatement(
                    "SELECT AccountID FROM Users WHERE Nickname = ? AND AccountID != ?"));
                checkDuplicateStmt->setString(1, name);
                checkDuplicateStmt->setUInt(2, accountID);

                std::unique_ptr<sql::ResultSet> duplicateResult(checkDuplicateStmt->executeQuery());

                if (duplicateResult->next())
                {
                    return std::unexpected("There's already a player with this nickname");
                }

                if (!isStaff)
                {
                    std::unique_ptr<sql::PreparedStatement> checkStmt(m_con->prepareStatement(
                        "SELECT CanUpdateNickname FROM Users WHERE AccountID = ?"));
                    checkStmt->setUInt(1, accountID);
                    std::unique_ptr<sql::ResultSet> result(checkStmt->executeQuery());

                    if (!result->next())
                    {
                        return std::unexpected("User not found");
                    }

                    if (!result->getBoolean("CanUpdateNickname"))
                    {
                        return std::unexpected("You cannot change your nickname right now");
                    }
                }

                std::string updateNameQuery = "UPDATE Users SET Nickname = ? WHERE AccountID = ?";
                std::unique_ptr<sql::PreparedStatement> stmt(m_con->prepareStatement(updateNameQuery));

                stmt->setString(1, name);
                stmt->setUInt(2, accountID);

                if (stmt->executeUpdate() == 0)
                {
                    ::Utils::Logger::log("No rows changed with query", Utils::LogType::Warning, "PersistentDatabase::updatePlayerName");
                    return std::unexpected("Failed to update nickname");
                }

                if (!isStaff)
                {
                    std::unique_ptr<sql::PreparedStatement> resetStmt(m_con->prepareStatement(
                        "UPDATE Users SET CanUpdateNickname = 0 WHERE AccountID = ?"));
                    resetStmt->setUInt(1, accountID);
                    resetStmt->executeUpdate();
                }
            }
            catch (const sql::SQLException& e)
            {
                ::Utils::Logger::log(std::string("MariaDB exception: ") + e.what(), Utils::LogType::Error, "PersistentDatabase::updatePlayerName");
                return std::unexpected("Database error occurred");
            }

            return true; 
        }

        bool PersistentDatabase::updateSuspension(const std::string& nickname, const std::string& until, const std::string& reason, std::uint32_t executorGrade)
        {
            try
            {
                TransactionGuard tg(m_transactionalCon.get());

                std::string checkGradeQuery = "SELECT Grade FROM Users WHERE Nickname = ?";
                std::unique_ptr<sql::PreparedStatement> checkStmt(m_transactionalCon->prepareStatement(checkGradeQuery));
                checkStmt->setString(1, nickname);

                std::unique_ptr<sql::ResultSet> res(checkStmt->executeQuery());
                if (!res->next())
                {
                    return false;
                }

                if (res->getInt(1) >= static_cast<int>(executorGrade))
                {
                    return false;
                }

                std::string updateQuery = "UPDATE Users SET SuspendedUntil = ?, SuspensionReason = ? WHERE Nickname = ?";
                std::unique_ptr<sql::PreparedStatement> updateStmt(m_transactionalCon->prepareStatement(updateQuery));
                updateStmt->setString(1, until);
                updateStmt->setString(2, reason);
                updateStmt->setString(3, nickname);

                if (updateStmt->executeUpdate() == 0)
                {
                    ::Utils::Logger::log("Error executing query: " + updateQuery, Utils::LogType::Warning, "PersistentDatabase::updateSuspension");
                    return false;
                }

                tg.commit();
            }
            catch (const sql::SQLException& e)
            {
                ::Utils::Logger::log(std::string("MariaDB exception: ") + e.what(), Utils::LogType::Error, "PersistentDatabase::updateSuspension");
                return false;
            }
            return true;
        }

        void PersistentDatabase::updateLatestRewardDay(const std::string& columnName, std::uint32_t accountId, const std::string& rewardDay)
        {
            try
            {
                std::string queryStr = "UPDATE Users SET " + columnName + " = ? WHERE AccountID = ?";
                std::unique_ptr<sql::PreparedStatement> stmt(m_con->prepareStatement(queryStr));

                stmt->setString(1, rewardDay);
                stmt->setUInt(2, accountId);

                if (stmt->executeUpdate() == 0)
                {
                    ::Utils::Logger::log("Error executing query: " + queryStr, Utils::LogType::Warning, "PersistentDatabase::updateLatestRewardDay");
                    return;
                }
            }
            catch (const sql::SQLException& e)
            {
                ::Utils::Logger::log(std::string("MariaDB exception: ") + e.what(), Utils::LogType::Error, "PersistentDatabase::updateLatestRewardDay");
                return;
            }
        }

        std::string PersistentDatabase::getLatestRewardDayFor(const std::string& columnName, std::uint32_t accountId)
        {
            try
            {
                const std::string queryStr = "SELECT " + columnName + " FROM Users WHERE AccountID = ?";
                std::unique_ptr<sql::PreparedStatement> stmt(m_con->prepareStatement(queryStr));

                stmt->setUInt(1, accountId);
                std::unique_ptr<sql::ResultSet> res(stmt->executeQuery());

                if (res->next())
                {
                    return res->getString(1).c_str();
                }
                else
                {
                    return "";
                }
            }
            catch (const sql::SQLException& e)
            {
                ::Utils::Logger::log(std::string("MariaDB exception: ") + e.what(), Utils::LogType::Error, "PersistentDatabase::getLatestRewardDayFor");
                return "";
            }
        }

        bool PersistentDatabase::mustRewardsBeUpdated(const std::string& tableName, std::uint32_t daysToCheck)
        {
            try
            {
                std::string queryStr = "SELECT Date FROM " + tableName + " ORDER BY Date DESC LIMIT 1";
                std::unique_ptr<sql::PreparedStatement> stmt(m_con->prepareStatement(queryStr));
                std::unique_ptr<sql::ResultSet> res(stmt->executeQuery());

                if (res->next())
                {
                    std::string dateStr = res->getString("Date").c_str();

                    std::istringstream ss(dateStr);
                    int year, month, day;
                    char char1, char2;
                    ss >> year >> char1 >> month >> char2 >> day;

                    if (ss.fail() || char1 != '-' || char2 != '-' || month < 1 || month > 12 || day < 1 || day > 31)
                    {
                        return false;
                    }

                    std::chrono::year_month_day parsedDate = std::chrono::year_month_day(std::chrono::year(year), std::chrono::month(month), std::chrono::day(day));
                    auto todayTimestamp = std::chrono::floor<std::chrono::days>(std::chrono::system_clock::now());
                    return std::chrono::duration_cast<std::chrono::days>(todayTimestamp - std::chrono::sys_days(parsedDate)).count() >= daysToCheck;
                }
                else
                {
                    return true;
                }
            }
            catch (const sql::SQLException& e)
            {
                ::Utils::Logger::log("MariaDB exception: " + std::string(e.what()), Utils::LogType::Error, "PersistentDatabase::mustRewardsBeUpdated");
                return false;
            }
        }

        void PersistentDatabase::updateRewards(const std::string& tableName, const std::vector<std::uint32_t>& items)
        {
            using std::chrono::system_clock;
            const auto today = std::format("{:%F}", std::chrono::floor<std::chrono::days>(system_clock::now()));

            try
            {
                TransactionGuard tg(m_transactionalCon.get());

                std::string countQueryStr = "SELECT COUNT(*) FROM " + tableName;
                std::unique_ptr<sql::PreparedStatement> countStmt(m_transactionalCon->prepareStatement(countQueryStr));
                std::unique_ptr<sql::ResultSet> countRes(countStmt->executeQuery());

                if (countRes->next() && countRes->getInt(1) > 0)
                {
                    std::string deleteQueryStr = "DELETE FROM " + tableName;
                    std::unique_ptr<sql::PreparedStatement> deleteStmt(m_transactionalCon->prepareStatement(deleteQueryStr));
                    if (deleteStmt->executeUpdate() == 0)
                    {
                        return;
                    }
                }

                std::string insertQueryStr = "INSERT INTO " + tableName + " (ItemID, Date) VALUES (?, ?)";
                std::unique_ptr<sql::PreparedStatement> insertStmt(m_transactionalCon->prepareStatement(insertQueryStr));

                for (const auto& item : items)
                {
                    insertStmt->setUInt(1, item);
                    insertStmt->setString(2, today);
                    if (insertStmt->executeUpdate() == 0)
                    {
                        return;
                    }
                }

                tg.commit();
            }
            catch (const sql::SQLException& e)
            {
                ::Utils::Logger::log("MariaDB exception: " + std::string(e.what()), Utils::LogType::Error, "PersistentDatabase::updateRewards");
                return;
            }
        }

        void PersistentDatabase::updateBattery(std::uint32_t accountId, std::uint32_t newBattery)
        {
            try
            {
                std::string updateQueryStr = "UPDATE Users SET Battery = ? WHERE AccountID = ?";
                std::unique_ptr<sql::PreparedStatement> stmt(m_con->prepareStatement(updateQueryStr));

                stmt->setUInt(1, newBattery);
                stmt->setUInt(2, accountId);

                if (stmt->executeUpdate() == 0)
                {
                    ::Utils::Logger::log("Update failed.", Utils::LogType::Warning, "PersistentDatabase::updateBattery");
                    return;
                }
            }
            catch (const sql::SQLException& e)
            {
                ::Utils::Logger::log("MariaDB exception: " + std::string(e.what()), Utils::LogType::Error, "PersistentDatabase::updateBattery");
                return;
            }
        }

        void PersistentDatabase::updatePlayerStats(std::uint32_t accountId, const Main::Structures::AccountInfo& updatedAccountInfo)
        {
            try
            {
                std::string updateQueryStr = "UPDATE Users SET MeleeKills = ?, RifleKills = ?, ShotgunKills = ?, SniperKills = ?, GatlingKills = ?, "
                    "BazookaKills = ?, GrenadeKills = ?, HighestKillstreak = ?, Kills = ?, Deaths = ?, Headshots = ?, Assists = ?, "
                    "Experience = ?, MicroPoints = ?, Wins = ?, Loses = ?, Draws = ?, Level = ?, ZombieKills = ?, InfectedKills = ?, "
                    "ClanKills = ?, ClanDeaths = ?, ClanAssists = ?, ClanWins = ?, ClanLoses = ?, ClanDraws = ?, ClanContribution = ?, "
                    "HighestSinglewaveScore = ?, HighestSinglewaveStage = ?, HasFinishedTutorial = ?, Playtime = ? WHERE AccountID = ? ";

                std::unique_ptr<sql::PreparedStatement> stmt(m_con->prepareStatement(updateQueryStr));

                stmt->setUInt(1, updatedAccountInfo.meleeKills);
                stmt->setUInt(2, updatedAccountInfo.rifleKills);
                stmt->setUInt(3, updatedAccountInfo.shotgunKills);
                stmt->setUInt(4, updatedAccountInfo.sniperKills);
                stmt->setUInt(5, updatedAccountInfo.microgunKills);
                stmt->setUInt(6, updatedAccountInfo.bazookaKills);
                stmt->setUInt(7, updatedAccountInfo.grenadeKills);
                stmt->setUInt(8, static_cast<std::uint32_t>(updatedAccountInfo.killstreak));
                stmt->setUInt(9, updatedAccountInfo.totalKills);
                stmt->setUInt(10, updatedAccountInfo.deaths);
                stmt->setUInt(11, static_cast<std::uint32_t>(updatedAccountInfo.headshots));
                stmt->setUInt(12, updatedAccountInfo.assists);
                stmt->setUInt(13, updatedAccountInfo.experience);
                stmt->setUInt(14, static_cast<std::uint32_t>(updatedAccountInfo.microPoints));
                stmt->setUInt(15, updatedAccountInfo.wins);
                stmt->setUInt(16, updatedAccountInfo.losses);
                stmt->setUInt(17, updatedAccountInfo.draws);
                stmt->setUInt(18, static_cast<std::uint32_t>(updatedAccountInfo.playerLevel - 1));
                stmt->setUInt(19, updatedAccountInfo.zombieKills); 
                stmt->setUInt(20, updatedAccountInfo.infected);    
                stmt->setUInt(21, updatedAccountInfo.clanKills);
                stmt->setUInt(22, updatedAccountInfo.clanDeaths);
                stmt->setUInt(23, updatedAccountInfo.clanAssists);
                stmt->setUInt(24, updatedAccountInfo.clanWins);
                stmt->setUInt(25, updatedAccountInfo.clanLosses);
                stmt->setUInt(26, updatedAccountInfo.clanDraws);
                stmt->setUInt(27, updatedAccountInfo.clanContribution);
                stmt->setUInt(28, updatedAccountInfo.highestSingleWaveScore);
                stmt->setUInt(29, updatedAccountInfo.highestSinglewaveStage);
                stmt->setUInt(30, updatedAccountInfo.isTutorialDone);
                stmt->setUInt(31, updatedAccountInfo.playtime); 
                stmt->setUInt(32, accountId); 


                if (stmt->executeUpdate() == 0)
                {
                    ::Utils::Logger::log("Update failed.", Utils::LogType::Warning, "PersistentDatabase::updatePlayerStats");
                }
            }
            catch (const sql::SQLException& e)
            {
                ::Utils::Logger::log("MariaDB exception: " + std::string(e.what()), Utils::LogType::Error, "PersistentDatabase::updatePlayerStats");
                return;
            }
        }

        bool PersistentDatabase::updateMute(const std::string& nickname, const std::string& until,const std::string& reason, const std::string& mutedBy,
            std::uint32_t executorGrade)
        {
            try
            {
                TransactionGuard tg(m_transactionalCon.get());

                std::string checkGradeQuery = "SELECT Grade FROM Users WHERE Nickname = ?";
                std::unique_ptr<sql::PreparedStatement> checkStmt(m_transactionalCon->prepareStatement(checkGradeQuery));
                checkStmt->setString(1, nickname);

                std::unique_ptr<sql::ResultSet> resultSet(checkStmt->executeQuery());  

                if (!resultSet->next())
                {
                    return false; 
                }

                const int targetPlayerGrade = resultSet->getInt("Grade");
                if (targetPlayerGrade >= executorGrade)
                {
                    return false;  
                }

                std::string updateQuery = "UPDATE Users SET MutedUntil = ?, MuteReason = ?, MutedBy = ? WHERE Nickname = ?";
                std::unique_ptr<sql::PreparedStatement> updateStmt(m_transactionalCon->prepareStatement(updateQuery));
                updateStmt->setString(1, until);
                updateStmt->setString(2, reason);
                updateStmt->setString(3, mutedBy);
                updateStmt->setString(4, nickname);

                if (updateStmt->executeUpdate() == 0)
                {
                    ::Utils::Logger::log("No rows updated in updateMute", Utils::LogType::Warning, "PersistentDatabase::updateMute");
                    return false;
                }

                tg.commit();
                return true;
            }
            catch (const sql::SQLException& e)
            {
                ::Utils::Logger::log("MariaDB exception: " + std::string(e.what()),
                    Utils::LogType::Error, "PersistentDatabase::updateMute");
                return false;
            }
        }

        bool PersistentDatabase::updateVotekickDisabledUntil(const std::string& nickname, const std::string& until)
        {
            try
            {
                std::string updateQuery = "UPDATE Users SET VotekickDisabledUntil = ? WHERE Nickname = ?";
                std::unique_ptr<sql::PreparedStatement> updateStmt(m_con->prepareStatement(updateQuery));
                updateStmt->setString(1, until);
                updateStmt->setString(2, nickname);

                if (updateStmt->executeUpdate() == 0)
                {
                    ::Utils::Logger::log("Update failed: No rows affected.", Utils::LogType::Warning, "PersistentDatabase::updateVotekickDisabledUntil");
                    return false;
                }

                return true;
            }
            catch (const sql::SQLException& e)
            {
                ::Utils::Logger::log("MariaDB exception: " + std::string(e.what()), Utils::LogType::Error, "PersistentDatabase::updateVotekickDisabledUntil");
                return false;
            }
        }

        bool PersistentDatabase::resetVotekickDisabledUntil(const std::string& nickname)
        {
            try
            {
                TransactionGuard tg(m_transactionalCon.get());

                std::string selectQueryStr = "SELECT AccountID FROM Users WHERE Nickname = ?";
                std::unique_ptr<sql::PreparedStatement> selectStmt(m_transactionalCon->prepareStatement(selectQueryStr));
                selectStmt->setString(1, nickname);

                std::unique_ptr<sql::ResultSet> resultSet(selectStmt->executeQuery());
                if (!resultSet->next())
                {
                    return false;
                }

                const std::uint32_t accountID = resultSet->getUInt("AccountID");
                std::string updateQueryStr = "UPDATE Users SET VotekickDisabledUntil = 0 WHERE AccountID = ?";
                std::unique_ptr<sql::PreparedStatement> updateStmt(m_transactionalCon->prepareStatement(updateQueryStr));
                updateStmt->setUInt(1, accountID);

                if (updateStmt->executeUpdate() == 0)
                {
                    return false;
                }

                tg.commit();
                return true;
            }
            catch (const sql::SQLException& e)
            {
                ::Utils::Logger::log("MariaDB exception: " + std::string(e.what()), Utils::LogType::Error, "PersistentDatabase::resetVotekickDisabledUntil");
                return false;
            }
        }

        bool PersistentDatabase::updateRoomCreationDisabledUntil(const std::string& nickname, const std::string& until)
        {
            try
            {
                std::string updateQuery = "UPDATE Users SET RoomCreationDisabledUntil = ? WHERE Nickname = ?";
                std::unique_ptr<sql::PreparedStatement> updateStmt(m_con->prepareStatement(updateQuery));
                updateStmt->setString(1, until);
                updateStmt->setString(2, nickname);

                if (updateStmt->executeUpdate() == 0)
                {
                    ::Utils::Logger::log("Update failed: No rows affected.", Utils::LogType::Warning, "PersistentDatabase::updateRoomCreationDisabledUntil");
                    return false;
                }

                return true;
            }
            catch (const sql::SQLException& e)
            {
                ::Utils::Logger::log("MariaDB exception: " + std::string(e.what()), Utils::LogType::Error, "PersistentDatabase::updateRoomCreationDisabledUntil");
                return false;;
            }
        }

        bool PersistentDatabase::resetRoomCreationDisabledUntil(const std::string& nickname)
        {
            try
            {
                TransactionGuard tg(m_transactionalCon.get());

                std::string selectQueryStr = "SELECT AccountID FROM Users WHERE Nickname = ?";
                std::unique_ptr<sql::PreparedStatement> selectStmt(m_transactionalCon->prepareStatement(selectQueryStr));
                selectStmt->setString(1, nickname);

                std::unique_ptr<sql::ResultSet> resultSet(selectStmt->executeQuery());
                if (!resultSet->next())
                {
                    return false;
                }

                const std::uint32_t accountID = resultSet->getUInt("AccountID");
                std::string updateQueryStr = "UPDATE Users SET RoomCreationDisabledUntil = 0 WHERE AccountID = ?";
                std::unique_ptr<sql::PreparedStatement> updateStmt(m_transactionalCon->prepareStatement(updateQueryStr));
                updateStmt->setUInt(1, accountID);

                if (updateStmt->executeUpdate() == 0)
                {
                    return false;
                }

                tg.commit();
                return true;
            }
            catch (const sql::SQLException& e)
            {
                ::Utils::Logger::log("MariaDB exception: " + std::string(e.what()), Utils::LogType::Error, "PersistentDatabase::resetRoomCreationDisabledUntil");
                return false;;
            }
        }

        bool PersistentDatabase::unmuteAccount(const std::string& nickname)
        {
            try
            {
                TransactionGuard tg(m_transactionalCon.get());

                std::string selectQueryStr = "SELECT AccountID FROM Users WHERE Nickname = ?";
                std::unique_ptr<sql::PreparedStatement> selectStmt(m_transactionalCon->prepareStatement(selectQueryStr));
                selectStmt->setString(1, nickname);

                std::unique_ptr<sql::ResultSet> resultSet(selectStmt->executeQuery());
                if (!resultSet->next())
                {
                    return false;
                }

                const std::uint32_t accountID = resultSet->getUInt("AccountID");
                std::string updateQueryStr = "UPDATE Users SET MutedUntil = '1970-01-01 00:00:00' WHERE AccountID = ?";
                std::unique_ptr<sql::PreparedStatement> updateStmt(m_transactionalCon->prepareStatement(updateQueryStr));
                updateStmt->setUInt(1, accountID);

                if (updateStmt->executeUpdate() == 0)
                {
                    return false;
                }

                tg.commit();
                return true;
            }
            catch (const sql::SQLException& e)
            {
                ::Utils::Logger::log("MariaDB exception: " + std::string(e.what()), Utils::LogType::Error, "PersistentDatabase::unmuteAccount");
                return false;;
            }
        }

        void PersistentDatabase::addFriend(std::uint32_t accountID, std::uint32_t targetAccountId)
        {
            try
            {
                TransactionGuard tg(m_transactionalCon.get());

                const std::string insertQuery = "INSERT IGNORE INTO Friendlist (AccountID, TargetAccountID) VALUES (?, ?)";
                std::unique_ptr<sql::PreparedStatement> stmt(m_transactionalCon->prepareStatement(insertQuery));

                stmt->setUInt(1, accountID);
                stmt->setUInt(2, targetAccountId);
                stmt->executeUpdate(); 

                stmt->setUInt(1, targetAccountId);
                stmt->setUInt(2, accountID);
                stmt->executeUpdate(); 

                tg.commit();
            }
            catch (const sql::SQLException& e)
            {
                ::Utils::Logger::log("MariaDB exception: " + std::string(e.what()), Utils::LogType::Error, "PersistentDatabase::addFriend");
            }
        }

        bool PersistentDatabase::resetKillDeath(std::uint32_t accountID)
        {
            try
            {
                const std::string updateQuery = "UPDATE Users SET Kills = 0, Deaths = 0 WHERE AccountID = ?";
                std::unique_ptr<sql::PreparedStatement> stmt(m_con->prepareStatement(updateQuery));
                stmt->setUInt(1, accountID);

                bool success = stmt->executeUpdate() > 0;

                if (!success)
                {
                    ::Utils::Logger::log("Failed to reset kills/deaths for account: " + std::to_string(accountID),
                        Utils::LogType::Warning, "PersistentDatabase::resetKillDeath");
                }

                return success;
            }
            catch (const sql::SQLException& e)
            {
                ::Utils::Logger::log("MariaDB exception: " + std::string(e.what()),
                    Utils::LogType::Error, "PersistentDatabase::resetKillDeath");
                return false;;
            }
        }

        bool PersistentDatabase::resetRecord(std::uint32_t accountID)
        {
            try
            {
                const std::string resetQuery = "UPDATE Users SET Wins = 0, Loses = 0, Draws = 0 WHERE AccountID = ?";
                std::unique_ptr<sql::PreparedStatement> stmt(m_con->prepareStatement(resetQuery));
                stmt->setUInt(1, accountID);

                bool success = stmt->executeUpdate() > 0;

                if (!success)
                {
                    ::Utils::Logger::log("Failed to reset record for account: " + std::to_string(accountID),Utils::LogType::Warning, "PersistentDatabase::resetRecord");
                }

                return success;
            }
            catch (const sql::SQLException& e)
            {
                ::Utils::Logger::log("MariaDB exception: " + std::string(e.what()),Utils::LogType::Error, "PersistentDatabase::resetRecord");
                return false;;
            }
        }

        bool PersistentDatabase::batteryRecharge(std::uint32_t accountID, std::uint32_t quantity)
        {
            try
            {
                TransactionGuard tg(m_transactionalCon.get());

                const std::string getBattery = "SELECT Battery FROM Users WHERE AccountID = ?";
                const std::string updateBattery = "UPDATE Users SET Battery = ? WHERE AccountID = ?";

                std::unique_ptr<sql::PreparedStatement> stmtGetBattery(m_transactionalCon->prepareStatement(getBattery));
                std::unique_ptr<sql::PreparedStatement> stmtUpdateBattery(m_transactionalCon->prepareStatement(updateBattery));

                stmtGetBattery->setUInt(1, accountID);
                std::unique_ptr<sql::ResultSet> resGetBattery(stmtGetBattery->executeQuery());

                if (!resGetBattery->next())
                {
                    return false;
                }

                uint32_t currentBattery = resGetBattery->getUInt("Battery");
                stmtUpdateBattery->setUInt(1, currentBattery + quantity);
                stmtUpdateBattery->setUInt(2, accountID);

                if (stmtUpdateBattery->executeUpdate() == 0)
                {
                    return false;
                }

                tg.commit();
                return true;
            }
            catch (const sql::SQLException& e)
            {
                ::Utils::Logger::log("MariaDB exception: " + std::string(e.what()), Utils::LogType::Error, "PersistentDatabase::batteryRecharge");
                return false;;
            }
        }

        bool PersistentDatabase::batteryExpansion(std::uint32_t accountID)
        {
            try
            {
                TransactionGuard tg(m_transactionalCon.get());

                const std::string getMaxBattery = "SELECT MaxBattery FROM Users WHERE AccountID = ?";
                const std::string updateMaxBattery = "UPDATE Users SET MaxBattery = ? WHERE AccountID = ?";

                std::unique_ptr<sql::PreparedStatement> stmtGetMaxBattery(m_transactionalCon->prepareStatement(getMaxBattery));
                std::unique_ptr<sql::PreparedStatement> stmtUpdateMaxBattery(m_transactionalCon->prepareStatement(updateMaxBattery));

                stmtGetMaxBattery->setUInt(1, accountID);
                std::unique_ptr<sql::ResultSet> resGetMaxBattery(stmtGetMaxBattery->executeQuery());

                if (!resGetMaxBattery->next())
                {
                    return false;
                }

                uint32_t currentMaxBattery = resGetMaxBattery->getUInt("MaxBattery");
                if (currentMaxBattery > 4000)
                {
                    return false;
                }

                uint32_t newMaxBattery = currentMaxBattery + 1000;
                stmtUpdateMaxBattery->setUInt(1, newMaxBattery);
                stmtUpdateMaxBattery->setUInt(2, accountID);

                if (stmtUpdateMaxBattery->executeUpdate() == 0)
                {
                    return false;
                }

                tg.commit();
                return true;
            }
            catch (const sql::SQLException& e)
            {
                ::Utils::Logger::log("MariaDB exception: " + std::string(e.what()), Utils::LogType::Error, "PersistentDatabase::batteryExpansion");
                return false;;
            }
        }

        bool PersistentDatabase::inventoryExpansion(std::uint32_t accountID, std::uint32_t spaceToAdd)
        {
            try
            {
                TransactionGuard tg(m_transactionalCon.get());

                const std::string getMaxInventory = "SELECT MaxInventory FROM Users WHERE AccountID = ?";
                const std::string updateMaxInventory = "UPDATE Users SET MaxInventory = ? WHERE AccountID = ?";

                std::unique_ptr<sql::PreparedStatement> stmtGetMaxInventory(m_transactionalCon->prepareStatement(getMaxInventory));
                std::unique_ptr<sql::PreparedStatement> stmtUpdateMaxInventory(m_transactionalCon->prepareStatement(updateMaxInventory));

                stmtGetMaxInventory->setUInt(1, accountID);
                std::unique_ptr<sql::ResultSet> resGetMaxInventory(stmtGetMaxInventory->executeQuery());

                if (!resGetMaxInventory->next())
                {
                    return false;
                }

                uint32_t currentMaxInventory = resGetMaxInventory->getUInt("MaxInventory");

                if (currentMaxInventory + spaceToAdd > 1000)
                {
                    return false;
                }

                uint32_t newMaxInventory = currentMaxInventory + spaceToAdd;
                stmtUpdateMaxInventory->setUInt(1, newMaxInventory);
                stmtUpdateMaxInventory->setUInt(2, accountID);

                if (stmtUpdateMaxInventory->executeUpdate() == 0)
                {
                    return false;
                }

                tg.commit();
                return true;
            }
            catch (const sql::SQLException& e)
            {
                ::Utils::Logger::log("MariaDB exception: " + std::string(e.what()), Utils::LogType::Error, "PersistentDatabase::inventoryExpansion");
                return false;;
            }
        }

        void PersistentDatabase::removeFriend(std::uint32_t accountID, std::uint32_t targetAccountId)
        {
            try
            {
                const std::string deleteFriend = "DELETE FROM Friendlist WHERE (AccountID = ? AND TargetAccountID = ?) OR (AccountID = ? AND TargetAccountID = ?)";
                std::unique_ptr<sql::PreparedStatement> stmtDeleteFriend(m_con->prepareStatement(deleteFriend));

                stmtDeleteFriend->setUInt(1, accountID);
                stmtDeleteFriend->setUInt(2, targetAccountId);
                stmtDeleteFriend->setUInt(3, targetAccountId);
                stmtDeleteFriend->setUInt(4, accountID);

                const int deleteResult = stmtDeleteFriend->executeUpdate();
                if (deleteResult == 0)
                {
                    ::Utils::Logger::log("Error executing delete: no rows affected", Utils::LogType::Warning, "PersistentDatabase::removeFriend");
                    return;
                }
            }
            catch (const sql::SQLException& e)
            {
                ::Utils::Logger::log("MariaDB exception: " + std::string(e.what()), Utils::LogType::Error, "PersistentDatabase::removeFriend");
                return;
            }
        }

        std::vector<Main::Structures::Friend> PersistentDatabase::loadFriends(std::uint32_t accountID)
        {
            std::vector<Main::Structures::Friend> friendList;
            std::size_t count = 0;
            try
            {
                static const char* selectFriendsQuery = "SELECT Friendlist.*, Users.Nickname AS TargetNickname "
                    "FROM Friendlist INNER JOIN Users ON Friendlist.TargetAccountID = Users.AccountID "
                    "WHERE Friendlist.AccountID = ?";

                std::unique_ptr<sql::PreparedStatement> stmtSelectFriends(m_con->prepareStatement(selectFriendsQuery));
                stmtSelectFriends->setUInt(1, accountID);
                std::unique_ptr<sql::ResultSet> res(stmtSelectFriends->executeQuery());

                Main::Structures::Friend ffriend;
                while (res->next())
                {
                    if (count >= Common::Constants::maxFriends)
                        break;

                    std::memcpy(ffriend.targetNickname, res->getString("TargetNickname").c_str(), Common::Constants::maxNicknameSize);
                    ffriend.targetAccountId = res->getInt("TargetAccountID");

                    friendList.push_back(ffriend);
                    ++count;
                }
            }
            catch (const sql::SQLException& e)
            {
                ::Utils::Logger::log("MariaDB exception: " + std::string(e.what()), Utils::LogType::Error, "PersistentDatabase::loadFriends");
            }

            return friendList;
        }

        std::vector<Main::Structures::BlockedPlayer> PersistentDatabase::loadBlockedPlayers(std::uint32_t accountID)
        {
            std::vector<Main::Structures::BlockedPlayer> blockedList;
            std::size_t count = 0;
            try
            {
                static const char* selectBlockedPlayersQuery = "SELECT BlockedPlayers.*, Users.Nickname AS TargetNickname "
                    "FROM BlockedPlayers INNER JOIN Users ON BlockedPlayers.TargetAccountID = Users.AccountID "
                    "WHERE BlockedPlayers.AccountID = ?";

                std::unique_ptr<sql::PreparedStatement> stmtSelectBlockedPlayers(m_con->prepareStatement(selectBlockedPlayersQuery));
                stmtSelectBlockedPlayers->setUInt(1, accountID);

                std::unique_ptr<sql::ResultSet> res(stmtSelectBlockedPlayers->executeQuery());

                Main::Structures::BlockedPlayer blockedPlayer;
                while (res->next())
                {
                    if (count >= Common::Constants::maxFriends)
                        break;

                    std::memcpy(blockedPlayer.targetNickname, res->getString("TargetNickname").c_str(), Common::Constants::maxNicknameSize);
                    blockedPlayer.targetAccountId = res->getInt("TargetAccountID");

                    blockedList.push_back(blockedPlayer);
                    ++count;
                }
            }
            catch (const sql::SQLException& e)
            {
                ::Utils::Logger::log("MariaDB exception: " + std::string(e.what()), Utils::LogType::Error, "PersistentDatabase::loadBlockedPlayers");
            }

            return blockedList;
        }

        void PersistentDatabase::blockPlayer(std::uint32_t accountID, std::uint32_t targetAccountId)
        {
            try
            {
                std::unique_ptr<sql::PreparedStatement> stmtBlockPlayer(m_con->prepareStatement("INSERT INTO BlockedPlayers (AccountID, TargetAccountID) VALUES (?, ?)"));
                stmtBlockPlayer->setUInt(1, accountID);
                stmtBlockPlayer->setUInt(2, targetAccountId);

                if (stmtBlockPlayer->executeUpdate() == 0)
                {
                    ::Utils::Logger::log("Error executing query", Utils::LogType::Warning, "PersistentDatabase::blockPlayer");
                    return;
                }
            }
            catch (const sql::SQLException& e)
            {
                ::Utils::Logger::log("MariaDB exception: " + std::string(e.what()), Utils::LogType::Error, "PersistentDatabase::blockPlayer");
                return;
            }
        }

        std::optional<std::uint32_t> PersistentDatabase::blockPlayerByNickname(std::uint32_t accountID, const std::string& targetNickname)
        {
            try
            {
                TransactionGuard tg(m_transactionalCon.get());

                std::unique_ptr<sql::PreparedStatement> stmtFind(m_transactionalCon->prepareStatement(
                    "SELECT AccountID, Grade FROM Users WHERE Nickname = ?"));
                stmtFind->setString(1, targetNickname);
                std::unique_ptr<sql::ResultSet> resFind(stmtFind->executeQuery());

                if (!resFind->next())
                    return std::nullopt;

                const std::uint32_t targetAccountId = resFind->getUInt("AccountID");
                const std::uint32_t targetGrade = resFind->getUInt("Grade");

                if (targetGrade >= 3)
                    return std::nullopt;

                std::unique_ptr<sql::PreparedStatement> stmtBlockPlayer(m_transactionalCon->prepareStatement(
                    "INSERT INTO BlockedPlayers (AccountID, TargetAccountID) VALUES (?, ?)"));
                stmtBlockPlayer->setUInt(1, accountID);
                stmtBlockPlayer->setUInt(2, targetAccountId);

                if (stmtBlockPlayer->executeUpdate() == 0)
                    return std::nullopt;

                tg.commit();
                return targetAccountId;
            }
            catch (const sql::SQLException& e)
            {
                ::Utils::Logger::log("MariaDB exception: " + std::string(e.what()), Utils::LogType::Error, "PersistentDatabase::blockPlayerByNickname");
                return std::nullopt;
            }
        }

        void PersistentDatabase::unblockPlayer(std::uint32_t accountID, std::uint32_t targetAccountId)
        {
            try
            {
                std::unique_ptr<sql::PreparedStatement> stmtUnblockPlayer(m_con->prepareStatement("DELETE FROM BlockedPlayers WHERE AccountID = ? AND TargetAccountID = ?"));
                stmtUnblockPlayer->setUInt(1, accountID);
                stmtUnblockPlayer->setUInt(2, targetAccountId);

                if (stmtUnblockPlayer->executeUpdate() == 0)
                {
                    ::Utils::Logger::log("Error executing query", Utils::LogType::Warning, "PersistentDatabase::unblockPlayer");
                    return;
                }
            }
            catch (const sql::SQLException& e)
            {
                ::Utils::Logger::log("MariaDB exception: " + std::string(e.what()), Utils::LogType::Error, "PersistentDatabase::unblockPlayer");
                return;
            }
        }

        Main::Enums::AddFriendServerExtra PersistentDatabase::addPendingFriendRequest(std::uint32_t aid, const char* targetName)
        {
            try
            {
                TransactionGuard tx(m_transactionalCon.get());

                std::uint32_t targetAid = 0;

                std::unique_ptr<sql::PreparedStatement> stmtFindPlayer(
                    m_transactionalCon->prepareStatement("SELECT AccountID FROM Users WHERE Nickname = ? FOR UPDATE"));
                stmtFindPlayer->setString(1, targetName);

                std::unique_ptr<sql::ResultSet> resFindPlayer(stmtFindPlayer->executeQuery());

                if (!resFindPlayer->next())
                    return Main::Enums::AddFriendServerExtra::TARGET_NOT_FOUND;

                targetAid = static_cast<std::uint32_t>(
                    resFindPlayer->getUInt("AccountID"));

                if (targetAid == aid)
                    return Main::Enums::AddFriendServerExtra::TARGET_NOT_FOUND;

                std::unique_ptr<sql::PreparedStatement> stmtFriendCount(
                    m_transactionalCon->prepareStatement("SELECT COUNT(*) AS FriendCount FROM Friendlist WHERE AccountID = ? FOR UPDATE"));
                stmtFriendCount->setUInt(1, targetAid);

                std::unique_ptr<sql::ResultSet> resFriendCount(stmtFriendCount->executeQuery());

                if (!resFriendCount->next())
                    return Main::Enums::AddFriendServerExtra::DB_ERROR;

                if (resFriendCount->getUInt("FriendCount") >= Common::Constants::maxFriends)
                    return Main::Enums::AddFriendServerExtra::TARGET_OR_SENDER_FRIEND_LIST_FULL;

                std::unique_ptr<sql::PreparedStatement> stmtBlocked(
                    m_transactionalCon->prepareStatement("SELECT 1 FROM BlockedPlayers WHERE AccountID = ? AND TargetAccountID = ? FOR UPDATE"));
                stmtBlocked->setUInt(1, targetAid);
                stmtBlocked->setUInt(2, aid);

                std::unique_ptr<sql::ResultSet> resBlocked(stmtBlocked->executeQuery());

                if (resBlocked->next())
                    return Main::Enums::AddFriendServerExtra::RECEIVER_BLOCKED_SENDER;

                std::unique_ptr<sql::PreparedStatement> stmtInsert(
                    m_transactionalCon->prepareStatement("INSERT INTO PendingFriendRequests (AccountID, TargetAccountID) VALUES (?, ?)"));
                stmtInsert->setUInt(1, targetAid);
                stmtInsert->setUInt(2, aid);

                if (stmtInsert->executeUpdate() == 0)
                    return Main::Enums::AddFriendServerExtra::DB_ERROR;

                tx.commit();
                return Main::Enums::AddFriendServerExtra::REQUEST_SENT;
            }
            catch (const sql::SQLException& e)
            {
                ::Utils::Logger::log("MariaDB exception: " + std::string(e.what()),Utils::LogType::Error,"PersistentDatabase::addPendingFriendRequest");
                return Main::Enums::AddFriendServerExtra::DB_ERROR;
            }
        }

        std::vector<Main::Structures::Friend> PersistentDatabase::loadPendingFriendRequests(std::uint32_t accountID)
        {
            std::vector<Main::Structures::Friend> pendingFriendRequests;

            try
            {
                TransactionGuard tx(m_transactionalCon.get());

                std::unique_ptr<sql::PreparedStatement> stmt(
                    m_transactionalCon->prepareStatement("SELECT PendingFriendRequests.*, Users.Nickname AS TargetNickname "
                        "FROM PendingFriendRequests INNER JOIN Users ON PendingFriendRequests.TargetAccountID = Users.AccountID "
                        "WHERE PendingFriendRequests.AccountID = ? FOR UPDATE"));

                stmt->setUInt(1, accountID);
                std::unique_ptr<sql::ResultSet> res(stmt->executeQuery());
                Main::Structures::Friend ffriend{};

                while (res->next())
                {
                    std::memcpy(ffriend.targetNickname,res->getString("TargetNickname").c_str(),16);

                    ffriend.targetAccountId = res->getUInt("TargetAccountID");

                    pendingFriendRequests.push_back(ffriend);
                }

                if (!pendingFriendRequests.empty())
                {
                    std::unique_ptr<sql::PreparedStatement> stmtRemove(m_transactionalCon->prepareStatement("DELETE FROM PendingFriendRequests WHERE AccountID = ?"));

                    stmtRemove->setUInt(1, accountID);
                    stmtRemove->executeUpdate();
                }

                tx.commit();
            }
            catch (const sql::SQLException& e)
            {
                ::Utils::Logger::log("MariaDB exception: " + std::string(e.what()),Utils::LogType::Error,"PersistentDatabase::loadPendingFriendRequests");
            }

            return pendingFriendRequests;
        }

        void PersistentDatabase::updatePlayerLuckyPoints(std::uint32_t accountID, std::uint32_t luckyPoints)
        {
            try
            {
                std::string updateLevelQuery = "UPDATE Users SET LuckyPoints = ? WHERE AccountID = ?";
                std::unique_ptr<sql::PreparedStatement> stmt(m_con->prepareStatement(updateLevelQuery));

                stmt->setUInt(1, luckyPoints);
                stmt->setUInt(2, accountID);

                if (stmt->executeUpdate() != 1)
                {
                    ::Utils::Logger::log("Error executing query: " + updateLevelQuery, Utils::LogType::Warning, "PersistentDatabase::updatePlayerLuckyPoints");
                    return;
                }
            }
            catch (const sql::SQLException& e)
            {
                ::Utils::Logger::log("SQLException: " + std::string(e.what()), Utils::LogType::Error, "PersistentDatabase::updatePlayerLuckyPoints");
                return;
            }
        }

        std::optional<std::pair<std::string, std::string>> PersistentDatabase::getGradedHwid(std::uint32_t accountId)
        {
            try
            {
                std::string query = "SELECT HWIDGraded, HWIDGradedSalt FROM Users WHERE AccountID = ?";
                std::unique_ptr<sql::PreparedStatement> stmt(m_con->prepareStatement(query));
                stmt->setUInt(1, accountId);

                std::unique_ptr<sql::ResultSet> res(stmt->executeQuery());
                if (res->next())
                {
                    std::string hwid = res->getString("HWIDGraded").c_str();
                    std::string salt = res->getString("HWIDGradedSalt").c_str();
                    return std::make_pair(std::move(hwid), std::move(salt));
                }

                return std::nullopt;
            }
            catch (const sql::SQLException& e)
            {
                ::Utils::Logger::log("MariaDB exception in getGradedHwid: " + std::string(e.what()),::Utils::LogType::Error);
                return std::nullopt;
            }
        }

    } // end namespace Main
} // end namespace Persistence
