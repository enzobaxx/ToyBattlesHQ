#ifndef CDB_UTILITY_H
#define CDB_UTILITY_H

#include <cstdint>
#include <optional>
#include <array>
#include <functional>
#include <chrono>

#include "MainEnums.h"
#include "ConstantDatabase/CdbSingleton.h"
#include "ConstantDatabase/Structures/CdbItemInfo.h"
#include "ConstantDatabase/Structures/CdbWeaponsInfo.h"
#include "ConstantDatabase/Structures/CdbUpgradeInfo.h"
#include "ConstantDatabase/Structures/CdbCapsuleInfo.h"
#include "ConstantDatabase/Structures/CdbCapsulePackageInfo.h"
#include "ConstantDatabase/Structures/CdbMapInfo.h"
#include "Structures/Capsule/CapsuleList.h"
#include "ConstantDatabase/Structures/CdbRewardInfo.h"
#include "ConstantDatabase/Structures/CdbGradeInfo.h"
#include "ConstantDatabase/Structures/CdbVendor.h"
#include "ConstantDatabase/Structures/CdbEffectInfo.h"
#include <random>
#include "Enums/RoomEnums.h"
#include <ConstantDatabase/Structures/CdbMissionEventInfo.h>
#include "Utils/SetupParser.h"

namespace Main
{
	namespace CdbUtils
	{
		template<typename T>
		using Cdb = Common::ConstantDatabase::Cdb<T>;

		using cdbItemWeapons = Common::ConstantDatabase::CdbSingleton<Common::ConstantDatabase::CdbItemWeapon>;
		using cdbUpgrades = Common::ConstantDatabase::CdbSingleton<Common::ConstantDatabase::CdbUpgradeInfo>;
		using cdbCapsuleInfos = Common::ConstantDatabase::CdbSingleton<Common::ConstantDatabase::CdbCapsuleInfo>;
		using cdbCapsulePackageInfos = Common::ConstantDatabase::CdbSingleton<Common::ConstantDatabase::CdbCapsulePackageInfo>;
		using cdbMapInfo = Common::ConstantDatabase::CdbSingleton<Common::ConstantDatabase::CdbMapInfo>;
		using cdbRewardInfo = Common::ConstantDatabase::CdbSingleton<Common::ConstantDatabase::CdbRewardInfo>;
		using ItemTypePricePair = std::pair<Main::Enums::ItemCurrencyType, std::uint32_t>;
		using cdbLevelInfo = Common::ConstantDatabase::CdbSingleton<Common::ConstantDatabase::CdbGradeInfo>;
		using itemPackageInfo = Common::ConstantDatabase::CdbSingleton<Common::ConstantDatabase::CdbItemPackageInfo>;
		using VendorInfo = Common::ConstantDatabase::CdbSingleton<Common::ConstantDatabase::CdbVendorInfo>;
		using EffectInfo = Common::ConstantDatabase::CdbSingleton<Common::ConstantDatabase::CdbEffectInfo>;

		inline std::uint32_t getRandomMapForMode(std::uint32_t modeId)
		{
			std::vector<std::uint32_t> retrievedMapIdsForMode;
			for (const auto& [entryId, mapInfo] : cdbMapInfo::getInstance().getEntries()) 
			{
				if ((modeId == Common::Enums::TeamDeathMatch || modeId == Common::Enums::Clan_TeamDeathMatch) && mapInfo.mi_mod_tdm
					|| modeId == Common::Enums::FreeForAll && mapInfo.mi_mod_ffa
					|| modeId == Common::Enums::ItemMatch && mapInfo.mi_mod_itm
					|| (modeId == Common::Enums::CaptureTheBattery || modeId == Common::Enums::Clan_CaptureTheBattery) && mapInfo.mi_mod_ctf
					|| modeId == Common::Enums::CloseCombat && mapInfo.mi_mod_ctm
					|| (modeId == Common::Enums::Elimination || modeId == Common::Enums::Clan_Elimination) && mapInfo.mi_mod_sab
					|| modeId == Common::Enums::SuperItemMatch && mapInfo.mi_mod_cim
					|| modeId == Common::Enums::ZombieMode && mapInfo.mi_mod_zsm
					|| modeId == Common::Enums::ArmsRace && mapInfo.mi_mod_grm
					|| modeId == Common::Enums::Scrimmage && mapInfo.mi_mod_grm
					|| (modeId == Common::Enums::BombBattle || modeId == Common::Enums::Clan_BombBattle) && mapInfo.mi_mod_bmb
					|| modeId == Common::Enums::SniperMode && mapInfo.mi_mod_sni
					|| modeId == Common::Enums::SquareMode && mapInfo.mi_mod_nod
					|| modeId == Common::Enums::BossBattle && mapInfo.mi_mod_pve
					|| modeId == Common::Enums::AiBattle && mapInfo.mi_mod_bot)
				{
					retrievedMapIdsForMode.push_back(mapInfo.mi_id);
				}
			}

			if (retrievedMapIdsForMode.empty())
			{
				return 6; // default map: bitmap
			}

			std::random_device rd;
			std::mt19937 gen(rd());
			std::uniform_int_distribution<std::size_t> dist(0, retrievedMapIdsForMode.size() - 1);

			return retrievedMapIdsForMode[dist(gen)];
		}

