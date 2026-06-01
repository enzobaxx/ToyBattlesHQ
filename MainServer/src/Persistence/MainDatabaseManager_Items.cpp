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


        bool PersistentDatabase::insertItemLogs(std::uint32_t accountId, const std::vector<Main::Structures::ItemLogInfo>& logs)
        {
            if (logs.empty()) return true;

            try
            {
                TransactionGuard txn(m_transactionalCon.get());

                const std::string query = R"(INSERT INTO ItemLogs (AccountID, Date, ItemNumber, ItemID, Action, ExpirationDate) VALUES (?, NOW(), ?, ?, ?, ?))";

                std::unique_ptr<sql::PreparedStatement> stmt(m_transactionalCon->prepareStatement(query));

                for (const auto& log : logs)
                {
                    stmt->setUInt64(1, accountId);
                    stmt->setUInt(2, log.itemNumber);
                    stmt->setUInt64(3, log.itemId);
                    stmt->setString(4, log.action);
                    stmt->setUInt(5, log.expirationDate);
                    stmt->executeUpdate();
                }

                txn.commit();
                return true;
            }
            catch (const sql::SQLException& e)
            {
                ::Utils::Logger::log("MariaDB exception: " + std::string(e.what()),Utils::LogType::Error,"PersistentDatabase::insertItemLogs");
                return false;
            }
        }

        bool PersistentDatabase::insertItemLog(std::uint32_t accountId, const Main::Structures::ItemLogInfo& log)
        {
            try
            {
                const std::string query = R"(INSERT INTO ItemLogs (AccountID, Date, ItemNumber, ItemID, Action, ExpirationDate) VALUES (?, NOW(), ?, ?, ?, ?))";

                std::unique_ptr<sql::PreparedStatement> stmt(m_con->prepareStatement(query));
                stmt->setUInt64(1, accountId);
                stmt->setUInt(2, log.itemNumber);
                stmt->setUInt64(3, log.itemId);
                stmt->setString(4, log.action);
                stmt->setUInt(5, log.expirationDate);
                stmt->executeUpdate();

                return true;
            }
            catch (const sql::SQLException& e)
            {
                ::Utils::Logger::log("MariaDB exception: " + std::string(e.what()), Utils::LogType::Error, "PersistentDatabase::insertItemLog");
                return false;
            }

            return false;
        }

        bool PersistentDatabase::logBoughtItems(std::uint32_t accountId, const std::vector<Main::Structures::BoughtItem>& boughtItems, bool isCouponItems)
        {
            std::vector<Main::Structures::ItemLogInfo> logInfos;
            for (const auto& currentBoughtItem : boughtItems)
            {
                logInfos.emplace_back(Main::Structures::ItemLogInfo{ currentBoughtItem.serialInfo.itemNumber, 
                    currentBoughtItem.itemId.itemId, currentBoughtItem.unknown, 
                    isCouponItems ? "The item was bought from the coupon shop" : "The item was bought from the normal shop" });
            }

            return insertItemLogs(accountId, logInfos);
        }

        bool PersistentDatabase::logBoxItems(std::uint32_t accountId, const std::vector<Main::Structures::BoxItem>& boxItem)
        {
            std::vector<Main::Structures::ItemLogInfo> logInfos;
            for (const auto& currentBoxItem : boxItem)
            {
                logInfos.emplace_back(Main::Structures::ItemLogInfo{ currentBoxItem.serialInfo.itemNumber,
                    currentBoxItem.itemId.itemId, currentBoxItem.expirationDate,
                    "The item was won through a box" });
            }

            return insertItemLogs(accountId, logInfos);
        }

        bool PersistentDatabase::logProlongedItems(std::uint32_t accountId, const std::vector<Main::Structures::BoughtItemToProlong>& boughtItems,
            const std::vector<std::uint64_t>& itemDurations)
        {
            // Need to use 32bits here due to the game using that .....
            const std::uint32_t timeNow = static_cast<std::uint32_t>(
                std::chrono::duration_cast<std::chrono::seconds>(std::chrono::system_clock::now().time_since_epoch()).count());

            std::vector<Main::Structures::ItemLogInfo> logInfos;
            for (std::uint32_t i = 0; const auto & currentBoughtItem : boughtItems)
            {
                std::uint32_t actualItemDuration = itemDurations[i] <= 3 ? itemDurations[i] : itemDurations[i] + timeNow;
                logInfos.emplace_back(Main::Structures::ItemLogInfo{currentBoughtItem.serialInfo.itemNumber, 0, actualItemDuration, 
                    "The item was prolonged after it expired" });
                ++i;
            }

            return insertItemLogs(accountId, logInfos);
        }

        auto PersistentDatabase::getPlayerItems(std::uint32_t playerID)
            -> std::pair<std::vector<Main::Structures::Item>, std::unordered_map<std::uint16_t, std::vector<Main::Structures::EquippedItem>>>
        {
            std::vector<Main::Structures::Item> nonEquippedItems;
            std::unordered_map<std::uint16_t, std::vector<Main::Structures::EquippedItem>> equippedItemsPerCharacter;
            std::vector<std::pair<std::uint64_t, std::uint64_t>> itemNumbersToUpdate;
            std::uint64_t itemNum = 0;

            try
            {
                TransactionGuard tx(m_transactionalCon.get());

                const std::string queryStr = "SELECT * FROM UserItems WHERE AccountID = ?";
                std::unique_ptr<sql::PreparedStatement> stmt(m_transactionalCon->prepareStatement(queryStr));
                stmt->setUInt(1, playerID);
                std::unique_ptr<sql::ResultSet> resultSet(stmt->executeQuery());

                nonEquippedItems.reserve(1000);
                equippedItemsPerCharacter.reserve(500);

                while (resultSet->next())
                {
                    Main::Structures::Item item{ static_cast<std::uint32_t>(resultSet->getInt("ItemID")) };
                    auto rowId = static_cast<std::uint64_t>(resultSet->getUInt64("rowid"));
                    item.serialInfo.itemOrigin = static_cast<std::uint64_t>(resultSet->getInt64("ItemOrigin"));
                    item.serialInfo.m_serverId = static_cast<std::uint64_t>(resultSet->getInt64("acquisitionServerId"));
                    item.serialInfo.itemCreationDate = static_cast<time32_t>(resultSet->getInt64("creationDate"));

                    item.serialInfo.itemNumber = ++itemNum;
                    itemNumbersToUpdate.emplace_back(std::pair{ rowId, static_cast<unsigned long>(item.serialInfo.itemNumber) });

                    const std::uint64_t itemDuration_s = static_cast<std::uint64_t>(resultSet->getInt64("ItemDuration"));
                    item.expirationDate = (itemDuration_s <= 2)
                        ? static_cast<time32_t>(itemDuration_s)
                        : static_cast<time32_t>(item.serialInfo.itemCreationDate + itemDuration_s);

                    item.durability = static_cast<std::uint16_t>(resultSet->getInt("durability"));
                    item.energy = static_cast<std::uint16_t>(resultSet->getInt("energy"));
                    item.isSealed = static_cast<std::uint32_t>(resultSet->getInt("isSealed"));
                    item.sealLevel = static_cast<std::uint32_t>(resultSet->getInt("sealLevel"));
                    item.experienceEnhancement = static_cast<std::uint32_t>(resultSet->getInt("expEnhancement"));
                    item.mpEnhancement = static_cast<std::uint32_t>(resultSet->getInt("mpEnhancement"));
                    item.unknown = static_cast<std::uint32_t>(resultSet->getInt("IsCoupon"));
                    item.itemId.stock = static_cast<std::uint32_t>(resultSet->getInt("Stocks"));

                    if (resultSet->getInt("IsEquipped") == 1)
                    {
                        Main::Structures::EquippedItem equippedItem{ item };
                        const auto characterId = static_cast<std::uint16_t>(resultSet->getInt("CharacterID"));

                        if ((equippedItem.type >= 0 && equippedItem.type <= 17) || equippedItem.type == 19 || equippedItem.type == 20 /* diorama and scaffold */)
                        {
                            auto& equippedList = equippedItemsPerCharacter[characterId];
                            auto duplicateIt = std::find_if(equippedList.begin(), equippedList.end(),
                                [&](const Main::Structures::EquippedItem& existing) {
                                    return existing.type == equippedItem.type;
                                });

                            if (duplicateIt == equippedList.end())
                            {
                                equippedList.push_back(equippedItem);
                            }
                            else
                            {
                                Utils::Logger::log("Logic warning: Player with AID " + std::to_string(playerID) +
                                    " has multiple equipped items of type " + std::to_string(equippedItem.type) +
                                    " for CharacterID " + std::to_string(characterId) + ". Auto-fixing by unequipping one.",
                                    Utils::LogType::Warning, "PersistentDatabase::getPlayerItems");

                                nonEquippedItems.push_back(std::move(item));

                                const std::string updateQuery = "UPDATE UserItems SET IsEquipped = 0 WHERE rowid = ?";
                                std::unique_ptr<sql::PreparedStatement> updateStmt(m_transactionalCon->prepareStatement(updateQuery));
                                updateStmt->setUInt64(1, rowId);
                                updateStmt->executeUpdate();
                            }
                        }
                        else
                        {
                            Utils::Logger::log("Logic error: Player with AID " + std::to_string(playerID) +
                                " got an equipped item that is not allowed (itemId: " + std::to_string((uint32_t)equippedItem.id) + ", type: " + std::to_string((uint32_t)equippedItem.type)
                                + ")", Utils::LogType::Error, "PersistentDatabase::getPlayerItems");

                            nonEquippedItems.push_back(std::move(item));

                            const std::string updateQuery = "UPDATE UserItems SET IsEquipped = 0 WHERE rowid = ?";
                            std::unique_ptr<sql::PreparedStatement> updateStmt(m_transactionalCon->prepareStatement(updateQuery));
                            updateStmt->setUInt64(1, rowId);
                            updateStmt->executeUpdate();
                        }
                    }
                    else
                    {
                        nonEquippedItems.push_back(std::move(item));
                    }
                }

                if (!itemNumbersToUpdate.empty())
                {
                    std::string updateItemNumbersQuery = "UPDATE UserItems SET ItemNumber = ? WHERE rowid = ?";
                    std::unique_ptr<sql::PreparedStatement> updateStmt(m_transactionalCon->prepareStatement(updateItemNumbersQuery));

                    for (const auto& [rowId, itemNumber] : itemNumbersToUpdate)
                    {
                        updateStmt->setInt64(1, itemNumber);
                        updateStmt->setUInt64(2, rowId);
                        updateStmt->executeUpdate();
                        updateStmt->clearParameters();
                    }
                }

                tx.commit();
            }
            catch (const sql::SQLException& e)
            {
                ::Utils::Logger::log("MariaDB exception: " + std::string(e.what()), Utils::LogType::Error, "PersistentDatabase::getPlayerItems");
            }

            return std::pair{ std::move(nonEquippedItems), std::move(equippedItemsPerCharacter) };
        }

        bool PersistentDatabase::replaceItem(std::uint32_t accountID, std::uint64_t itemNumber, std::uint32_t newItemId)
        {
            try
            {
                const std::string queryStr = "UPDATE UserItems SET ItemID = ?, energy = 0 WHERE AccountID = ? AND ItemNumber = ?";

                std::unique_ptr<sql::PreparedStatement> stmt(m_con->prepareStatement(queryStr));
                stmt->setUInt(1, newItemId);
                stmt->setUInt(2, accountID);
                stmt->setUInt64(3, itemNumber); 

                if (stmt->executeUpdate() == 0)
                {
                    ::Utils::Logger::log("replaceItem: No rows affected. Possible reasons: item not found or already has the same ItemID. "
                        "AccountID: " + std::to_string(accountID) + ", ItemNumber: " + std::to_string(itemNumber) + ", NewItemID: " + std::to_string(newItemId),
                        Utils::LogType::Warning, "PersistentDatabase::replaceItem"); 
                    return false;
                }
                return true;
            }
            catch (const sql::SQLException& e)
            {
                ::Utils::Logger::log("MariaDB exception: " + std::string(e.what()) + " | AccountID: " + std::to_string(accountID) +
                    ", ItemNumber: " + std::to_string(itemNumber) + ", NewItemID: " + std::to_string(newItemId), 
                    Utils::LogType::Error, "PersistentDatabase::replaceItem");  
                return false;
            }
        }

        bool PersistentDatabase::replaceItemResetEnergy(std::uint32_t accountID, std::uint64_t itemNumber, std::uint32_t newItemId)
        {
            try
            {
                const std::string queryStr = "UPDATE UserItems SET ItemID = ?, energy = 0 WHERE AccountID = ? AND ItemNumber = ?";

                std::unique_ptr<sql::PreparedStatement> stmt(m_con->prepareStatement(queryStr));
                stmt->setUInt(1, newItemId);
                stmt->setUInt(2, accountID);
                stmt->setUInt64(3, itemNumber);

                if (stmt->executeUpdate() == 0)
                {
                    ::Utils::Logger::log("replaceItemResetEnergy: No rows affected. Possible reasons: item not found or already has the same ItemID. "
                        "AccountID: " + std::to_string(accountID) +
                        ", ItemNumber: " + std::to_string(itemNumber) +
                        ", NewItemID: " + std::to_string(newItemId),
                        Utils::LogType::Warning, "PersistentDatabase::replaceItemResetEnergy");
                    return false;
                }
                return true;
            }
            catch (const sql::SQLException& e)
            {
                ::Utils::Logger::log("MariaDB exception: " + std::string(e.what()) +
                    " | AccountID: " + std::to_string(accountID) +
                    ", ItemNumber: " + std::to_string(itemNumber) +
                    ", NewItemID: " + std::to_string(newItemId),
                    Utils::LogType::Error, "PersistentDatabase::replaceItemResetEnergy");
                return false;
            }
        }

        bool PersistentDatabase::addPlayerItems(std::uint32_t accountID, const std::vector<Item>& items, std::uint32_t latestCharacterSelected)
        {
            try
            {
                TransactionGuard tx(m_transactionalCon.get());

                const std::string queryStr =
                    "INSERT INTO UserItems (AccountID, IsEquipped, CharacterID, ItemID, ItemDuration, ItemNumber, ItemOrigin, acquisitionServerId, creationDate,"
                    " durability, energy, isSealed, sealLevel, expEnhancement, mpEnhancement, IsCoupon, Stocks) "
                    "VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?)";

                std::unique_ptr<sql::PreparedStatement> stmt(m_transactionalCon->prepareStatement(queryStr));
                for (const auto& item : items)
                {
                    stmt->setUInt(1, accountID);
                    stmt->setUInt(2, latestCharacterSelected == -1 ? 0 : 1);
                    stmt->setUInt(3, latestCharacterSelected == -1 ? 0 : latestCharacterSelected);
                    stmt->setUInt(4, item.itemId.itemId);
                    stmt->setInt64(5, static_cast<std::int64_t>(item.expirationDate <= 3 ? item.expirationDate : item.expirationDate - item.serialInfo.itemCreationDate));
                    stmt->setInt64(6, static_cast<std::int64_t>(item.serialInfo.itemNumber));
                    stmt->setInt64(7, static_cast<std::int64_t>(item.serialInfo.itemOrigin));
                    stmt->setInt64(8, static_cast<std::int64_t>(item.serialInfo.m_serverId));
                    stmt->setInt64(9, static_cast<std::int64_t>(item.serialInfo.itemCreationDate));
                    stmt->setUInt(10, static_cast<std::uint32_t>(item.durability));
                    stmt->setUInt(11, static_cast<std::uint32_t>(item.energy));
                    stmt->setUInt(12, 0); // isSealed
                    stmt->setUInt(13, 0); // sealLevel
                    stmt->setUInt(14, 0); // expEnhancement
                    stmt->setUInt(15, 0); // mpEnhancement
                    stmt->setUInt(16, 0); // unknown
                    stmt->setUInt(17, item.itemId.stock);
                    stmt->executeUpdate();
                }
                tx.commit();
                return true;
            }
            catch (const sql::SQLException& e)
            {
                ::Utils::Logger::log("MariaDB exception: " + std::string(e.what()), Utils::LogType::Error, "PersistentDatabase::addPlayerItems");
                return false;
            }
        }

        bool PersistentDatabase::updateItemStock(std::uint32_t accountID, std::uint64_t itemNumber, std::uint32_t newStock)
        {
            try
            {
                const std::string queryStr = "UPDATE UserItems SET Stocks = ? WHERE AccountID = ? AND ItemNumber = ?";
                std::unique_ptr<sql::PreparedStatement> stmt(m_con->prepareStatement(queryStr));
                stmt->setUInt(1, newStock);     
                stmt->setUInt(2, accountID);      
                stmt->setInt64(3, static_cast<std::int64_t>(itemNumber)); 
                stmt->executeUpdate();
                return true;
            }
            catch (const sql::SQLException& e)
            {
                ::Utils::Logger::log("MariaDB exception: " + std::string(e.what()), Utils::LogType::Error, "PersistentDatabase::updateItemStock");
                return false;
            }
        }

        void PersistentDatabase::prolongItems(std::uint32_t accountID, const std::vector<Main::Structures::BoughtItemToProlong>& toProlongItems,
            const std::vector<std::uint64_t>& itemDurations, std::uint32_t timeNow)
        {
            try
            {
                TransactionGuard tg(m_transactionalCon.get());

                const std::string prolongItemQuery = "UPDATE UserItems SET ItemDuration = ?, ItemOrigin = ?, acquisitionServerId = ? "
                    "WHERE AccountID = ? AND ItemNumber = ?";

                std::unique_ptr<sql::PreparedStatement> stmt(m_transactionalCon->prepareStatement(prolongItemQuery));

                for (std::size_t i = 0; i < toProlongItems.size(); ++i)
                {
                    const auto& itemToProlong = toProlongItems[i];
                    stmt->setInt64(1, static_cast<std::int64_t>(itemDurations[i] <= 3 ? itemDurations[i] : itemDurations[i] + timeNow));
                    stmt->setInt64(2, static_cast<std::int64_t>(itemToProlong.serialInfo.itemOrigin));
                    stmt->setInt64(3, static_cast<std::int64_t>(itemToProlong.serialInfo.m_serverId));
                    stmt->setInt64(4, static_cast<std::int64_t>(accountID));
                    stmt->setInt(5, static_cast<std::int32_t>(itemToProlong.serialInfo.itemNumber));

                    if (stmt->executeUpdate() == 0)
                    {
                        ::Utils::Logger::log("Failed to update item with ItemNumber: " + std::to_string(itemToProlong.serialInfo.itemNumber) +
                            " for AccountID: " + std::to_string(accountID), Utils::LogType::Warning, "PersistentDatabase::prolongItems");

                        return; 
                    }
                }

                tg.commit(); 
            }
            catch (const sql::SQLException& e)
            {
                ::Utils::Logger::log("MariaDB exception: " + std::string(e.what()), Utils::LogType::Error, "PersistentDatabase::prolongItems");
                return;
            }
        }

        bool PersistentDatabase::removePlayerItem(std::uint32_t accountId, std::uint64_t itemNumber, const std::string& caller)
        {
            try
            {
                const std::string deleteItem = "DELETE FROM UserItems WHERE AccountID = ? AND ItemNumber = ?";
                std::unique_ptr<sql::PreparedStatement> stmt(m_con->prepareStatement(deleteItem));

                stmt->setUInt(1, accountId);
                stmt->setUInt64(2, itemNumber);

                if (stmt->executeUpdate() == 0)
                {
                    ::Utils::Logger::log("[CALLER: " + caller + "] Warning: No rows affected by query for AccountID: " + std::to_string(accountId) +
                        " and ItemNumber: " + std::to_string(itemNumber), Utils::LogType::Warning, "PersistentDatabase::removePlayerItem");
                    return false;
                }

                return true;
            }
            catch (const sql::SQLException& e)
            {
                ::Utils::Logger::log("[Main::Database::removePlayerItem] MariaDB exception: " + std::string(e.what()) +
                    " | AccountID: " + std::to_string(accountId) +
                    ", ItemNumber: " + std::to_string(itemNumber),
                    Utils::LogType::Error, "PersistentDatabase::removePlayerItem");
                return false;
            }
        }

        bool PersistentDatabase::removeAllPlayerItems(const std::string& nickname, std::uint32_t executorGrade)
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
                const std::string deleteItems = "DELETE FROM UserItems WHERE AccountID = ?";
                std::unique_ptr<sql::PreparedStatement> stmt(m_con->prepareStatement(deleteItems));
                stmt->setUInt(1, accountId);
                stmt->executeUpdate();
                return true;
            }
            catch (const sql::SQLException& e)
            {
                ::Utils::Logger::log("[Main::Database::removeAllPlayerItems] MariaDB exception: " + std::string(e.what()) +
                    " | Nickname: " + nickname,
                    Utils::LogType::Error, "PersistentDatabase::removeAllPlayerItems");
                return false;
            }
        }

        void PersistentDatabase::reduceDurability(std::uint32_t accountId, const std::vector<std::pair<std::uint32_t, std::uint64_t>>& equippedItems)
        {
            try
            {
                TransactionGuard tg(m_transactionalCon.get());

                std::string updateQueryStr = "UPDATE UserItems SET Durability = ? WHERE AccountID = ? AND ItemNumber = ? AND ItemDuration = 0";
                std::unique_ptr<sql::PreparedStatement> stmt(m_transactionalCon->prepareStatement(updateQueryStr));

                for (const auto& currentEquippedItem : equippedItems)
                {
                    const auto currentItemBaseDurability = Main::CdbUtils::getItemDurability(currentEquippedItem.first);

                    if (!currentItemBaseDurability || *currentItemBaseDurability == 0)
                        continue;

                    const std::uint32_t reduction = (*currentItemBaseDurability / 100) * 1;

                    const std::uint32_t newDurability = (*currentItemBaseDurability > reduction)
                        ? (*currentItemBaseDurability - reduction)
                        : 0;

                    stmt->setUInt(1, newDurability);
                    stmt->setUInt(2, accountId);
                    stmt->setUInt(3, currentEquippedItem.second);
                    stmt->executeUpdate();
                }

                tg.commit();
            }
            catch (const sql::SQLException& e)
            {
                ::Utils::Logger::log(std::string("SQL Error: ") + e.what(), Utils::LogType::Error, "PersistentDatabase::reduceDurability");
                return;
            }
        }

        void PersistentDatabase::updateItemDurability(std::uint32_t accountId, std::uint32_t itemNumber, std::uint32_t newDurability)
        {
            try
            {
                const std::string updateQueryStr =
                    "UPDATE UserItems SET Durability = ? WHERE AccountID = ? AND ItemNumber = ?";

                std::unique_ptr<sql::PreparedStatement> stmt(m_con->prepareStatement(updateQueryStr));

                stmt->setUInt(1, newDurability);
                stmt->setUInt(2, accountId);
                stmt->setUInt(3, itemNumber);
                stmt->executeUpdate();
            }
            catch (const sql::SQLException& e)
            {
                ::Utils::Logger::log(std::string("SQL Error: ") + e.what(), Utils::LogType::Error, "PersistentDatabase::updateItemDurability");
                return;
            }
        }

        void PersistentDatabase::switchItemEquip(std::uint32_t accountID, std::uint64_t itemNumber, std::uint32_t characterId)
        {
            try
            {
                std::string queryStr = "UPDATE UserItems SET IsEquipped = CASE WHEN IsEquipped = 0 THEN 1 ELSE 0 END, "
                    " CharacterID = ? WHERE AccountID = ? AND ItemNumber = ?";
                std::unique_ptr<sql::PreparedStatement> stmt(m_con->prepareStatement(queryStr));

                stmt->setUInt(1, characterId);
                stmt->setUInt(2, accountID);
                stmt->setUInt64(3, itemNumber);

                if (stmt->executeUpdate() == 0)
                {
                    ::Utils::Logger::log("Update failed.", Utils::LogType::Warning, "PersistentDatabase::switchItemEquip");
                    return;
                }
            }
            catch (const sql::SQLException& e)
            {
                ::Utils::Logger::log("MariaDB exception: " + std::string(e.what()), Utils::LogType::Error, "PersistentDatabase::switchItemEquip");
                return;
            }
        }

        void PersistentDatabase::unequipItem(std::uint32_t accountID, std::uint64_t unequipItemNumber)
        {
            try
            {
                std::string queryStr = "UPDATE UserItems SET IsEquipped = 0, CharacterID = 0 WHERE AccountID = ? AND ItemNumber = ?";
                std::unique_ptr<sql::PreparedStatement> stmt(m_con->prepareStatement(queryStr));

                stmt->setUInt(1, accountID);
                stmt->setUInt64(2, unequipItemNumber);

                if (stmt->executeUpdate() == 0)
                {
                    ::Utils::Logger::log("Update failed", Utils::LogType::Warning, "PersistentDatabase::unequipItem");
                    return;
                }
            }
            catch (const sql::SQLException& e)
            {
                ::Utils::Logger::log("MariaDB exception: " + std::string(e.what()), Utils::LogType::Error, "PersistentDatabase::unequipItem");
                return;
            }
        }

        void PersistentDatabase::equipItem(std::uint32_t accountID, std::uint64_t equipItemNumber, std::uint16_t characterId)
        {
            try
            {
                std::string queryStr = "UPDATE UserItems SET IsEquipped = 1, CharacterID = ? WHERE AccountID = ? AND ItemNumber = ?";
                std::unique_ptr<sql::PreparedStatement> stmt(m_con->prepareStatement(queryStr));

                stmt->setUInt(1, characterId);
                stmt->setUInt(2, accountID);
                stmt->setUInt64(3, equipItemNumber);

                if (stmt->executeUpdate() == 0)
                {
                    ::Utils::Logger::log("Update failed", Utils::LogType::Warning, "PersistentDatabase::equipItem");
                    return;
                }
            }
            catch (const sql::SQLException& e)
            {
                ::Utils::Logger::log("MariaDB exception: " + std::string(e.what()), Utils::LogType::Error, "PersistentDatabase::equipItem");
                return;
            }
        }

        void PersistentDatabase::swapItems(std::uint32_t accountID, std::uint64_t toUnequipItemNumber, std::uint64_t toEquipItemNumber, std::uint16_t characterId)
        {
            try
            {
                TransactionGuard tg(m_transactionalCon.get());

                std::string queryStr1 = "UPDATE UserItems SET IsEquipped = 1, CharacterID = ? WHERE AccountID = ? AND ItemNumber = ?";
                std::unique_ptr<sql::PreparedStatement> stmt1(m_transactionalCon->prepareStatement(queryStr1));
                stmt1->setUInt(1, characterId);
                stmt1->setUInt(2, accountID);
                stmt1->setUInt64(3, toEquipItemNumber);

                std::string queryStr2 = "UPDATE UserItems SET IsEquipped = 0, CharacterID = 0 WHERE AccountID = ? AND ItemNumber = ?";
                std::unique_ptr<sql::PreparedStatement> stmt2(m_transactionalCon->prepareStatement(queryStr2));
                stmt2->setUInt(1, accountID);
                stmt2->setUInt64(2, toUnequipItemNumber);

                if (stmt1->executeUpdate() == 0)
                {
                    return;
                }

                if (stmt2->executeUpdate() == 0)
                {
                    return;
                }

                tg.commit();
            }
            catch (const sql::SQLException& e)
            {
                ::Utils::Logger::log("MariaDB exception: " + std::string(e.what()), Utils::LogType::Error, "PersistentDatabase::swapItems");
                return;
            }
        }

        void PersistentDatabase::addPlayerItem(const Item& item, std::uint32_t accountID, std::uint32_t latestCharacterSelected)
        {
            addPlayerItems(accountID, std::vector<Item>{ item }, latestCharacterSelected);
        }

        void PersistentDatabase::insertEnergyToItem(std::uint32_t accountID, std::uint64_t itemNumber, std::uint32_t newItemEnergy, std::uint32_t newTotalEnergy)
        {
            try
            {
                TransactionGuard tx(m_transactionalCon.get());

                const std::string setEnergyForItem = "UPDATE UserItems SET energy = ? WHERE AccountID = ? AND ItemNumber = ?";
                std::unique_ptr<sql::PreparedStatement> stmt1(m_transactionalCon->prepareStatement(setEnergyForItem));
                stmt1->setUInt(1, newItemEnergy);
                stmt1->setUInt(2, accountID);
                stmt1->setUInt64(3, itemNumber);

                const std::string removeEnergyFromTotalEnergy = "UPDATE Users SET Battery = ? WHERE AccountID = ?";
                std::unique_ptr<sql::PreparedStatement> stmt2(m_transactionalCon->prepareStatement(removeEnergyFromTotalEnergy));
                stmt2->setUInt(1, newTotalEnergy);
                stmt2->setUInt(2, accountID);

                if (stmt1->executeUpdate() != 1 || stmt2->executeUpdate() != 1)
                {
                    ::Utils::Logger::log("Error executing queries for energy-insertion", Utils::LogType::Warning, "PersistentDatabase::insertEnergyToItem");
                    return;
                }

                Main::Structures::ItemLogInfo logInfo{ itemNumber, 0, 0, "The user added energy to this item. New item energy is " + std::to_string(newItemEnergy) };
                insertItemLog(accountID, logInfo);

                tx.commit();
            }
            catch (const sql::SQLException& e)
            {
                ::Utils::Logger::log("SQLException: " + std::string(e.what()), Utils::LogType::Error, "PersistentDatabase::insertEnergyToItem");
                return;
            }
        }

        [[nodiscard]] bool PersistentDatabase::isItemTradeable(std::uint32_t accountId, std::uint32_t itemNumber)
        {
            try
            {
                std::unique_ptr<sql::PreparedStatement> stmt(m_con->prepareStatement(
                    "SELECT IsTradeable FROM UserItems WHERE AccountID = ? AND ItemNumber = ?"));

                stmt->setUInt(1, accountId);
                stmt->setUInt(2, itemNumber);

                std::unique_ptr<sql::ResultSet> result(stmt->executeQuery());

                if (result->next())
                {
                    return result->getInt("IsTradeable") == 1;
                }

                return false; 
            }
            catch (const sql::SQLException& e)
            {
                ::Utils::Logger::log("MariaDB exception in isItemTradeable: " + std::string(e.what()),
                    Utils::LogType::Error, "PersistentDatabase::isItemTradeable");
                return false;
            }
        }

    } // end namespace Main
} // end namespace Persistence