		inline std::optional<ItemTypePricePair> getItemPrice(std::uint32_t itemId)
		{
			if (const auto entry = cdbItemWeapons::getInstance().getEntry(itemId); entry)
			{
				if (entry->ii_buy_coupon)
					return std::pair{ Main::Enums::ItemCurrencyType::ITEM_COUPON, entry->ii_buy_coupon };
				else if (entry->ii_buy_cash)
					return std::pair{ Main::Enums::ItemCurrencyType::ITEM_RT, entry->ii_buy_cash };
				else if (entry->ii_buy_point)
					return std::pair{ Main::Enums::ItemCurrencyType::ITEM_MP, entry->ii_buy_point };
			}
			return std::nullopt;
		}

		// ei_key == three ==> HP
		// ei_key == thirteen ==> Speed.
		inline std::pair<std::uint32_t, std::uint32_t> getExpAndMpEnhancementFor(std::uint32_t itemId)
		{
			std::pair<std::uint32_t, std::uint32_t> totalExpMpPercentageToAdd{};

			if (const auto entry = cdbItemWeapons::getInstance().getEntry(itemId); entry)
			{
				const std::array<std::uint32_t, 3> effects = { entry->ii_effect_1, entry->ii_effect_2, entry->ii_effect_3 };
				for (auto effectId : effects)
				{
					if (const auto effect = EffectInfo::getInstance().getEntry(effectId))
					{
						if (effect->ei_valueA >= 10 && effect->ei_valueA <= 1000)
						{
							if (effect->ei_key == 122) // exp
							{
								totalExpMpPercentageToAdd.first += effect->ei_valueA / 10;
							}
							else if (effect->ei_key == 123) // mp
							{
								totalExpMpPercentageToAdd.second += effect->ei_valueA / 10;
							}
						}
					}
				}
			}

			return totalExpMpPercentageToAdd;
		}

		inline bool isNoOptionItem(std::uint32_t itemId)
		{
			if (const auto entry = cdbItemWeapons::getInstance().getEntry(itemId); entry)
			{
				std::string optionStr(entry->ii_option.data(), strnlen(entry->ii_option.data(), entry->ii_option.size()));
				return optionStr.find("No option") != std::string::npos;
			}
			return false;
		}

		inline std::optional<std::uint16_t> getItemDurability(std::uint32_t itemId)
		{
			if (const auto entry = cdbItemWeapons::getInstance().getEntry(itemId); entry)
				return entry->ii_durable_value;
			return std::nullopt;
		}

		inline std::optional<std::uint16_t> getItemStocks(std::uint32_t itemId)
		{
			if (const auto entry = cdbItemWeapons::getInstance().getEntry(itemId); entry)
				return entry->ii_stocks;
			return std::nullopt;
		}

		inline std::optional<std::string> getItemName(std::uint32_t itemId)
		{
			if (const auto entry = cdbItemWeapons::getInstance().getEntry(itemId); entry)
				return std::string{ entry->ii_name.begin(), entry->ii_name.end() };
			return std::nullopt;
		}

		inline std::optional<bool> isImmediatelySet(std::uint32_t itemId)
		{
			if (const auto entry = cdbItemWeapons::getInstance().getEntry(itemId); entry)
			{
				return entry->ii_immediately_set;
			}
			return std::nullopt;
		}

		/**
		* @return The duration of the given itemId. If the item is not found, returns 3, which signals that 
		* the item is expired to the game client so that it cannot be used further (e.g. prevent exploits)
		* Note: usually, the caller should do a sanity check such as: expirationDate = duration <= 3 ? duration : serialInfo.itemCreationDate + duration
		*/
		inline std::uint32_t getItemDuration(std::uint32_t itemId)
		{
			if (const auto entry = cdbItemWeapons::getInstance().getEntry(itemId); entry)
			{
				return entry->ii_limited_time;
			}
			return 3; // expired (TODO: remove magic number)
		}

		inline bool itemExists(std::uint32_t itemId)
		{
			return getItemDuration(itemId) != 3;
		}

		inline std::optional<std::uint32_t> getItemType(std::uint32_t itemId)
		{
			if (const auto entry = cdbItemWeapons::getInstance().getEntry(itemId); entry)
			{
				return entry->ii_type;
			}
			return std::nullopt;
		}

		inline std::optional<bool> isTradeable(std::uint32_t itemId)
		{
			if (const auto entry = cdbItemWeapons::getInstance().getEntry(itemId); entry)
			{
				return entry->ii_tradeable && (entry->ii_type_inven >= 1 && entry->ii_type_inven <= 21) && entry->ii_type_inven != 10;
			}
			return std::nullopt;
		}

		inline std::optional<std::uint32_t> getBaseItemId(std::uint32_t itemId)
		{
			std::uint32_t currentId = itemId;
			while (true)
			{
				const auto entry = cdbUpgrades::getInstance().getEntry(currentId);
				if (!entry)
					return std::nullopt;
				if (entry->ui_parentid == 0)
					return entry->ui_itemid;
				currentId = entry->ui_parentid;
			}
		}

		inline std::vector<Main::Structures::CapsuleList> getCapsuleEvents(std::uint64_t startDate, std::uint64_t endDate, std::uint32_t newMpPrice, 
			std::uint32_t newRtPrice)
		{
			std::vector<Main::Structures::CapsuleList> ret;

			const std::uint64_t now = static_cast<std::uint64_t>(std::chrono::duration_cast<std::chrono::seconds>(std::chrono::system_clock::now().time_since_epoch()).count());
			const bool isEventActive = now >= startDate && now <= endDate;

			for (const auto& [id, capsuleInfoStruct] : cdbCapsuleInfos::getInstance().getEntries())
			{
				Main::Structures::CapsuleList capsuleSaleEvent;
				capsuleSaleEvent.capsuleInfoId = static_cast<std::uint32_t>(capsuleInfoStruct.gi_id);
				capsuleSaleEvent.saleEventStartDate = isEventActive ? startDate : 0;
				capsuleSaleEvent.saleEventEndDate = isEventActive ? endDate : 0;

				if (isEventActive)
				{
					if (capsuleInfoStruct.gi_type == Main::Enums::CAPSULE_ROCKTOTENS && capsuleInfoStruct.gi_price > 0)
					{
						capsuleSaleEvent.newPrice = newRtPrice;
					}
					else if (capsuleInfoStruct.gi_type == Main::Enums::CAPSULE_MICROPOINTS && capsuleInfoStruct.gi_price > 0)
					{
						capsuleSaleEvent.newPrice = newMpPrice;
					}
					else
					{
						capsuleSaleEvent.newPrice = capsuleInfoStruct.gi_price;
					}
				}
				else
				{
					capsuleSaleEvent.newPrice = capsuleInfoStruct.gi_price;
				}

				ret.push_back(capsuleSaleEvent);
			}

			return ret;
		}

		inline std::optional<Common::ConstantDatabase::CdbCapsuleInfo> getCapsuleInfoById(std::uint32_t gi_id)
		{
			for (const auto& [unused, capsuleInfoStruct] : cdbCapsuleInfos::getInstance().getEntries())
			{
				if (capsuleInfoStruct.gi_id == gi_id) return capsuleInfoStruct;
			}
			return std::nullopt;
		}

		inline std::optional<std::uint32_t> getEventMissionRewardFor(std::uint32_t em_id)
		{
			using evetMission = Common::ConstantDatabase::CdbSingleton<Common::ConstantDatabase::CdbEventMissionInfo>;
			if (const auto entry = evetMission::getInstance().getEntry(em_id); entry)
			{
				return entry->em_rewardid;
			}
			return std::nullopt;
		}

		inline std::array<double, 3> getAverageSpinCostByCurrency()
		{
			std::array<double, 3> averageCosts{}; // [0]=coin, [1]=rt, [2]=mp, [3]=lucky spin
			std::array<double, 3> totalCapsuleTypesByCurrency{};
			for (std::size_t total = 0; const auto & [id, capsuleInfoStruct] : cdbCapsuleInfos::getInstance().getEntries())
			{
				if (capsuleInfoStruct.gi_type >= 3) continue;
				averageCosts[capsuleInfoStruct.gi_type] += capsuleInfoStruct.gi_price;
				++totalCapsuleTypesByCurrency[capsuleInfoStruct.gi_type];
			}
			averageCosts[0] /= totalCapsuleTypesByCurrency[0];
			averageCosts[1] /= totalCapsuleTypesByCurrency[1];
			averageCosts[2] /= totalCapsuleTypesByCurrency[2];

			return averageCosts;
		}

		template<typename T>
		inline std::optional<std::vector<T>> getAllEntries(std::uint32_t id)
			requires std::same_as<T, Common::ConstantDatabase::CdbCapsulePackageInfo> or std::same_as<T, Common::ConstantDatabase::CdbItemPackageInfo>
		{
			using CdbType = std::conditional_t<std::same_as<T, Common::ConstantDatabase::CdbCapsulePackageInfo>, cdbCapsulePackageInfos, itemPackageInfo>;
			if (const auto entries = CdbType::getInstance().getEntriesFor(id); entries)
			{
				return *entries;
			}
			return std::nullopt;
		}

		inline std::vector<Common::ConstantDatabase::CdbCapsulePackageInfo> getAllRareCapsuleItems()
		{
			std::vector<Common::ConstantDatabase::CdbCapsulePackageInfo> rareItems;
			const auto& capsuleInfos = cdbCapsuleInfos::getInstance().getEntries();

			for (const auto& [unused, capsuleInfo] : capsuleInfos)
			{
				if (auto relatedPackages = cdbCapsulePackageInfos::getInstance().getEntriesFor(capsuleInfo.gi_infoid))
				{
					for (const auto& packageInfo : relatedPackages.value())
					{
						if (packageInfo.gi_type == 1)
							rareItems.push_back(packageInfo);
					}
				}
			}
			return rareItems;
		}
	}
}


#endif